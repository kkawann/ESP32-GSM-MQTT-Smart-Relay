#include <Arduino.h>
#include "clock.h"
#include "globals.h"
#include "utils.h"

uint8_t getDayOfWeek()
{
    if (!internalClock.isValid)
        return 0;
    int d = internalClock.day;
    int m = internalClock.month;
    int y = internalClock.year;
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3)
        y--;
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7);
}
bool isWeekdayAllowed(uint8_t mask)
{
    if (mask == 0x00 || mask == 0x7F)
        return true;
    return (mask & (1 << getDayOfWeek())) != 0;
}

void updateInternalClock()
{
    if (!internalClock.isValid)
        return;

    unsigned long now = millis();
    unsigned long elapsed = (now >= internalClock.lastUpdate) ? (now - internalClock.lastUpdate) : (0xFFFFFFFF - internalClock.lastUpdate + now);

    if (elapsed < 1000)
        return;

    uint32_t totalSeconds = elapsed / 1000;
    internalClock.lastUpdate = now;
    internalClock.second += totalSeconds;

    while (internalClock.second >= 60)
    {
        internalClock.second -= 60;
        internalClock.minute++;
    }
    while (internalClock.minute >= 60)
    {
        internalClock.minute -= 60;
        internalClock.hour++;
    }
    while (internalClock.hour >= 24)
    {
        internalClock.hour -= 24;
        internalClock.day++;
        for (int i = 0; i < sceneCount; i++)
            scenes[i].triggeredToday = false;
    }

    uint8_t daysInMonth = 31;
    if (internalClock.month == 2)
    {
        bool leap = (internalClock.year % 4 == 0 && internalClock.year % 100 != 0) ||
                    (internalClock.year % 400 == 0);
        daysInMonth = leap ? 29 : 28;
    }
    else if (internalClock.month == 4 || internalClock.month == 6 ||
             internalClock.month == 9 || internalClock.month == 11)
    {
        daysInMonth = 30;
    }

    if (internalClock.day > daysInMonth)
    {
        internalClock.day = 1;
        internalClock.month++;
        if (internalClock.month > 12)
        {
            internalClock.month = 1;
            internalClock.year++;
        }
    }
}

bool getCurrentTime(uint8_t &h, uint8_t &m, uint8_t &s)
{
    if (!mutexClock)
        return false;

    bool valid = false;
    if (xSemaphoreTake(mutexClock, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        valid = internalClock.isValid;
        if (valid)
        {
            updateInternalClock();
            h = internalClock.hour;
            m = internalClock.minute;
            s = internalClock.second;
        }
        xSemaphoreGive(mutexClock);
    }
    return valid;
}

void startClockSync()
{
    if (clockSyncState != CSYNC_IDLE)
        return;
    if (!sim800Ready || !networkReady)
        return;

    // ✅ جلوگیری از تداخل clock sync با GPRS/TCP
    // اگه GPRS در حال اتصال باشه، خواندن مستقیم سریال جواب GPRS رو می بلعه
    if (sim800.gprs_state != GPRS_IDLE && sim800.gprs_state != GPRS_CONNECTED)
    {
        Serial.println("[CCLK] Skipped - GPRS busy");
        return;
    }
    if (sim800.tcp_state != TCP_IDLE && sim800.tcp_state != TCP_CONNECTED)
    {
        Serial.println("[CCLK] Skipped - TCP busy");
        return;
    }

    clockSyncBuffer = "";
    clockRawBuffer = "";
    clockWaitingResp = true;
    clockSyncState = CSYNC_SENT;
    clockSyncStarted = millis();
    sim800.clockSyncGotCCLK = false; // ✅ ریست فیلد قبل از ارسال دستور

    SIM800Serial.println("AT+CCLK?");
    Serial.println("[CCLK] Sent AT+CCLK?");
}

void processClockSync()
{
    if (clockSyncState == CSYNC_IDLE)
        return;

    unsigned long now = millis();

    // ── Timeout ──
    if ((now - clockSyncStarted) >= CSYNC_TIMEOUT)
    {
        Serial.println("[CCLK] Timeout");
        clockSyncState = CSYNC_IDLE;
        clockSyncBuffer = "";
        clockRawBuffer = "";
        clockWaitingResp = false;
        lastSyncAttempt = now;
        return;
    }

    // ✅ خواندن مستقیم سریال حذف شد
    // SIM800_Process() همیشه اجرا میشه و پاسخ +CCLK رو HandleURC پردازش میکنه
    // فقط کافیه فیلد clockSyncGotCCLK رو چک کنیم

    if (!sim800.clockSyncGotCCLK)
        return; // هنوز پاسخ نیومده

    // ✅ پاسخ +CCLK دریافت شد - زمان رو از فیلدهای SIM800 بخون
    sim800.clockSyncGotCCLK = false;

    uint8_t h   = sim800.net_hour;
    uint8_t mi  = sim800.net_minute;
    uint8_t s   = sim800.net_second;
    uint16_t yr = 2000 + sim800.net_year;
    uint8_t mo  = sim800.net_month;
    uint8_t dy  = sim800.net_day;

    if (h > 23 || mi > 59 || s > 59 ||
        mo == 0 || mo > 12 ||
        dy == 0 || dy > 31)
    {
        Serial.printf("[CCLK] Validation failed: %d/%d/%d %d:%d:%d\n",
                      yr, mo, dy, h, mi, s);
        clockSyncState = CSYNC_IDLE;
        clockWaitingResp = false;
        lastSyncAttempt = now;
        return;
    }

    if (mutexClock &&
        xSemaphoreTake(mutexClock, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        internalClock.hour = h;
        internalClock.minute = mi;
        internalClock.second = s;
        internalClock.day = dy;
        internalClock.month = mo;
        internalClock.year = yr;
        internalClock.lastUpdate = now;
        internalClock.isValid = true;
        xSemaphoreGive(mutexClock);
    }
    else
    {
        Serial.println("[CCLK] Mutex timeout!");
    }

    lastClockSync = now;
    lastSyncAttempt = now;
    clockSyncState = CSYNC_IDLE;
    clockSyncBuffer = "";
    clockRawBuffer = "";
    clockWaitingResp = false;

    Serial.printf("[CCLK] Clock synced: %04d/%02d/%02d %02d:%02d:%02d\n",
                  yr, mo, dy, h, mi, s);

    addLog(4, 0, "Clock synced via GSM");
}