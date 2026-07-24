#include "utils.h"
#include "globals.h"
#include "SIM800_MQTT.h"

void addLog(uint8_t type, uint8_t level, const char *msg)
{
    eventLogs[logIndex].timestamp = millis();
    eventLogs[logIndex].type = type;
    eventLogs[logIndex].level = level;
    strncpy(eventLogs[logIndex].message, msg, 63);
    eventLogs[logIndex].message[63] = '\0';
    eventLogs[logIndex].active = true;
    logIndex = (logIndex + 1) % MAX_EVENT_LOGS;
    if (logCount < MAX_EVENT_LOGS)
        logCount++;

    if (level > 0)
    {
        const char *types[] = {"SYS", "RELAY", "RF", "SMS", "GSM", "SCENE", "SENSOR", "AUTO"};
        const char *levels[] = {"I", "W", "E"};

        if (type < 8)
        {
            Serial.printf("[%s %s] %s\n", types[type], levels[level], msg);
        }
    }

    if (MQTT_IsConnected(&mqttClient))
    {
        const char *typeStr = "unknown";
        switch (type)
        {
        case 0:
            typeStr = "system";
            break;
        case 1:
            typeStr = "relay";
            break;
        case 2:
            typeStr = "rf";
            break;
        case 3:
            typeStr = "sms";
            break;
        case 4:
            typeStr = "gsm";
            break;
        case 5:
            typeStr = "scene";
            break;
        case 6:
            typeStr = "sensor";
            break;
        case 7:
            typeStr = "automation";
            break;
        }

        const char *levelStr = (level == 0) ? "info" : ((level == 1) ? "warn" : "error");

        char json[256];
        snprintf(json, sizeof(json),
                 "{\"type\":\"%s\",\"level\":\"%s\",\"msg\":\"%.150s\",\"ts\":%lu}",
                 typeStr, levelStr, msg, millis() / 1000);

        MQTT_PublishString(&mqttClient, mqttTopicLog, json, false);
    }
}