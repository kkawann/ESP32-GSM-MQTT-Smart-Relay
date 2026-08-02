#include "tasks.h"
#include "globals.h"
#include "relay.h"
#include "gsm.h"
#include "clock.h"
#include "rf.h"
#include "sensor.h"
#include "scene.h"
#include "automation.h"
#include "storage.h"
#include "utils.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include "esp_task_wdt.h"
#include "SIM800_MQTT.h"

// Timer callbacks
void cbRelayTimer(TimerHandle_t t)
{
    uint32_t idx = (uint32_t)pvTimerGetTimerID(t);
    RelayCommand cmd = {(uint8_t)idx, 0, 0};
    xQueueSend(qRelay, &cmd, 0);
}

void cbGSMCheck(TimerHandle_t t)
{
    if (taskGSM)
        xTaskNotify(taskGSM, 0x01, eSetBits);
}

void cbClockSync(TimerHandle_t t)
{
    if (taskGSM)
        xTaskNotify(taskGSM, 0x02, eSetBits);
}

void cbLED(TimerHandle_t t)
{
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}
// ============================================================================
// MQTT Response Helper
// ============================================================================
static void mqttRespond(const char *json)
{
    MQTT_PublishString(&mqttClient, mqttTopicStatus, json, false);
}

// ============================================================================
// MQTT Message Callback — full command set
// ============================================================================
void mqttMessageCallback(const char *topic, const uint8_t *payload, uint16_t len)
{
    Serial.print("[MQTT RX] Topic: ");
    Serial.println(topic);

    Serial.print("   Payload: ");
    Serial.write(payload, len);
    Serial.println();

    char message[512];
    if (len < sizeof(message))
    {
        memcpy(message, payload, len);
        message[len] = '\0';
    }
    else
    {
        Serial.println("[MQTT] Payload too large");
        return;
    }

    // ── Relay topic: device/SmartRelay_001/relay/{1-4} ────────────────
    if (strstr(topic, "/relay/") != NULL)
    {
        char *relayNumStr = strrchr(topic, '/') + 1;
        int relayNum = atoi(relayNumStr);
        if (relayNum >= 1 && relayNum <= 4)
        {
            bool turnOn = (strcmp(message, "ON") == 0 || strcmp(message, "1") == 0);
            bool isToggle = (strcmp(message, "TOGGLE") == 0 || strcmp(message, "2") == 0);
            RelayCommand cmd;
            if (isToggle)
                cmd = {(uint8_t)(relayNum - 1), 2, 0};
            else
                cmd = {(uint8_t)(relayNum - 1), (uint8_t)(turnOn ? 1 : 0), 0};
            xQueueSend(qRelay, &cmd, 0);
            Serial.printf("[MQTT] Relay %d %s\n", relayNum, turnOn ? "ON" : isToggle ? "TOGGLE" : "OFF");
        }
        return;
    }

    // ── Command topic: device/SmartRelay_001/cmd ──────────────────────
    if (strcmp(topic, mqttTopicCommand) != 0)
        return;

    // ── RELAY: R{1-4}:{ON|OFF|TOGGLE} ─────────────────────────────────
    if (message[0] == 'R' && message[1] >= '1' && message[1] <= '4')
    {
        int idx = message[1] - '1';
        RelayCommand cmd;
        if (strstr(message + 2, "ON"))
            cmd = {(uint8_t)idx, 1, 0};
        else if (strstr(message + 2, "OFF"))
            cmd = {(uint8_t)idx, 0, 0};
        else
            cmd = {(uint8_t)idx, 2, 0};
        xQueueSend(qRelay, &cmd, 0);
        Serial.printf("[MQTT] Relay %d cmd: %s\n", idx + 1, message + 2);
        return;
    }

    // ── STATUS ─────────────────────────────────────────────────────────
    if (strcmp(message, "STATUS") == 0)
    {
        if (taskMQTT)
            xTaskNotify(taskMQTT, 0x01, eSetBits);
        return;
    }

    // ── RESTART ────────────────────────────────────────────────────────
    if (strcmp(message, "RESTART") == 0)
    {
        Serial.println("[MQTT] Remote restart requested");
        addLog(4, 2, "MQTT restart cmd");
        delay(500);
        ESP.restart();
        return;
    }

    // ── LOG_LIST ───────────────────────────────────────────────────────
    if (strcmp(message, "LOG_LIST") == 0)
    {
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();
        int count = min(logCount, MAX_EVENT_LOGS);
        for (int i = 0; i < count; i++)
        {
            int idx = (logIndex - 1 - i + MAX_EVENT_LOGS) % MAX_EVENT_LOGS;
            if (!eventLogs[idx].active)
                continue;
            JsonObject obj = arr.createNestedObject();
            obj["time"] = eventLogs[idx].timestamp / 1000;
            obj["type"] = eventLogs[idx].type;
            obj["level"] = eventLogs[idx].level;
            obj["msg"] = eventLogs[idx].message;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── LOG_CLEAR ──────────────────────────────────────────────────────
    if (strcmp(message, "LOG_CLEAR") == 0)
    {
        logIndex = 0;
        logCount = 0;
        for (int i = 0; i < MAX_EVENT_LOGS; i++)
            eventLogs[i].active = false;
        mqttRespond("{\"ok\":true}");
        addLog(5, 0, "Logs cleared via MQTT");
        return;
    }

    // ── SCENE_LIST ─────────────────────────────────────────────────────
    if (strcmp(message, "SCENE_LIST") == 0)
    {
        DynamicJsonDocument doc(6144);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < sceneCount; i++)
        {
            if (!scenes[i].active)
                continue;
            JsonObject obj = arr.createNestedObject();
            obj["id"] = scenes[i].id;
            obj["name"] = scenes[i].name;
            obj["isSequential"] = scenes[i].isSequential;
            obj["timeEnabled"] = scenes[i].timeEnabled;
            obj["triggerHour"] = scenes[i].triggerHour;
            obj["triggerMinute"] = scenes[i].triggerMinute;
            obj["weekdayMask"] = scenes[i].weekdayMask;
            obj["repeatInterval"] = scenes[i].repeatInterval;
            JsonArray steps = obj.createNestedArray("steps");
            for (int j = 0; j < scenes[i].stepCount; j++)
            {
                JsonObject step = steps.createNestedObject();
                step["relay"] = scenes[i].steps[j].relay;
                step["action"] = scenes[i].steps[j].action;
                step["duration"] = scenes[i].steps[j].duration;
                step["delayBefore"] = scenes[i].steps[j].delayBefore;
            }
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── SCENE_RUN:{id} ────────────────────────────────────────────────
    if (strncmp(message, "SCENE_RUN:", 10) == 0)
    {
        int id = atoi(message + 10);
        executeScene(id);
        char resp[32];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"id\":%d}", id);
        mqttRespond(resp);
        return;
    }

    // ── SCENE_SAVE:{JSON} ─────────────────────────────────────────────
    if (strncmp(message, "SCENE_SAVE:", 11) == 0)
    {
        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, message + 11);
        if (err)
        {
            mqttRespond("{\"error\":\"json\"}");
            return;
        }
        if (sceneCount >= MAX_SCENES)
        {
            mqttRespond("{\"error\":\"full\"}");
            return;
        }
        scenes[sceneCount].id = sceneCount;
        String name = doc["name"] | String("Scene");
        strncpy(scenes[sceneCount].name, name.c_str(), 31);
        scenes[sceneCount].name[31] = '\0';
        scenes[sceneCount].isSequential = doc["isSequential"];
        scenes[sceneCount].active = true;
        scenes[sceneCount].timeEnabled = doc["timeEnabled"] | false;
        scenes[sceneCount].triggerHour = doc["triggerHour"] | 0;
        scenes[sceneCount].triggerMinute = doc["triggerMinute"] | 0;
        scenes[sceneCount].weekdayMask = doc["weekdayMask"] | 0x7F;
        scenes[sceneCount].repeatInterval = doc["repeatInterval"] | 0;
        scenes[sceneCount].triggeredToday = false;
        scenes[sceneCount].lastRunMs = 0;
        JsonArray steps = doc["steps"];
        scenes[sceneCount].stepCount = min((int)steps.size(), MAX_SCENE_STEPS);
        for (int i = 0; i < scenes[sceneCount].stepCount; i++)
        {
            scenes[sceneCount].steps[i].relay = steps[i]["relay"];
            scenes[sceneCount].steps[i].action = steps[i]["action"];
            scenes[sceneCount].steps[i].duration = steps[i]["duration"];
            scenes[sceneCount].steps[i].delayBefore = steps[i]["delayBefore"];
        }
        sceneCount++;
        saveAllSettingsAsync();
        char resp[32];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"id\":%d}", sceneCount - 1);
        mqttRespond(resp);
        return;
    }

    // ── SCENE_DEL:{id} ────────────────────────────────────────────────
    if (strncmp(message, "SCENE_DEL:", 10) == 0)
    {
        int id = atoi(message + 10);
        bool found = false;
        for (int i = 0; i < sceneCount; i++)
        {
            if (scenes[i].id == id)
            {
                for (int j = i; j < sceneCount - 1; j++)
                {
                    scenes[j] = scenes[j + 1];
                    scenes[j].id = j;
                }
                sceneCount--;
                if (currentSceneRunning == i)
                    currentSceneRunning = -1;
                else if (currentSceneRunning > i)
                    currentSceneRunning--;
                saveAllSettingsAsync();
                found = true;
                break;
            }
        }
        mqttRespond(found ? "{\"ok\":true}" : "{\"error\":\"not_found\"}");
        return;
    }

    // ── SENSOR_LIST ────────────────────────────────────────────────────
    if (strcmp(message, "SENSOR_LIST") == 0)
    {
        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();
        const char *units[] = {"%", "C", "%RH", "cm", "V", "bool", "bool", ""};
        const char *typeNames[] = {"percent", "temp", "humidity", "distance", "voltage", "door", "motion", "custom"};
        for (int i = 0; i < rfSensorCount; i++)
        {
            if (!rfSensors[i].active)
                continue;
            uint8_t sid = getSensorTypeFromMask(rfSensors[i].baseMask);
            int vt = min((int)sid, 7);
            JsonObject o = arr.createNestedObject();
            o["id"] = i;
            o["name"] = rfSensors[i].name;
            o["sensorTypeId"] = sid;
            o["typeName"] = typeNames[vt];
            o["unit"] = units[vt];
            o["baseCode"] = rfSensors[i].baseCode;
            o["baseMask"] = rfSensors[i].baseMask;
            o["valueBits"] = rfSensors[i].valueBits;
            o["scale"] = rfSensors[i].scale;
            o["offset"] = rfSensors[i].offset;
            o["protocol"] = rfSensors[i].protocol;
            o["bitLength"] = rfSensors[i].bitLength;
            o["hasValue"] = rfSensors[i].hasValue;
            o["lastValue"] = rfSensors[i].hasValue ? (float)((int)(rfSensors[i].lastValue * 10 + 0.5f)) / 10.0f : 0.0f;
            o["rxCount"] = rfSensors[i].rxCount;
            o["ageS"] = rfSensors[i].hasValue ? ((uint32_t)millis() - rfSensors[i].lastUpdateMs) / 1000UL : (uint32_t)9999;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── SENSOR_VALUES ──────────────────────────────────────────────────
    if (strcmp(message, "SENSOR_VALUES") == 0)
    {
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.to<JsonArray>();
        const char *units[] = {"%", "C", "%RH", "cm", "V", "bool", "bool", ""};
        for (int i = 0; i < rfSensorCount; i++)
        {
            if (!rfSensors[i].active)
                continue;
            uint8_t sid = getSensorTypeFromMask(rfSensors[i].baseMask);
            JsonObject o = arr.createNestedObject();
            o["id"] = i;
            o["name"] = rfSensors[i].name;
            o["unit"] = units[min((int)sid, 7)];
            o["hasValue"] = rfSensors[i].hasValue;
            o["value"] = rfSensors[i].hasValue ? rfSensors[i].lastValue : 0.0f;
            o["ageS"] = rfSensors[i].hasValue ? ((uint32_t)millis() - rfSensors[i].lastUpdateMs) / 1000UL : (uint32_t)9999;
            o["rxCount"] = rfSensors[i].rxCount;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── SENSOR_SAVE:{JSON} ────────────────────────────────────────────
    if (strncmp(message, "SENSOR_SAVE:", 12) == 0)
    {
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, message + 12);
        if (err)
        {
            mqttRespond("{\"error\":\"json\"}");
            return;
        }
        int id = doc["id"] | -1;
        bool isNew = (id == -1);
        if (isNew)
        {
            if (rfSensorCount >= MAX_RF_SENSORS)
            {
                mqttRespond("{\"error\":\"full\"}");
                return;
            }
            id = rfSensorCount;
            memset(&rfSensors[id], 0, sizeof(RFSensor));
            memset(&sensorHistory[id], 0, sizeof(SensorRingBuf));
        }
        if (id < 0 || id >= MAX_RF_SENSORS)
        {
            mqttRespond("{\"error\":\"invalid_id\"}");
            return;
        }
        if (doc.containsKey("name"))
        {
            String nm = doc["name"].as<String>();
            strncpy(rfSensors[id].name, nm.c_str(), 31);
            rfSensors[id].name[31] = '\0';
        }
        if (doc.containsKey("valueType"))
            rfSensors[id].valueType = doc["valueType"];
        if (doc.containsKey("baseCode"))
            rfSensors[id].baseCode = (uint32_t)(long)doc["baseCode"];
        if (doc.containsKey("baseMask"))
            rfSensors[id].baseMask = (uint32_t)(long)doc["baseMask"];
        if (doc.containsKey("valueBits"))
            rfSensors[id].valueBits = doc["valueBits"];
        if (doc.containsKey("scale"))
            rfSensors[id].scale = doc["scale"];
        if (doc.containsKey("offset"))
            rfSensors[id].offset = doc["offset"];
        if (doc.containsKey("protocol"))
            rfSensors[id].protocol = doc["protocol"];
        if (doc.containsKey("bitLength"))
            rfSensors[id].bitLength = doc["bitLength"];
        uint8_t sid = SID_CUSTOM;
        if (doc.containsKey("sensorTypeId"))
            sid = (uint8_t)doc["sensorTypeId"];
        else if (doc.containsKey("valueType"))
            sid = (uint8_t)doc["valueType"];
        packSensorTypeIntoMask(rfSensors[id], sid);
        rfSensors[id].active = true;
        if (isNew)
            rfSensorCount++;
        saveSensorsAsync();
        char resp[32];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"id\":%d}", id);
        mqttRespond(resp);
        return;
    }

    // ── SENSOR_DEL:{id} ───────────────────────────────────────────────
    if (strncmp(message, "SENSOR_DEL:", 11) == 0)
    {
        int id = atoi(message + 11);
        if (id < 0 || id >= rfSensorCount)
        {
            mqttRespond("{\"error\":\"not_found\"}");
            return;
        }
        for (int j = id; j < rfSensorCount - 1; j++)
        {
            rfSensors[j] = rfSensors[j + 1];
            sensorHistory[j] = sensorHistory[j + 1];
        }
        rfSensorCount--;
        memset(&rfSensors[rfSensorCount], 0, sizeof(RFSensor));
        memset(&sensorHistory[rfSensorCount], 0, sizeof(SensorRingBuf));
        saveSensorsAsync();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── SENSOR_LEARN ───────────────────────────────────────────────────
    if (strcmp(message, "SENSOR_LEARN") == 0)
    {
        s_learnedCode = 0;
        s_learnWaiting = true;
        s_learnStartMs = millis();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── SENSOR_LEARN_STATUS ────────────────────────────────────────────
    if (strcmp(message, "SENSOR_LEARN_STATUS") == 0)
    {
        if (s_learnWaiting && (millis() - s_learnStartMs) > 30000UL)
        {
            s_learnWaiting = false;
            s_learnedCode = 0;
        }
        bool done = (s_learnedCode != 0 && !s_learnWaiting);
        StaticJsonDocument<256> doc;
        doc["active"] = s_learnWaiting;
        doc["done"] = done;
        doc["rawCode"] = s_learnedCode;
        doc["protocol"] = s_learnedProtocol;
        doc["bitLength"] = s_learnedBits;
        if (done)
        {
            uint32_t fullMask = (s_learnedBits >= 32) ? 0xFFFFFFFFUL : ((1UL << s_learnedBits) - 1);
            uint32_t valMask = 0xFF & fullMask;
            uint32_t baseMask = (~valMask) & fullMask;
            doc["baseCode"] = s_learnedCode & baseMask;
            doc["baseMask"] = baseMask;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── SENSOR_LEARN_CANCEL ────────────────────────────────────────────
    if (strcmp(message, "SENSOR_LEARN_CANCEL") == 0)
    {
        s_learnWaiting = false;
        s_learnedCode = 0;
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── AUTO_LIST ──────────────────────────────────────────────────────
    if (strcmp(message, "AUTO_LIST") == 0)
    {
        DynamicJsonDocument doc(8192);
        JsonArray arr = doc.to<JsonArray>();
        const char *condNames[] = {"always", "relay_on", "relay_off", "time_between", "sensor_gt", "sensor_lt", "sensor_between", "sensor_outside", "sensor_offline", "day_of_week"};
        const char *actNames[] = {"none", "relay_on", "relay_off", "toggle", "timed", "all_off", "scene", "sms", "pulse"};
        for (int i = 0; i < automationCount; i++)
        {
            Automation &a = automations[i];
            JsonObject obj = arr.createNestedObject();
            obj["id"] = a.id;
            obj["name"] = a.name;
            obj["active"] = a.active;
            obj["logicOp"] = a.logicOp;
            obj["cooldownEnabled"] = a.cooldownEnabled;
            obj["cooldownMinutes"] = a.cooldownMinutes;
            obj["triggered"] = a.triggered;
            JsonArray conds = obj.createNestedArray("conditions");
            for (int j = 0; j < a.conditionCount; j++)
            {
                JsonObject c = conds.createNestedObject();
                c["type"] = a.conditions[j].type;
                c["typeName"] = condNames[min((int)a.conditions[j].type, 9)];
                c["relayId"] = a.conditions[j].relayId;
                c["sensorId"] = a.conditions[j].sensorId;
                JsonArray sids = c.createNestedArray("sensorIds");
                for (uint8_t k = 0; k < a.conditions[j].sensorIdCount && k < MAX_SENSORS_PER_COND; k++)
                    sids.add(a.conditions[j].sensorIds[k]);
                c["sensorIdCount"] = a.conditions[j].sensorIdCount;
                c["thresh1"] = a.conditions[j].thresh1;
                c["thresh2"] = a.conditions[j].thresh2;
                c["hourStart"] = a.conditions[j].hourStart;
                c["hourEnd"] = a.conditions[j].hourEnd;
                c["minuteStart"] = a.conditions[j].minuteStart;
                c["minuteEnd"] = a.conditions[j].minuteEnd;
                c["weekdayMask"] = a.conditions[j].weekdayMask;
                c["offlineMinutes"] = a.conditions[j].offlineMinutes;
                c["negate"] = a.conditions[j].negate;
            }
            JsonArray acts = obj.createNestedArray("actions");
            for (int j = 0; j < a.actionCount; j++)
            {
                JsonObject ac = acts.createNestedObject();
                ac["type"] = a.actions[j].type;
                ac["typeName"] = actNames[min((int)a.actions[j].type, 8)];
                ac["targetId"] = a.actions[j].targetId;
                ac["durationMs"] = a.actions[j].durationMs;
                ac["smsText"] = a.actions[j].smsText;
                ac["pulseOnMs"] = a.actions[j].pulseOnMs;
                ac["pulseOffMs"] = a.actions[j].pulseOffMs;
                ac["pulseCount"] = a.actions[j].pulseCount;
                ac["delayBeforeMs"] = a.actions[j].delayBeforeMs;
            }
            JsonObject hyst = obj.createNestedObject("hysteresis");
            hyst["enabled"] = a.hysteresis.enabled;
            hyst["onThreshold"] = a.hysteresis.onThreshold;
            hyst["offThreshold"] = a.hysteresis.offThreshold;
            hyst["sensorId"] = a.hysteresis.sensorId;
            hyst["relayId"] = a.hysteresis.relayId;
            hyst["state"] = a.hysteresis.state;
            obj["lastTriggerMs"] = (uint32_t)a.lastTriggerMs;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── AUTO_SAVE:{JSON} ──────────────────────────────────────────────
    if (strncmp(message, "AUTO_SAVE:", 10) == 0)
    {
        DynamicJsonDocument doc(4096);
        DeserializationError err = deserializeJson(doc, message + 10);
        if (err)
        {
            mqttRespond("{\"error\":\"json\"}");
            return;
        }
        int id = doc["id"] | -1;
        bool isNew = (id == -1);
        if (!isNew && (id < 0 || id >= automationCount))
        {
            mqttRespond("{\"error\":\"not_found\"}");
            return;
        }
        if (isNew)
        {
            if (automationCount >= MAX_AUTOMATIONS)
            {
                mqttRespond("{\"error\":\"full\"}");
                return;
            }
            id = automationCount;
            memset(&automations[id], 0, sizeof(Automation));
        }
        Automation &a = automations[id];
        a.id = id;
        a.active = doc["active"] | true;
        a.logicOp = doc["logicOp"] | (uint8_t)LOGIC_AND;
        String nm = doc["name"] | String("Automation");
        strncpy(a.name, nm.c_str(), 31);
        a.name[31] = '\0';
        a.cooldownEnabled = doc["cooldownEnabled"] | false;
        a.cooldownMinutes = doc["cooldownMinutes"] | (uint16_t)0;
        JsonArray conds = doc["conditions"];
        a.conditionCount = 0;
        if (conds)
        {
            for (JsonObject c : conds)
            {
                if (a.conditionCount >= MAX_CONDITIONS)
                    break;
                int j = a.conditionCount;
                a.conditions[j].type = c["type"] | (uint8_t)0;
                a.conditions[j].relayId = c["relayId"] | (uint8_t)0;
                a.conditions[j].sensorId = c["sensorId"] | (uint8_t)0;
                a.conditions[j].thresh1 = c["thresh1"] | 0.0f;
                a.conditions[j].thresh2 = c["thresh2"] | 100.0f;
                a.conditions[j].hourStart = c["hourStart"] | (uint8_t)0;
                a.conditions[j].hourEnd = c["hourEnd"] | (uint8_t)23;
                a.conditions[j].minuteStart = c["minuteStart"] | (uint8_t)0;
                a.conditions[j].minuteEnd = c["minuteEnd"] | (uint8_t)59;
                a.conditions[j].weekdayMask = c["weekdayMask"] | (uint8_t)0x7F;
                a.conditions[j].offlineMinutes = c["offlineMinutes"] | (uint16_t)10;
                a.conditions[j].negate = c["negate"] | false;
                a.conditions[j].sensorIdCount = 0;
                memset(a.conditions[j].sensorIds, 0, MAX_SENSORS_PER_COND);
                JsonArray sids = c["sensorIds"];
                if (sids)
                {
                    for (uint8_t k = 0; k < MAX_SENSORS_PER_COND && k < sids.size(); k++)
                    {
                        a.conditions[j].sensorIds[k] = sids[k] | (uint8_t)0;
                        a.conditions[j].sensorIdCount++;
                    }
                }
                a.conditionCount++;
            }
        }
        JsonArray acts = doc["actions"];
        a.actionCount = 0;
        if (acts)
        {
            for (JsonObject ac : acts)
            {
                if (a.actionCount >= MAX_ACTIONS)
                    break;
                int j = a.actionCount;
                a.actions[j].type = ac["type"] | (uint8_t)0;
                a.actions[j].targetId = ac["targetId"] | (uint8_t)0;
                a.actions[j].durationMs = ac["durationMs"] | (uint32_t)0;
                a.actions[j].pulseOnMs = ac["pulseOnMs"] | (uint16_t)500;
                a.actions[j].pulseOffMs = ac["pulseOffMs"] | (uint16_t)500;
                a.actions[j].pulseCount = ac["pulseCount"] | (uint8_t)3;
                a.actions[j].delayBeforeMs = ac["delayBeforeMs"] | (uint16_t)0;
                String sms = ac["smsText"] | String("");
                strncpy(a.actions[j].smsText, sms.c_str(), 79);
                a.actions[j].smsText[79] = '\0';
                a.actionCount++;
            }
        }
        if (doc.containsKey("hysteresis"))
        {
            JsonObject h = doc["hysteresis"];
            a.hysteresis.enabled = h["enabled"] | false;
            a.hysteresis.onThreshold = h["onThreshold"] | 0.0f;
            a.hysteresis.offThreshold = h["offThreshold"] | 0.0f;
            a.hysteresis.sensorId = h["sensorId"] | (uint8_t)0;
            a.hysteresis.relayId = h["relayId"] | (uint8_t)0;
            a.hysteresis.state = false;
        }
        a.lastTriggerMs = 0;
        a.lastEval = false;
        a.triggered = false;
        if (isNew)
            automationCount++;
        saveAutomationsAsync();
        char resp[32];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"id\":%d}", id);
        mqttRespond(resp);
        return;
    }

    // ── AUTO_DEL:{id} ─────────────────────────────────────────────────
    if (strncmp(message, "AUTO_DEL:", 9) == 0)
    {
        int id = atoi(message + 9);
        if (id < 0 || id >= automationCount)
        {
            mqttRespond("{\"error\":\"not_found\"}");
            return;
        }
        for (int j = id; j < automationCount - 1; j++)
        {
            automations[j] = automations[j + 1];
            automations[j].id = j;
        }
        automationCount--;
        memset(&automations[automationCount], 0, sizeof(Automation));
        saveAutomationsAsync();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── AUTO_TOGGLE:{id} ──────────────────────────────────────────────
    if (strncmp(message, "AUTO_TOGGLE:", 12) == 0)
    {
        int id = atoi(message + 12);
        if (id < 0 || id >= automationCount)
        {
            mqttRespond("{\"error\":\"not_found\"}");
            return;
        }
        automations[id].active = !automations[id].active;
        automations[id].lastEval = false;
        saveAutomationsAsync();
        char resp[48];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"id\":%d,\"active\":%s}",
                 id, automations[id].active ? "true" : "false");
        mqttRespond(resp);
        return;
    }

    // ── AUTO_TEST:{id} ────────────────────────────────────────────────
    if (strncmp(message, "AUTO_TEST:", 10) == 0)
    {
        int id = atoi(message + 10);
        if (id < 0 || id >= automationCount)
        {
            mqttRespond("{\"error\":\"not_found\"}");
            return;
        }
        bool eval = evalAutomationNoTime(automations[id]);
        for (int j = 0; j < automations[id].actionCount; j++)
            executeAutoAction(automations[id].actions[j]);
        char resp[64];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"id\":%d,\"eval\":%s,\"actions\":%d}",
                 id, eval ? "true" : "false", automations[id].actionCount);
        mqttRespond(resp);
        return;
    }

    // ── RF_LIST ────────────────────────────────────────────────────────
    if (strcmp(message, "RF_LIST") == 0)
    {
        DynamicJsonDocument doc(3072);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < rfButtonCount; i++)
        {
            if (!rfButtons[i].active)
                continue;
            JsonObject obj = arr.createNestedObject();
            obj["code"] = rfButtons[i].code;
            obj["protocol"] = rfButtons[i].protocol;
            obj["bitLength"] = rfButtons[i].bitLength;
            obj["name"] = rfButtons[i].name;
            obj["singleAction"] = rfButtons[i].singleAction;
            obj["singleTarget"] = rfButtons[i].singleTarget;
            obj["doubleAction"] = rfButtons[i].doubleAction;
            obj["doubleTarget"] = rfButtons[i].doubleTarget;
            obj["longAction"] = rfButtons[i].longAction;
            obj["longTarget"] = rfButtons[i].longTarget;
            obj["tripleAction"] = rfButtons[i].tripleAction;
            obj["tripleTarget"] = rfButtons[i].tripleTarget;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── RF_LEARN ───────────────────────────────────────────────────────
    if (strcmp(message, "RF_LEARN") == 0)
    {
        startRFLearning();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── RF_LEARNED ─────────────────────────────────────────────────────
    if (strcmp(message, "RF_LEARNED") == 0)
    {
        StaticJsonDocument<128> doc;
        doc["code"] = rfLearnedCode;
        doc["protocol"] = rfLearnedProtocol;
        doc["bitLength"] = rfLearnedBitLength;
        doc["ready"] = rfCodeReady;
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── RF_CANCEL ──────────────────────────────────────────────────────
    if (strcmp(message, "RF_CANCEL") == 0)
    {
        stopRFLearning();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── RF_SAVE:{JSON} ────────────────────────────────────────────────
    if (strncmp(message, "RF_SAVE:", 8) == 0)
    {
        DynamicJsonDocument doc(512);
        DeserializationError err = deserializeJson(doc, message + 8);
        if (err)
        {
            mqttRespond("{\"error\":\"json\"}");
            return;
        }
        if (rfButtonCount >= MAX_RF_BUTTONS)
        {
            mqttRespond("{\"error\":\"full\"}");
            return;
        }
        rfButtons[rfButtonCount].code = doc["code"] | rfLearnedCode;
        rfButtons[rfButtonCount].protocol = doc["protocol"] | rfLearnedProtocol;
        rfButtons[rfButtonCount].bitLength = doc["bitLength"] | rfLearnedBitLength;
        String name = doc["name"].as<String>();
        if (name.length() == 0)
            name = "Button";
        strncpy(rfButtons[rfButtonCount].name, name.c_str(), 15);
        rfButtons[rfButtonCount].name[15] = '\0';
        rfButtons[rfButtonCount].singleAction = doc["singleAction"];
        rfButtons[rfButtonCount].singleTarget = doc["singleTarget"];
        rfButtons[rfButtonCount].doubleAction = doc["doubleAction"];
        rfButtons[rfButtonCount].doubleTarget = doc["doubleTarget"];
        rfButtons[rfButtonCount].longAction = doc["longAction"];
        rfButtons[rfButtonCount].longTarget = doc["longTarget"];
        rfButtons[rfButtonCount].tripleAction = doc["tripleAction"];
        rfButtons[rfButtonCount].tripleTarget = doc["tripleTarget"];
        rfButtons[rfButtonCount].active = true;
        rfButtonCount++;
        saveAllSettingsAsync();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── RF_DEL:{code} ─────────────────────────────────────────────────
    if (strncmp(message, "RF_DEL:", 7) == 0)
    {
        unsigned long code = atol(message + 7);
        bool found = false;
        for (int i = 0; i < rfButtonCount; i++)
        {
            if (rfButtons[i].code == code)
            {
                for (int j = i; j < rfButtonCount - 1; j++)
                    rfButtons[j] = rfButtons[j + 1];
                rfButtonCount--;
                saveAllSettingsAsync();
                found = true;
                break;
            }
        }
        mqttRespond(found ? "{\"ok\":true}" : "{\"error\":\"not_found\"}");
        return;
    }

    // ── COMBO_LIST ─────────────────────────────────────────────────────
    if (strcmp(message, "COMBO_LIST") == 0)
    {
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < rfComboCount; i++)
        {
            if (!rfCombos[i].active)
                continue;
            JsonObject obj = arr.createNestedObject();
            obj["code1"] = rfCombos[i].code1;
            obj["code2"] = rfCombos[i].code2;
            obj["name"] = rfCombos[i].name;
            obj["actionType"] = rfCombos[i].actionType;
            obj["actionId"] = rfCombos[i].actionId;
        }
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── COMBO_SAVE:{JSON} ─────────────────────────────────────────────
    if (strncmp(message, "COMBO_SAVE:", 11) == 0)
    {
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, message + 11);
        if (err)
        {
            mqttRespond("{\"error\":\"json\"}");
            return;
        }
        if (rfComboCount >= MAX_RF_COMBOS)
        {
            mqttRespond("{\"error\":\"full\"}");
            return;
        }
        rfCombos[rfComboCount].code1 = doc["code1"];
        rfCombos[rfComboCount].code2 = doc["code2"];
        String name = doc["name"].as<String>();
        strncpy(rfCombos[rfComboCount].name, name.c_str(), 15);
        rfCombos[rfComboCount].name[15] = '\0';
        rfCombos[rfComboCount].actionType = doc["actionType"];
        rfCombos[rfComboCount].actionId = doc["actionId"];
        rfCombos[rfComboCount].active = true;
        rfComboCount++;
        saveAllSettingsAsync();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── COMBO_DEL:{code1,code2} ──────────────────────────────────────
    if (strncmp(message, "COMBO_DEL:", 10) == 0)
    {
        unsigned long c1 = 0, c2 = 0;
        sscanf(message + 10, "%lu,%lu", &c1, &c2);
        bool found = false;
        for (int i = 0; i < rfComboCount; i++)
        {
            if ((rfCombos[i].code1 == c1 && rfCombos[i].code2 == c2) ||
                (rfCombos[i].code1 == c2 && rfCombos[i].code2 == c1))
            {
                for (int j = i; j < rfComboCount - 1; j++)
                    rfCombos[j] = rfCombos[j + 1];
                rfComboCount--;
                saveAllSettingsAsync();
                found = true;
                break;
            }
        }
        mqttRespond(found ? "{\"ok\":true}" : "{\"error\":\"not_found\"}");
        return;
    }

    // ── PHONE_LIST ─────────────────────────────────────────────────────
    if (strcmp(message, "PHONE_LIST") == 0)
    {
        DynamicJsonDocument doc(512);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < allowedCount; i++)
            arr.add(allowedNumbers[i]);
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── PHONE_SAVE:{number} ───────────────────────────────────────────
    if (strncmp(message, "PHONE_SAVE:", 11) == 0)
    {
        const char *num = message + 11;
        if (strlen(num) < 6)
        {
            mqttRespond("{\"error\":\"invalid\"}");
            return;
        }
        if (allowedCount >= MAX_PHONES)
        {
            mqttRespond("{\"error\":\"full\"}");
            return;
        }
        allowedNumbers[allowedCount++] = String(num);
        saveAllSettingsAsync();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── PHONE_DEL:{index} ─────────────────────────────────────────────
    if (strncmp(message, "PHONE_DEL:", 10) == 0)
    {
        int idx = atoi(message + 10);
        if (idx < 0 || idx >= allowedCount)
        {
            mqttRespond("{\"error\":\"not_found\"}");
            return;
        }
        for (int j = idx; j < allowedCount - 1; j++)
            allowedNumbers[j] = allowedNumbers[j + 1];
        allowedCount--;
        saveAllSettingsAsync();
        mqttRespond("{\"ok\":true}");
        return;
    }

    // ── GSM_SOFT_RST ──────────────────────────────────────────────────
    if (strcmp(message, "GSM_SOFT_RST") == 0)
    {
        SIM800_ForceReinit(&sim800);
        mqttRespond("{\"ok\":true}");
        addLog(4, 1, "GSM soft reset via MQTT");
        return;
    }

    // ── GSM_HARD_RST ──────────────────────────────────────────────────
    if (strcmp(message, "GSM_HARD_RST") == 0)
    {
        hwResetInProgress = true;
        hwResetStage = 0;
        digitalWrite(SIM800_RST_PIN, LOW);
        if (taskGSM)
            xTaskNotify(taskGSM, 0x04, eSetBits);
        mqttRespond("{\"ok\":true}");
        addLog(4, 1, "GSM hard reset via MQTT");
        return;
    }

    // ── MQTT_STATUS ────────────────────────────────────────────────────
    if (strcmp(message, "MQTT_STATUS") == 0)
    {
        StaticJsonDocument<256> doc;
        doc["enabled"] = mqttEnabled;
        doc["connected"] = MQTT_IsConnected(&mqttClient);
        doc["state"] = (int)MQTT_GetState(&mqttClient);
        doc["broker"] = mqttBroker;
        doc["port"] = mqttPort;
        doc["clientId"] = mqttClientId;
        doc["apn"] = mqttAPN;
        String json;
        serializeJson(doc, json);
        mqttRespond(json.c_str());
        return;
    }

    // ── Unknown command ────────────────────────────────────────────────
    Serial.printf("[MQTT] Unknown cmd: %s\n", message);
    mqttRespond("{\"error\":\"unknown_cmd\"}");

    char logMsg[128];
    snprintf(logMsg, sizeof(logMsg), "MQTT: %.30s", message);
    addLog(5, 0, logMsg);
}

void mqttConnectCallback(bool connected)
{
    if (connected)
    {
        Serial.println("[MQTT] Connected to broker");
        addLog(5, 0, "MQTT connected");
        // ✅ Subscribe ها از حلقه MQTT task اجرا میشن نه از callback
        // چون callback همه رو یکجا صدا میزنه و SIM800 فقط یکی رو همزمان میفرسته
        if (taskMQTT)
            xTaskNotify(taskMQTT, 0x02, eSetBits); // signal: do subscribes
    }
    else
    {
        Serial.println("[MQTT] Disconnected");
        addLog(5, 1, "MQTT disconnected");
    }
}

void taskMQTTFn(void *param)
{
    Serial.println("[MQTT Task] Started");
    esp_task_wdt_add(NULL);

    MQTT_SetMessageCallback(&mqttClient, mqttMessageCallback);
    MQTT_SetConnectCallback(&mqttClient, mqttConnectCallback);

    MQTT_SetAPN(&mqttClient, "mcinet", "", "");
    MQTT_SetBroker(&mqttClient, mqttBroker, mqttPort);

    if (strlen(mqttUser) > 0)
    {
        MQTT_SetAuth(&mqttClient, mqttClientId, mqttUser, mqttPass);
    }
    else
    {
        MQTT_SetAuth(&mqttClient, mqttClientId, "", "");
    }

    Serial.printf("[MQTT] Broker: %s:%d\n", mqttBroker, mqttPort);
    Serial.printf("[MQTT] Client ID: %s\n", mqttClientId);

    TickType_t lastPublish = 0;
    const TickType_t publishInterval = pdMS_TO_TICKS(30000);
    uint32_t notifyVal = 0;

    while (true)
    {
        esp_task_wdt_reset();
        MQTT_Process(&mqttClient);

        if (MQTT_IsConnected(&mqttClient))
        {
            TickType_t now = xTaskGetTickCount();

            if (now - lastPublish >= publishInterval)
            {
                lastPublish = now;

                char status[512];
                uint8_t relayStates[4] = {0, 0, 0, 0};
                uint32_t relayRemaining[4] = {0, 0, 0, 0};
                if (xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(20)) == pdTRUE)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        relayStates[i] = relays[i].isActive;
                        if (relays[i].isActive && relays[i].duration > 0)
                        {
                            unsigned long elapsed = millis() - relays[i].startTime;
                            if (elapsed < relays[i].duration)
                                relayRemaining[i] = (relays[i].duration - elapsed) / 1000;
                        }
                    }
                    xSemaphoreGive(mutexRelay);
                }
                snprintf(status, sizeof(status),
                         "{\"relays\":["
                         "{\"id\":1,\"active\":%s,\"remaining\":%lu},"
                         "{\"id\":2,\"active\":%s,\"remaining\":%lu},"
                         "{\"id\":3,\"active\":%s,\"remaining\":%lu},"
                         "{\"id\":4,\"active\":%s,\"remaining\":%lu}"
                         "],\"rssi\":%d,\"heap\":%u,\"uptime\":%lu,\"network\":%s}",
                         relayStates[0] ? "true" : "false", relayRemaining[0],
                         relayStates[1] ? "true" : "false", relayRemaining[1],
                         relayStates[2] ? "true" : "false", relayRemaining[2],
                         relayStates[3] ? "true" : "false", relayRemaining[3],
                         SIM800_GetSignalStrength(&sim800),
                         ESP.getFreeHeap(),
                         millis() / 1000,
                         networkReady ? "true" : "false");

                if (MQTT_PublishString(&mqttClient, mqttTopicStatus, status, false))
                {
                    Serial.println("📤 [MQTT] Status published");
                }
            }
        }

        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, 0) == pdTRUE)
        {
            if (notifyVal & 0x01)
            {
                lastPublish = 0;
            }

            // ✅ Subscribe ها یکی یکی با تأخیر اجرا میشن
            if (notifyVal & 0x02)
            {
                MQTT_Subscribe(&mqttClient, mqttTopicCommand);
                Serial.printf("[MQTT] Subscribed: %s\n", mqttTopicCommand);
                vTaskDelay(pdMS_TO_TICKS(200));

                for (int i = 1; i <= 4; i++)
                {
                    char relayTopic[80];
                    snprintf(relayTopic, sizeof(relayTopic), "%s/%d", mqttTopicRelay, i);
                    MQTT_Subscribe(&mqttClient, relayTopic);
                    Serial.printf("[MQTT] Subscribed: %s\n", relayTopic);
                    vTaskDelay(pdMS_TO_TICKS(200));
                }

                MQTT_PublishString(&mqttClient, mqttTopicLog, "Device online", true);
                lastPublish = 0; // publish status فوری
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==================== Task: WebServer ====================
void taskWebServerFn(void *p)
{
    esp_task_wdt_add(NULL);
    for (;;)
    {
        esp_task_wdt_reset();
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ==================== Task: GSM ====================
// ==================== Task: GSM ====================
// ==================== Task: GSM ====================
// ==================== Task: GSM ====================
void taskGSMFn(void *p)
{
    esp_task_wdt_add(NULL);
    uint32_t notifyVal = 0;
    unsigned long lastHealthCheck = 0;

    bool mqttAutoConnectDone = false;
    unsigned long mqttConnectAttemptTime = 0;

    for (;;)
    {
        esp_task_wdt_reset();
        unsigned long now = millis();

        // 1. پردازش دیتای سریال
        if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            // ✅ SIM800_Process همیشه اجرا بشه تا دستورات GPRS/TCP جواب بگیرن
            // clock sync از طریق command queue انجام میشه نه خواندن مستقیم سریال
            SIM800_Process(&sim800);
            if (clockSyncState != CSYNC_IDLE)
                processClockSync();
            xSemaphoreGive(mutexSerial);
        }

        // 2. مدیریت اتصال MQTT
        if (mqttEnabled && !mqttAutoConnectDone && !hwResetInProgress &&
            SIM800_IsReady(&sim800) && SIM800_IsNetworkRegistered(&sim800))
        {
            if (mqttConnectAttemptTime == 0)
            {
                mqttConnectAttemptTime = now;
                Serial.println("[MQTT] SIM ready, will connect in 5s...");
            }
            else if ((now - mqttConnectAttemptTime) >= 5000)
            {
                mqttAutoConnectDone = true;
                Serial.println("[MQTT] Starting connection...");
                MQTT_Connect(&mqttClient);
                addLog(4, 0, "MQTT auto-connect started");
            }
        }

        if (mqttAutoConnectDone && !networkReady)
        {
            mqttAutoConnectDone = false;
            mqttConnectAttemptTime = 0;
            Serial.println("[MQTT] Network lost, will retry when available");
        }

        // 3. ارسال پیامک‌های خروجی (فقط وقتی شبکه وصل است)
        if (sim800Ready && networkReady && !hwResetInProgress)
        {
            SMSOutMessage sms;
            if (xQueueReceive(qSMSOut, &sms, 0) == pdTRUE)
            {
                if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(500)) == pdTRUE)
                {
                    SIM800_SendSMS(&sim800, sms.number, sms.text);
                    lastSIM800Activity = millis();
                    xSemaphoreGive(mutexSerial);
                }
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }
        else
        {
            // شبکه قطع است - صف ارسال را خالی کن
            SMSOutMessage discard;
            uint8_t drained = 0;
            while (xQueueReceive(qSMSOut, &discard, 0) == pdTRUE && ++drained < 10)
                ;
            if (drained > 0)
            {
                char msg[48];
                snprintf(msg, 48, "Dropped %d SMS (SIM down)", drained);
                addLog(3, 1, msg);
            }
        }

        // Process inbound SMS commands (works even during network init)
        {
            SMSInMessage incomingSMS;
            if (xQueueReceive(qSMSIn, &incomingSMS, 0) == pdTRUE)
            {
                Serial.printf("[SMS CMD] Processing msg from: %s\n", incomingSMS.sender);
                if (isNumberAllowed(incomingSMS.sender))
                {
                    processSMSCommand(incomingSMS.text, incomingSMS.sender);
                }
                else
                {
                    Serial.println("[SMS] Unauthorized number ignored");
                    addLog(3, 1, "Unauthorized SMS blocked");
                }
            }
        }

        // Clock sync after SIM init
        if (pendingTimeSync && networkReady && !hwResetInProgress)
        {
            if ((now - simInitDoneAt) >= 5000UL)
            {
                pendingTimeSync = false;
                if (clockSyncState == CSYNC_IDLE)
                {
                    if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        startClockSync();
                        xSemaphoreGive(mutexSerial);
                    }
                }
            }
        }

        // 6. Health Check
        if (!hwResetInProgress && !gsmBooting &&
            (now - lastHealthCheck) >= GSM_CHECK_INTERVAL)
        {
            lastHealthCheck = now;

            if (sim800Ready && sim800.initialized && (now - simInitDoneAt) < 15000)
            {
                // Grace Period - 15 ثانیه بعد از init صبر کن
                networkReady = true;
                gsmFailureDetected = false;
                gsmFailureStart = 0;
            }
            else
            {
                int sig = SIM800_GetSignalStrength(&sim800);
                int state = SIM800_GetInitState(&sim800);
                bool reg = SIM800_IsNetworkRegistered(&sim800);

                sim800Ready = (state == INIT_COMPLETE);
                networkReady = sim800Ready && reg;

                bool commAlive = (lastSIM800Activity == 0) ||
                                 ((now - lastSIM800Activity) < SIM800_COMM_WATCHDOG) ||
                                 (MQTT_IsConnected(&mqttClient) &&
                                  (now - lastSIM800Activity) < SIM800_COMM_WATCHDOG * 3); // ✅ MQTT وصله ولی سریال هم باید تازه باشه

                bool healthy = networkReady && (sig > 0 && sig < 99) && commAlive;

                // ✅ اگه GPRS/TCP در حال اتصاله، health check رو متوقف کن
                // دستورات AT توی صف هستن ولی جواب هنوز نیومده
                if (sim800.gprs_state != GPRS_IDLE && sim800.gprs_state != GPRS_CONNECTED)
                    healthy = true;
                if (sim800.tcp_state != TCP_IDLE && sim800.tcp_state != TCP_CONNECTED)
                    healthy = true;

                if (!healthy)
                {
                    if (!gsmFailureDetected)
                    {
                        gsmFailureDetected = true;
                        gsmFailureStart = now;
                        softResetDone = false;
                        char msg[64];
                        snprintf(msg, 64, "GSM fail: rdy=%d net=%d sig=%d comm=%d",
                                 sim800Ready, networkReady, sig, (int)commAlive);
                        addLog(4, 1, msg);
                    }
                    else
                    {
                        unsigned long dur = now - gsmFailureStart;

                        if (dur >= GSM_HARD_RESET_TIMEOUT && !hwResetInProgress)
                        {
                            addLog(4, 2, "GSM hard reset triggered");
                            hwResetInProgress = true;
                            hwResetStage = 0;
                            gsmFailureDetected = false;
                            softResetDone = false;
                            hardwareResetCount++;

                            if (MQTT_IsConnected(&mqttClient))
                                MQTT_Disconnect(&mqttClient);
                            mqttClient.state = MQTT_IDLE; // ✅ ریست state بعد از قطع

                            mqttAutoConnectDone = false;
                            mqttConnectAttemptTime = 0;

                            digitalWrite(SIM800_RST_PIN, LOW);
                            xTaskNotify(taskGSM, 0x04, eSetBits);
                        }
                        else if (dur >= GSM_SOFT_RESET_TIMEOUT && !softResetDone)
                        {
                            addLog(4, 1, "GSM soft reset");
                            if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(200)) == pdTRUE)
                            {
                                SIM800_ForceReinit(&sim800);
                                xSemaphoreGive(mutexSerial);
                            }
                            mqttClient.state = MQTT_IDLE; // ✅ ریست state MQTT
                            sim800Ready = false;
                            networkReady = false;
                            softResetDone = true;
                            softwareResetCount++;
                            lastSIM800Activity = millis();
                            mqttAutoConnectDone = false;
                            mqttConnectAttemptTime = 0;
                        }
                    }
                }
                else
                {
                    if (gsmFailureDetected)
                    {
                        gsmFailureDetected = false;
                        gsmFailureStart = 0;
                        softResetDone = false;
                        addLog(4, 0, "GSM recovered");
                    }
                }
            }

            if (timerLED)
                xTimerChangePeriod(timerLED, pdMS_TO_TICKS(sim800Ready ? 1000 : 200), 0);
        }

        // 7. پایان زمان بوت
        if (gsmBooting && (now - gsmBootStarted) >= SIM800_BOOT_GRACE)
        {
            gsmBooting = false;
            lastHealthCheck = now;
            lastSIM800Activity = now;
            addLog(4, 0, "SIM800 boot grace done, monitoring...");
        }

        // 8. دریافت سیگنال‌های تسک
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, 0) == pdTRUE)
        {
            if (notifyVal & 0x02)
            {
                if (clockSyncState == CSYNC_IDLE && networkReady && !hwResetInProgress)
                {
                    if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        startClockSync();
                        xSemaphoreGive(mutexSerial);
                    }
                }
            }

            if (notifyVal & 0x04)
            {
                addLog(4, 1, "SIM800 RST LOW, holding 300ms...");
                vTaskDelay(pdMS_TO_TICKS(300));

                digitalWrite(SIM800_RST_PIN, HIGH);
                addLog(4, 1, "SIM800 RST HIGH, waiting boot...");
                vTaskDelay(pdMS_TO_TICKS(5000));

                if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(500)) == pdTRUE)
                {
                    SIM800_ForceReinit(&sim800);
                    xSemaphoreGive(mutexSerial);
                }

                sim800Ready = false;
                networkReady = false;
                hwResetInProgress = false;
                hwResetStage = 0;
                mqttClient.state = MQTT_IDLE; // ✅ ریست state MQTT
                internalClock.isValid = false;
                pendingTimeSync = true;
                simInitDoneAt = millis();
                gsmBooting = true;
                gsmBootStarted = millis();
                mqttAutoConnectDone = false;
                mqttConnectAttemptTime = 0;

                addLog(4, 1, "SIM800 hard reset complete, grace period started");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// ==================== Task: RF ====================
void taskRFFn(void *p)
{
    esp_task_wdt_add(NULL);
    for (;;)
    {
        esp_task_wdt_reset();

        if (rcSwitch.available())
        {
            noInterrupts();
            unsigned long code = rcSwitch.getReceivedValue();
            uint8_t proto = (uint8_t)rcSwitch.getReceivedProtocol();
            uint16_t bits = rcSwitch.getReceivedBitlength();
            rcSwitch.resetAvailable();
            interrupts();

            if (code != 0)
            {
                if (rfLearningMode)
                {
                    rfLearnedCode = code;
                    rfLearnedProtocol = proto;
                    rfLearnedBitLength = bits;
                    rfCodeReady = true;
                    rfLearningMode = false;
                    lastRFCode = 0;
                    lastRFTime = 0;
                    rfClickCount = 0;
                    longPressDetected = false;
                    Serial.printf("[RF] LEARNED BUTTON: 0x%08lX\n", code);
                    goto rf_done;
                }

                if (s_learnWaiting)
                {
                    s_learnedCode = code;
                    s_learnedProtocol = proto;
                    s_learnedBits = bits;
                    s_learnWaiting = false;
                    Serial.printf("[RF] LEARNED SENSOR: 0x%08lX\n", code);
                    goto rf_done;
                }

                if (processSensorCode((uint32_t)code, proto, bits))
                    goto rf_done;

                handleRFCode(code, proto, bits);

            rf_done:;
            }
        }

        if (!rfLearningMode)
        {
            processRFClickDetection();
        }

        if (rfLearningMode &&
            (millis() - rfLearningStart) >= RF_LEARNING_TIMEOUT)
        {
            rfLearningMode = false;
            rfCodeReady = false;
            lastRFCode = 0;
            rfClickCount = 0;
            longPressDetected = false;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ==================== Task: Relay ====================
void taskRelayFn(void *p)
{
    esp_task_wdt_add(NULL);
    for (;;)
    {
        esp_task_wdt_reset();

        RelayCommand cmd;
        while (xQueueReceive(qRelay, &cmd, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            if (xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                switch (cmd.mode)
                {
                case 0:
                    _deactivateRelay(cmd.index);
                    break;
                case 1:
                    _activateRelay(cmd.index, cmd.duration);
                    break;
                case 2:
                    _toggleRelay(cmd.index);
                    break;
                }
                xSemaphoreGive(mutexRelay);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ==================== Task: Scene ====================
void taskSceneFn(void *p)
{
    esp_task_wdt_add(NULL);
    TickType_t lastSchedule = xTaskGetTickCount();
    TickType_t lastAutomation = xTaskGetTickCount();

    for (;;)
    {
        esp_task_wdt_reset();
        TickType_t now = xTaskGetTickCount();

        processEventQueue();
        processSequentialScene();
        processPulseStates();

        if ((now - lastSchedule) >= pdMS_TO_TICKS(1000))
        {
            lastSchedule = now;
            if (internalClock.isValid)
                checkScheduledScenes();
            updateInternalClock();
        }

        if ((now - lastAutomation) >= pdMS_TO_TICKS(2000))
        {
            lastAutomation = now;
            if (automationCount > 0)
                checkAutomations();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void taskSaveFn(void *p)
{
    esp_task_wdt_add(NULL);
    SaveCommand cmd;

    Serial.println("[Save Task] Storage worker initialized");

    for (;;)
    {
        esp_task_wdt_reset();

        if (xQueueReceive(qSave, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            Serial.printf("[Save] Type=%d execution start\n", cmd.type);

            switch (cmd.type)
            {
            case 0:
                saveAllSettings();
                break;
            case 1:
                saveRelayStates();
                break;
            case 2:
                saveSensors();
                break;
            case 3:
                saveAutomations();
                break;
            case 4:
                saveMQTTSettings();
                break;
            default:
                Serial.printf("[Save] Unknown command type: %d\n", cmd.type);
                break;
            }

            Serial.printf("[Save] Type=%d operation successful\n", cmd.type);

            // ریست مجدد واچ‌داگ بلافاصله پس از اتمام عملیات سنگین فلش
            esp_task_wdt_reset();

            // یک تنفس کوتاه به سیستم برای جابجایی تسک‌ها در صورت وجود دستورات متوالی در صف
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        // وقتی صف خالی است، تابع xQueueReceive خودش ۱ ثانیه تسک را بلاک کرده و CPU را آزاد نگه می‌دارد.
    }
}