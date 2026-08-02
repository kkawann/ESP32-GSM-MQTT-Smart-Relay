#include <Arduino.h>
#include "gsm.h"
#include "globals.h"
#include "relay.h"
#include "scene.h"
#include "utils.h"
#include "SIM800_Arduino.h"
#include "SIM800_MQTT.h"

// Normalize Iranian phone numbers to 09xxxxxxxxx form
String normalizeNumber(String n)
{
    n.replace(" ", "");
    n.replace("-", "");
    n.replace("(", "");
    n.replace(")", "");
    n.trim();

    if (n.startsWith("+98"))
        n = "0" + n.substring(3);
    else if (n.startsWith("0098"))
        n = "0" + n.substring(4);
    else if (n.startsWith("98") && n.length() == 12)
        n = "0" + n.substring(2);

    return n;
}

void smsCallback(const char *text, const char *sender)
{
    lastSIM800Activity = millis();

    SMSInMessage msg;
    strncpy(msg.sender, sender, sizeof(msg.sender) - 1);
    strncpy(msg.text, text, sizeof(msg.text) - 1);

    if (xQueueSend(qSMSIn, &msg, 0) != pdTRUE)
    {
        Serial.println("[SMS] Input queue full");
    }
}

void initCallback(bool success)
{
    sim800Ready = success;
    networkReady = false;

    if (success)
    {
        lastSIM800Activity = millis();
        pendingTimeSync = true;
        simInitDoneAt = millis();

        gsmFailureDetected = false;
        gsmFailureStart = 0;
        softResetDone = false;
        hwResetInProgress = false;

        if (xSemaphoreTake(mutexClock, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            internalClock.isValid = false;
            xSemaphoreGive(mutexClock);
        }

        lastSyncAttempt = 0;
        addLog(4, 0, "SIM800 init OK, pending time sync");
    }
    else
    {
        addLog(4, 2, "SIM800 init FAILED");
    }
}
void sendRelayNotification(int index, bool isOn, const char *reason)
{
    if (!sim800Ready || !networkReady)
        return;
    if (allowedCount == 0)
        return;
    if (!relays[index].notifySMS)
        return;

    char msg[120];
    const char *nums[] = {"1", "2", "3", "4"};

    if (isOn)
    {
        if (strlen(reason) > 0)
            snprintf(msg, sizeof(msg), "Relay %s ON | %s", nums[index], reason);
        else
            snprintf(msg, sizeof(msg), "Relay %s ON", nums[index]);
    }
    else
    {
        unsigned long onDur = 0;
        if (relays[index].startTime > 0)
            onDur = (millis() - relays[index].startTime) / 1000UL;

        if (strlen(reason) > 0)
            snprintf(msg, sizeof(msg), "Relay %s OFF | %s | on for %lus",
                     nums[index], reason, onDur);
        else
            snprintf(msg, sizeof(msg), "Relay %s OFF | on for %lus",
                     nums[index], onDur);
    }

    for (int i = 0; i < allowedCount; i++)
    {
        if (allowedNumbers[i].length() > 5)
        {
            sendSMSAsync(allowedNumbers[i].c_str(), msg);
        }
    }

    // Publish relay event to MQTT
    if (MQTT_IsConnected(&mqttClient))
    {
        char json[256];
        snprintf(json, sizeof(json),
                 "{\"type\":\"relay\",\"index\":%d,\"state\":\"%s\",\"reason\":\"%.50s\"}",
                 index + 1,
                 isOn ? "ON" : "OFF",
                 reason);
        MQTT_PublishString(&mqttClient, mqttTopicLog, json, false);
    }
}

void handleGSMFailure()
{
    static unsigned long lastHardReset = 0;
    static unsigned long lastSoftReset = 0;

    unsigned long now = millis();

    if (sim800Ready)
    {
        gsmFailureDetected = false;
        gsmFailureStart = 0;
        softResetDone = false;
        return;
    }

    if (!gsmFailureDetected)
    {
        gsmFailureDetected = true;
        gsmFailureStart = now;
        softResetDone = false;
        addLog(4, 1, "GSM failure detected");
        return;
    }

    if (hwResetInProgress)
        return;

    unsigned long dur = now - gsmFailureStart;

    // Hard Reset - حداقل 30 ثانیه فاصله
    if (dur >= GSM_HARD_RESET_TIMEOUT)
    {
        if (now - lastHardReset < 30000)
        {
            Serial.println("[GSM] Hard reset throttled");
            return;
        }

        lastHardReset = now;
        addLog(4, 2, "GSM hard reset triggered");
        hwResetInProgress = true;
        hwResetStage = 0;
        gsmFailureDetected = false;
        softResetDone = false;
        hardwareResetCount++;

        digitalWrite(SIM800_RST_PIN, LOW);
        delay(300); // delay به جای xTaskNotify
        digitalWrite(SIM800_RST_PIN, HIGH);

        // مستقیم reinit کن
        sim800.last_command_success_time = millis();
        SIM800_ForceReinit(&sim800);
        hwResetInProgress = false;

        return;
    }

    // Soft Reset - حداقل 15 ثانیه فاصله
    if (dur >= GSM_SOFT_RESET_TIMEOUT && !softResetDone)
    {
        if (now - lastSoftReset < 15000)
        {
            Serial.println("[GSM] Soft reset throttled");
            return;
        }

        lastSoftReset = now;
        addLog(4, 1, "GSM soft reset");

        sim800.last_command_success_time = millis();

        SIM800_ForceReinit(&sim800);
        sim800Ready = false;
        networkReady = false;
        softResetDone = true;
        softwareResetCount++;
    }
}

// ============================================================================
// Process SMS Command
// ============================================================================

void processSMSCommand(String cmd, const char *sender)
{
    cmd.toUpperCase();
    cmd.trim();

    bool isStatusCmd = (cmd == "?" || cmd == "?!" || cmd == "STATUS" || cmd == "S");

    if (!isStatusCmd)
    {
        char logMsg[64];
        snprintf(logMsg, 64, "SMS: '%s' from %s", cmd.substring(0, 15).c_str(), sender);
        addLog(3, 0, logMsg);

        if (MQTT_IsConnected(&mqttClient))
        {
            char json[256];
            snprintf(json, sizeof(json),
                     "{\"type\":\"sms\",\"level\":\"info\","
                     "\"msg\":\"%.100s\",\"from\":\"%.20s\",\"ts\":%lu}",
                     logMsg, sender, millis() / 1000);
            MQTT_PublishString(&mqttClient, mqttTopicLog, json, false);
        }
    }

    if (cmd == "?!")
    {
        sendLongSMS(sender, getFullStatus());
        return;
    }

    if (cmd == "STATUS" || cmd == "S" || cmd == "?")
    {
        sendSMSAsync(sender, getStatus().c_str());
        return;
    }

    if (cmd == "HELP")
    {
        sendSMSAsync(sender, "COMMANDS:\nSTATUS\nSCENE1\nR1 30s\nON1\nOFF1\nALL OFF");
        return;
    }

    if (cmd.startsWith("SCENE"))
    {
        String param = cmd.substring(5);
        param.trim();
        if (param.length() > 0 && isDigit(param.charAt(0)))
        {
            int id = param.toInt() - 1;
            if (id >= 0 && id < sceneCount)
            {
                executeScene(id);
                sendSMSAsync(sender, ("OK " + String(scenes[id].name)).c_str());
            }
        }
        return;
    }

    if (cmd == "ALL OFF" || cmd == "ALLOFF" || cmd == "STOP")
    {
        for (int i = 0; i < 4; i++)
            deactivateRelayTS(i);
        sendSMSAsync(sender, "All OFF");
        return;
    }

    if (cmd == "ALL ON" || cmd == "ALLON")
    {
        for (int i = 0; i < 4; i++)
            activateRelayTS(i, 0);
        sendSMSAsync(sender, "All ON");
        return;
    }

    if (cmd.startsWith("ON") && cmd.length() == 3)
    {
        int r = cmd.charAt(2) - '1';
        if (r >= 0 && r <= 3)
        {
            activateRelayTS(r, 0);
            sendSMSAsync(sender, ("R" + String(r + 1) + " ON").c_str());
        }
        return;
    }

    if (cmd.startsWith("OFF") && cmd.length() == 4)
    {
        int r = cmd.charAt(3) - '1';
        if (r >= 0 && r <= 3)
        {
            deactivateRelayTS(r);
            sendSMSAsync(sender, ("R" + String(r + 1) + " OFF").c_str());
        }
        return;
    }

    if (cmd.startsWith("R") && cmd.length() >= 2)
    {
        int r = cmd.charAt(1) - '1';
        if (r < 0 || r > 3)
            return;

        int sp = cmd.indexOf(' ');
        if (sp < 0)
        {
            activateRelayTS(r, 0);
            sendSMSAsync(sender, ("R" + String(r + 1) + " ON").c_str());
            return;
        }

        String param = cmd.substring(sp + 1);
        param.trim();
        param.toUpperCase();

        if (param == "ON")
        {
            activateRelayTS(r, 0);
            sendSMSAsync(sender, ("R" + String(r + 1) + " ON").c_str());
            return;
        }
        if (param == "OFF")
        {
            deactivateRelayTS(r);
            sendSMSAsync(sender, ("R" + String(r + 1) + " OFF").c_str());
            return;
        }

        unsigned long dur = parseTime(param);
        if (dur > 0)
        {
            activateRelayTS(r, dur);
            sendSMSAsync(sender, ("R" + String(r + 1) + " " + param).c_str());
        }
    }
}

bool isNumberAllowed(const char *number)
{
    // Empty whitelist = allow all
    if (allowedCount == 0)
    {
        Serial.println("[AUTH] No numbers in list - allowing all");
        return true;
    }

    String incoming = normalizeNumber(String(number));

    Serial.printf("[AUTH] Checking: '%s' (normalized: '%s')\n",
                  number, incoming.c_str());

    for (int i = 0; i < allowedCount; i++)
    {
        String stored = normalizeNumber(allowedNumbers[i]);

        Serial.printf("[AUTH] vs stored[%d]: '%s' (normalized: '%s')\n",
                      i, allowedNumbers[i].c_str(), stored.c_str());

        if (incoming == stored)
        {
            Serial.println("[AUTH] Exact match");
            return true;
        }

        // Match last 10 digits if both long enough
        if (incoming.length() >= 10 && stored.length() >= 10)
        {
            String inLast = incoming.substring(incoming.length() - 10);
            String stLast = stored.substring(stored.length() - 10);

            if (inLast == stLast)
            {
                Serial.println("[AUTH] Last-10 match");
                return true;
            }
        }
    }

    Serial.printf("[AUTH] Number not allowed: %s\n", number);
    return false;
}

// ============================================================================
// parseTime
// ============================================================================

unsigned long parseTime(String timeStr)
{
    timeStr.trim();
    timeStr.toUpperCase();
    if (timeStr.length() == 0)
        return 0;

    char unit = timeStr.charAt(timeStr.length() - 1);
    if (isdigit(unit))
        return timeStr.toInt() * 1000UL;

    int value = timeStr.substring(0, timeStr.length() - 1).toInt();
    if (value <= 0)
        return 0;

    switch (unit)
    {
    case 'S':
        return value * 1000UL;
    case 'M':
        return value * 60000UL;
    case 'H':
        return value * 3600000UL;
    default:
        return 0;
    }
}

// ============================================================================
// getStatus
// ============================================================================

String getStatus()
{
    // Take mutexRelay to safely read relay states
    if (mutexRelay)
        xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(20));

    String s = "=STATUS=\n";

    for (int i = 0; i < 4; i++)
    {
        s += "R" + String(i + 1) + ":";
        if (relays[i].isActive)
        {
            if (relays[i].duration == 0)
            {
                s += "ON";
            }
            else
            {
                unsigned long elapsed = millis() - relays[i].startTime;
                if (elapsed < relays[i].duration)
                {
                    unsigned long rem = (relays[i].duration - elapsed) / 1000;
                    s += "ON(" + String(rem) + "s)";
                }
                else
                {
                    s += "ON(0s)";
                }
            }
        }
        else
        {
            s += "OFF";
        }

        if (i < 3)
            s += " ";
    }

    s += "\nSIG:" + String(SIM800_GetSignalStrength(&sim800));

    if (internalClock.isValid)
    {
        char tbuf[20];
        snprintf(tbuf, sizeof(tbuf), "\nTime:%02d:%02d %04d/%02d/%02d",
                 internalClock.hour, internalClock.minute,
                 internalClock.year, internalClock.month, internalClock.day);
        s += tbuf;
    }
    else
    {
        s += "\nTime:--";
    }

    s += "\nRF:" + String(rfButtonCount);
    s += " SCN:" + String(sceneCount);
    s += " AUTO:" + String(automationCount);

    if (MQTT_IsConnected(&mqttClient))
        s += "\nMQTT:OK";
    else
        s += "\nMQTT:--";

    // Release mutexRelay
    if (mutexRelay)
        xSemaphoreGive(mutexRelay);

    return s;
}

String getFullStatus()
{
    String s = getStatus();

    if (rfSensorCount > 0)
    {
        s += "\n\n=SENSORS=";
        for (int i = 0; i < rfSensorCount; i++)
        {
            if (!rfSensors[i].active)
                continue;
            s += "\n" + String(rfSensors[i].name) + ":";
            if (rfSensors[i].hasValue)
            {
                char vbuf[16];
                snprintf(vbuf, sizeof(vbuf), "%.1f", rfSensors[i].lastValue);
                s += vbuf;
                uint32_t age = ((uint32_t)millis() - rfSensors[i].lastUpdateMs) / 1000;
                s += " (" + String(age) + "s ago)";
            }
            else
                s += "N/A";
        }
    }

    // اتوماسیون‌های فعال
    if (automationCount > 0)
    {
        s += "\n\n=AUTOS=";
        for (int i = 0; i < automationCount; i++)
        {
            s += "\n" + String(automations[i].name) + ":";
            s += automations[i].active ? "ON" : "OFF";
            if (automations[i].triggered)
                s += "*";
        }
    }

    // لاگ آخر ۱۰ رویداد
    s += "\n\n=LOGS=";
    int shown = 0;
    for (int i = 0; i < MAX_EVENT_LOGS && shown < 10; i++)
    {
        int idx = (logIndex - 1 - i + MAX_EVENT_LOGS) % MAX_EVENT_LOGS;
        if (!eventLogs[idx].active)
            continue;

        const char *types[] = {"RLY", "SCN", "RF", "SMS", "SYS"};
        const char *levels[] = {"I", "W", "E"};

        uint8_t t = min((uint8_t)4, eventLogs[idx].type);
        uint8_t l = min((uint8_t)2, eventLogs[idx].level);

        char lbuf[72];
        snprintf(lbuf, sizeof(lbuf), "\n[%s:%s] %s",
                 types[t], levels[l], eventLogs[idx].message);
        s += lbuf;
        shown++;
    }
    if (shown == 0)
        s += "\nNo logs";

    char hbuf[24];
    snprintf(hbuf, sizeof(hbuf), "\n\nHeap:%u", ESP.getFreeHeap());
    s += hbuf;

    return s;
}

// ============================================================================
// SMS Async
// ============================================================================

void sendSMSAsync(const char *number, const char *text)
{
    SMSOutMessage msg;
    strncpy(msg.number, number, 19);
    msg.number[19] = '\0';
    strncpy(msg.text, text, 159);
    msg.text[159] = '\0';

    if (xQueueSend(qSMSOut, &msg, pdMS_TO_TICKS(200)) != pdTRUE)
    {
        Serial.println("⚠ SMS queue full!");
    }
}

void sendLongSMS(const char *number, String text)
{
    int len = text.length();
    if (len <= 159)
    {
        sendSMSAsync(number, text.c_str());
        return;
    }

    int part = 1;
    for (int i = 0; i < len; i += 155)
    {
        String chunk = String(part) + ">" + text.substring(i, min(i + 155, len));
        sendSMSAsync(number, chunk.c_str());
        part++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}