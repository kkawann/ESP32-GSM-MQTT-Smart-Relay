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

    clockSyncBuffer = "";
    clockRawBuffer = "";
    clockWaitingResp = true;
    clockSyncState = CSYNC_SENT;
    clockSyncStarted = millis();

    SIM800Serial.println("AT+CCLK?");
    Serial.println("[CCLK] Sent AT+CCLK?");
}

void processClockSync()
{
    if (clockSyncState == CSYNC_IDLE)
        return;

    unsigned long now = millis();

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

    // Read serial here only (not inside SIM800_Process)
    while (SIM800Serial.available())
    {
        char c = (char)SIM800Serial.read();
        clockRawBuffer += c;
        lastSIM800Activity = now;
    }

    if (clockRawBuffer.indexOf("OK") < 0 &&
        clockRawBuffer.indexOf("ERROR") < 0)
    {
        return; // incomplete response
    }

    Serial.print("[CCLK] Raw: ");
    Serial.println(clockRawBuffer);

    int start = clockRawBuffer.indexOf("+CCLK: \"");
    if (start < 0)
        start = clockRawBuffer.indexOf("+CCLK:\"");

    if (start < 0)
    {
        Serial.println("[CCLK] No +CCLK in response");
        clockSyncState = CSYNC_IDLE;
        clockRawBuffer = "";
        clockSyncBuffer = "";
        clockWaitingResp = false;
        lastSyncAttempt = now;
        return;
    }

    int q1 = clockRawBuffer.indexOf('"', start);
    int q2 = clockRawBuffer.indexOf('"', q1 + 1);

    if (q1 < 0 || q2 < 0 || q2 <= q1 + 1)
    {
        Serial.println("[CCLK] Quote parse failed");
        clockSyncState = CSYNC_IDLE;
        clockRawBuffer = "";
        clockWaitingResp = false;
        lastSyncAttempt = now;
        return;
    }

    // Example content: 25/05/30,14:25:10+14
    String content = clockRawBuffer.substring(q1 + 1, q2);
    Serial.print("[CCLK] Content: ");
    Serial.println(content);

    int comma = content.indexOf(',');
    if (comma < 0)
    {
        Serial.println("[CCLK] No comma found");
        clockSyncState = CSYNC_IDLE;
        clockRawBuffer = "";
        clockWaitingResp = false;
        lastSyncAttempt = now;
        return;
    }

    String datePart = content.substring(0, comma);  // YY/MM/DD
    String timeFull = content.substring(comma + 1); // HH:MM:SS±zz

    // Strip timezone offset from time
    String timePart = timeFull;
    int plusIdx = timeFull.lastIndexOf('+');
    int minusIdx = timeFull.lastIndexOf('-');
    if (plusIdx > 5)
        timePart = timeFull.substring(0, plusIdx);
    if (minusIdx > 5)
        timePart = timeFull.substring(0, minusIdx);

    Serial.print("[CCLK] date=");
    Serial.print(datePart);
    Serial.print(" time=");
    Serial.println(timePart);

    if (datePart.length() < 8 || timePart.length() < 8)
    {
        Serial.println("[CCLK] Length check failed");
        clockSyncState = CSYNC_IDLE;
        clockRawBuffer = "";
        clockWaitingResp = false;
        lastSyncAttempt = now;
        return;
    }

    uint8_t h = (uint8_t)timePart.substring(0, 2).toInt();
    uint8_t mi = (uint8_t)timePart.substring(3, 5).toInt();
    uint8_t s = (uint8_t)timePart.substring(6, 8).toInt();
    uint16_t year = 2000 + (uint16_t)datePart.substring(0, 2).toInt();
    uint8_t month = (uint8_t)datePart.substring(3, 5).toInt();
    uint8_t day = (uint8_t)datePart.substring(6, 8).toInt();

    if (h > 23 || mi > 59 || s > 59 ||
        month == 0 || month > 12 ||
        day == 0 || day > 31)
    {
        Serial.printf("[CCLK] Validation failed: %d/%d/%d %d:%d:%d\n",
                      year, month, day, h, mi, s);
        clockSyncState = CSYNC_IDLE;
        clockRawBuffer = "";
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
        internalClock.day = day;
        internalClock.month = month;
        internalClock.year = year;
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
    clockRawBuffer = "";
    clockSyncBuffer = "";
    clockWaitingResp = false;

    Serial.printf("[CCLK] Clock synced: %04d/%02d/%02d %02d:%02d:%02d\n",
                  year, month, day, h, mi, s);

    addLog(4, 0, "Clock synced via GSM");
}