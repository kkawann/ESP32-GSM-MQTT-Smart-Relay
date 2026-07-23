#include "webapi.h"
#include "globals.h"
#include "html_page.h"
#include "relay.h"
#include "rf.h"
#include "scene.h"
#include "gsm.h"
#include "storage.h"
#include "sensor.h"
#include "automation.h"
#include "clock.h"
#include <ArduinoJson.h>
#include "ota.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "SIM800_MQTT.h"

void handleAPIStatus()
{
    StaticJsonDocument<768> doc;
    doc["wifi"] = wifiReady;
    doc["sim800"] = sim800Ready;
    doc["network"] = networkReady;
    doc["signal"] = SIM800_GetSignalStrength(&sim800);
    doc["rfCount"] = rfButtonCount;
    doc["comboCount"] = rfComboCount;
    doc["sceneCount"] = sceneCount;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["phoneCount"] = allowedCount;

    char timeBuf[20] = "--:--:--";
    char dateBuf[12] = "----/--/--";
    bool clockValid = false;

    if (mutexClock &&
        xSemaphoreTake(mutexClock, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        clockValid = internalClock.isValid;
        if (clockValid)
        {
            snprintf(timeBuf, sizeof(timeBuf),
                     "%02d:%02d:%02d",
                     internalClock.hour,
                     internalClock.minute,
                     internalClock.second);
            snprintf(dateBuf, sizeof(dateBuf),
                     "%04d/%02d/%02d",
                     internalClock.year,
                     internalClock.month,
                     internalClock.day);
        }
        xSemaphoreGive(mutexClock);
    }

    doc["time"] = timeBuf;
    doc["date"] = dateBuf;
    doc["clockValid"] = clockValid;

    // ── relay array ──────────────────────────────────────────────────
    JsonArray relayArr = doc.createNestedArray("relays");
    for (int i = 0; i < 4; i++)
    {
        JsonObject r = relayArr.createNestedObject();
        r["id"] = i + 1;
        r["active"] = relays[i].isActive;
        if (relays[i].isActive && relays[i].duration > 0)
        {
            unsigned long elapsed = millis() - relays[i].startTime;
            r["remaining"] = (elapsed < relays[i].duration)
                                 ? (relays[i].duration - elapsed) / 1000
                                 : 0;
        }
    }

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPIRelay()
{
    if (!requireAuth())
        return;

    // اطمینان از وجود بدنه درخواست
    if (!server.hasArg("plain"))
    {
        server.send(400, "text/plain", "Bad Request: No Body");
        return;
    }

    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));

    if (err)
    {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    int index = doc["index"];
    String mode = doc["mode"].as<String>();
    uint32_t dur = doc["duration"] | 0;

    if (mode == "on")
        activateRelayTS(index, dur);
    else if (mode == "off")
        deactivateRelayTS(index);
    else if (mode == "toggle")
        toggleRelayTS(index);

    server.send(200, "text/plain", "OK");
}

void handleAPIRFLearn()
{
    if (!requireAuth())
        return;
    startRFLearning();
    server.send(200, "text/plain", "OK");
}

void handleAPIRFLearned()
{
    StaticJsonDocument<128> doc;
    doc["code"] = rfLearnedCode;
    doc["protocol"] = rfLearnedProtocol;
    doc["bitLength"] = rfLearnedBitLength;
    doc["ready"] = rfCodeReady;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPIRFCancel()
{
    stopRFLearning();
    server.send(200, "text/plain", "OK");
}

void handleAPIRFSave()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<512> doc;
    deserializeJson(doc, server.arg("plain"));
    if (rfButtonCount >= MAX_RF_BUTTONS)
    {
        server.send(507, "text/plain", "Full");
        return;
    }
    // از JSON بخون (frontend می‌فرسته)، fallback به global در صورت نبود
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
    server.send(200, "text/plain", "OK");
}

void handleAPIRFButtons()
{
    DynamicJsonDocument doc(3072);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < rfButtonCount; i++)
    {
        if (!rfButtons[i].active)
            continue;
        JsonObject obj = arr.createNestedObject();
        obj["code"] = rfButtons[i].code;
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
    server.send(200, "application/json", json);
}

void handleAPIRFDelete()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    unsigned long code = doc["code"];
    for (int i = 0; i < rfButtonCount; i++)
    {
        if (rfButtons[i].code == code)
        {
            for (int j = i; j < rfButtonCount - 1; j++)
            {
                rfButtons[j] = rfButtons[j + 1];
            }
            rfButtonCount--;
            saveAllSettingsAsync();
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(404, "text/plain", "Not found");
}

void handleAPICombos()
{
    StaticJsonDocument<1024> doc;
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
    server.send(200, "application/json", json);
}

void handleAPIComboSave()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));
    if (rfComboCount >= MAX_RF_COMBOS)
    {
        server.send(507, "text/plain", "Full");
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
    server.send(200, "text/plain", "OK");
}

void handleAPIComboDelete()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    unsigned long code1 = doc["code1"];
    unsigned long code2 = doc["code2"];
    for (int i = 0; i < rfComboCount; i++)
    {
        if ((rfCombos[i].code1 == code1 && rfCombos[i].code2 == code2) ||
            (rfCombos[i].code1 == code2 && rfCombos[i].code2 == code1))
        {
            for (int j = i; j < rfComboCount - 1; j++)
            {
                rfCombos[j] = rfCombos[j + 1];
            }
            rfComboCount--;
            saveAllSettingsAsync();
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(404, "text/plain", "Not found");
}

void handleAPIScenes()
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
    server.send(200, "application/json", json);
}

void handleAPISceneSave()
{
    if (!requireAuth())
        return;
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, server.arg("plain"));
    if (sceneCount >= MAX_SCENES)
    {
        server.send(507, "text/plain", "Full");
        return;
    }
    scenes[sceneCount].id = sceneCount;
    String name = doc["name"].as<String>();
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
    server.send(200, "text/plain", "OK");
}

void handleAPISceneRun()
{
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"];
    executeScene(id);
    server.send(200, "text/plain", "OK");
}

void handleAPISceneDelete()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"];
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
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(404, "text/plain", "Not found");
}

void handleAPIRelaySettings()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<256> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < 4; i++)
    {
        JsonObject r = arr.createNestedObject();
        r["index"] = i;
        r["logic"] = relays[i].logic;
        r["notifySMS"] = relays[i].notifySMS;
    }
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPIRelaySettingsSave()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    int index = doc["index"] | -1;
    if (index < 0 || index > 3)
    {
        server.send(400, "text/plain", "Invalid index");
        return;
    }
    relays[index].logic = doc["logic"] | false;
    relays[index].notifySMS = doc["notifySMS"] | false;
    saveAllSettingsAsync();
    server.send(200, "text/plain", "OK");
}

void handleAPIResetSoft()
{
    if (!requireAuth())
        return;
    SIM800_ForceReinit(&sim800);
    server.send(200, "text/plain", "OK");
}

void handleAPIResetHard()
{
    if (!requireAuth())
        return;
    hwResetInProgress = true;
    hwResetStage = 0;
    digitalWrite(SIM800_RST_PIN, LOW);
    if (taskGSM)
        xTaskNotify(taskGSM, 0x04, eSetBits);
    server.send(200, "text/plain", "OK");
}

void handleAPIClear()
{
    if (!requireAuth())
        return;
    prefs.begin("relay", false);
    prefs.clear();
    prefs.end();
    server.send(200, "text/plain", "OK");
}

void handleAPIPhones()
{
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < allowedCount; i++)
        arr.add(allowedNumbers[i]);
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPIPhoneSave()
{
    if (!requireAuth())
        return;

    if (!server.hasArg("plain"))
    {
        server.send(400, "text/plain", "No body");
        return;
    }

    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));

    if (err)
    {
        Serial.printf("[API] JSON parse error: %s\n", err.c_str());
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    if (allowedCount >= MAX_PHONES)
    {
        server.send(507, "text/plain", "Full");
        return;
    }

    String num = doc["phone"].as<String>();
    if (num.length() < 6 || num == "null" || num == "NULL" || num == "")
    {
        Serial.printf("[API] Invalid phone number: '%s'\n", num.c_str());
        server.send(400, "text/plain", "Invalid phone number");
        return;
    }

    allowedNumbers[allowedCount++] = num;
    Serial.printf("[API] Added phone[%d]: %s\n", allowedCount - 1, num.c_str());

    saveAllSettingsAsync();
    server.send(200, "text/plain", "OK");
}

void handleAPIPhoneDelete()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    int idx = doc["index"];
    for (int j = idx; j < allowedCount - 1; j++)
        allowedNumbers[j] = allowedNumbers[j + 1];
    allowedCount--;
    saveAllSettingsAsync();
    server.send(200, "text/plain", "OK");
}

void handleAPILogs()
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
    server.send(200, "application/json", json);
}

void handleAPILogsClear()
{
    if (!requireAuth())
        return;
    logIndex = 0;
    logCount = 0;
    for (int i = 0; i < MAX_EVENT_LOGS; i++)
    {
        eventLogs[i].active = false;
    }
    server.send(200, "text/plain", "OK");
}
// ==================== Sensor API ====================

// ==================== API Sensors ====================

void handleAPISensors()
{
    DynamicJsonDocument doc(4096);
    JsonArray arr = doc.to<JsonArray>();
    const char *units[] = {"%", "°C", "%RH", "cm", "V", ""};
    const char *typeNames[] = {"percent", "temp", "humidity", "distance", "voltage", "raw"};

    for (int i = 0; i < rfSensorCount; i++)
    {
        if (!rfSensors[i].active)
            continue;
        int vt = min((int)rfSensors[i].valueType, 5);
        JsonObject o = arr.createNestedObject();
        o["id"] = i;
        o["name"] = rfSensors[i].name;
        o["valueType"] = rfSensors[i].valueType;
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
    server.send(200, "application/json", json);
}

void handleAPISensorSave()
{
    if (!requireAuth())
        return;
    DynamicJsonDocument doc(512);
    deserializeJson(doc, server.arg("plain"));

    int id = doc["id"] | -1;
    bool isNew = (id == -1);

    if (isNew)
    {
        if (rfSensorCount >= MAX_RF_SENSORS)
        {
            server.send(507, "text/plain", "Full");
            return;
        }
        id = rfSensorCount;
        memset(&rfSensors[id], 0, sizeof(RFSensor));
        memset(&sensorHistory[id], 0, sizeof(SensorRingBuf));
    }

    if (id < 0 || id >= MAX_RF_SENSORS)
    {
        server.send(400, "text/plain", "Invalid id");
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

    rfSensors[id].active = true;

    if (isNew)
        rfSensorCount++;

    saveSensorsAsync();

    char resp[24];
    snprintf(resp, sizeof(resp), "{\"id\":%d}", id);
    server.send(200, "application/json", resp);
}

void handleAPISensorDelete()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"] | -1;

    if (id < 0 || id >= rfSensorCount)
    {
        server.send(404, "text/plain", "Not found");
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
    server.send(200, "text/plain", "OK");
}

void handleAPISensorLearnStart()
{
    if (!requireAuth())
        return;
    s_learnedCode = 0;
    s_learnWaiting = true;
    s_learnStartMs = millis();
    server.send(200, "text/plain", "OK");
}

void handleAPISensorLearnStatus()
{
    // بررسی timeout
    if (s_learnWaiting &&
        (millis() - s_learnStartMs) > 30000UL)
    {
        s_learnWaiting = false;
        s_learnedCode = 0;
    }

    // بررسی پایان یادگیری (تسک RF متغیر s_learnedCode را پر کرده است)
    bool done = (s_learnedCode != 0 && !s_learnWaiting);

    StaticJsonDocument<256> doc;
    doc["active"] = s_learnWaiting;
    doc["done"] = done;
    doc["timeout"] = false;
    doc["phase"] = done ? 2 : 0;
    doc["rawCode"] = s_learnedCode;
    doc["protocol"] = s_learnedProtocol;
    doc["bitLength"] = s_learnedBits;

    if (done)
    {
        uint32_t fullMask = (s_learnedBits >= 32) ? 0xFFFFFFFFUL : ((1UL << s_learnedBits) - 1);

        // پیش‌فرض برای DHT شما: 8 بیت پایین = مقدار
        uint32_t valMask = 0xFF & fullMask;
        uint32_t baseMask = (~valMask) & fullMask;

        doc["baseCode"] = s_learnedCode & baseMask;
        doc["baseMask"] = baseMask;
    }

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPISensorLearnCancel()
{
    s_learnWaiting = false;
    s_learnedCode = 0;
    server.send(200, "text/plain", "OK");
}

void handleAPISensorValues()
{
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();
    const char *units[] = {"%", "°C", "%RH", "cm", "V", ""};

    for (int i = 0; i < rfSensorCount; i++)
    {
        if (!rfSensors[i].active)
            continue;
        JsonObject o = arr.createNestedObject();
        o["id"] = i;
        o["name"] = rfSensors[i].name;
        o["unit"] = units[min((int)rfSensors[i].valueType, 5)];
        o["hasValue"] = rfSensors[i].hasValue;
        o["value"] = rfSensors[i].hasValue ? rfSensors[i].lastValue : 0.0f;
        o["ageS"] = rfSensors[i].hasValue ? ((uint32_t)millis() - rfSensors[i].lastUpdateMs) / 1000UL : (uint32_t)9999;
        o["rxCount"] = rfSensors[i].rxCount;
    }
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPIAutomations()
{
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();

    const char *condNames[] = {
        "همیشه", "رله روشن", "رله خاموش", "بین ساعت",
        "سنسور >", "سنسور <", "سنسور بین", "سنسور خارج",
        "سنسور آفلاین", "روز هفته"};
    const char *actNames[] = {
        "—", "روشن", "خاموش", "Toggle", "روشن زماندار",
        "همه خاموش", "سناریو", "SMS", "Pulse"};

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

        // شرط‌ها
        JsonArray conds = obj.createNestedArray("conditions");
        for (int j = 0; j < a.conditionCount; j++)
        {
            JsonObject c = conds.createNestedObject();
            c["type"] = a.conditions[j].type;
            c["typeName"] = condNames[min((int)a.conditions[j].type, 9)];
            c["relayId"] = a.conditions[j].relayId;
            c["sensorId"] = a.conditions[j].sensorId;
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

        // اعمال
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

        // هیسترزیس
        JsonObject hyst = obj.createNestedObject("hysteresis");
        hyst["enabled"] = a.hysteresis.enabled;
        hyst["onThreshold"] = a.hysteresis.onThreshold;
        hyst["offThreshold"] = a.hysteresis.offThreshold;
        hyst["sensorId"] = a.hysteresis.sensorId;
        hyst["relayId"] = a.hysteresis.relayId;
        hyst["state"] = a.hysteresis.state;

        // وضعیت runtime
        obj["lastTriggerMs"] = (uint32_t)a.lastTriggerMs;
    }

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleAPIAutomationSave()
{
    if (!requireAuth())
        return;

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err)
    {
        server.send(400, "text/plain", "JSON error");
        return;
    }

    int id = doc["id"] | -1;
    bool isNew = (id == -1);
    if (!isNew && (id < 0 || id >= automationCount))
    {
        server.send(404, "text/plain", "Not found");
        return;
    }
    if (isNew)
    {
        if (automationCount >= MAX_AUTOMATIONS)
        {
            server.send(507, "text/plain", "Full");
            return;
        }
        id = automationCount;
        memset(&automations[id], 0, sizeof(Automation));
    }
    if (id < 0 || id >= MAX_AUTOMATIONS)
    {
        server.send(400, "text/plain", "Invalid id");
        return;
    }

    Automation &a = automations[id];
    a.id = id;
    a.active = doc["active"] | true;
    a.logicOp = doc["logicOp"] | (uint8_t)LOGIC_AND;

    String nm = doc["name"] | String("Automation");
    strncpy(a.name, nm.c_str(), 31);
    a.name[31] = '\0';

    // Cooldown
    a.cooldownEnabled = doc["cooldownEnabled"] | false;
    a.cooldownMinutes = doc["cooldownMinutes"] | (uint16_t)0;

    // ── شرط‌ها ────────────────────────────────────────────────────
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
            a.conditionCount++;
        }
    }

    // ── اعمال ─────────────────────────────────────────────────────
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

    // ── هیسترزیس ──────────────────────────────────────────────────
    if (doc.containsKey("hysteresis"))
    {
        JsonObject h = doc["hysteresis"];
        a.hysteresis.enabled = h["enabled"] | false;
        a.hysteresis.onThreshold = h["onThreshold"] | 0.0f;
        a.hysteresis.offThreshold = h["offThreshold"] | 0.0f;
        a.hysteresis.sensorId = h["sensorId"] | (uint8_t)0;
        a.hysteresis.relayId = h["relayId"] | (uint8_t)0;
        a.hysteresis.state = false; // runtime reset
    }

    // ریست runtime
    a.lastTriggerMs = 0;
    a.lastEval = false;
    a.triggered = false;

    if (isNew)
        automationCount++;

    saveAutomationsAsync();

    char resp[32];
    snprintf(resp, sizeof(resp), "{\"id\":%d}", id);
    server.send(200, "application/json", resp);
}

void handleAPIAutomationDelete()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"] | -1;

    if (id < 0 || id >= automationCount)
    {
        server.send(404, "text/plain", "Not found");
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
    server.send(200, "text/plain", "OK");
}

void handleAPIAutomationToggle()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"] | -1;

    if (id < 0 || id >= automationCount)
    {
        server.send(404, "text/plain", "Not found");
        return;
    }
    automations[id].active = !automations[id].active;
    automations[id].lastEval = false; // edge detection reset

    saveAutomationsAsync();

    char resp[40];
    snprintf(resp, sizeof(resp),
             "{\"id\":%d,\"active\":%s}",
             id, automations[id].active ? "true" : "false");
    server.send(200, "application/json", resp);
}

void handleAPIAutomationTest()
{
    if (!requireAuth())
        return;
    StaticJsonDocument<64> doc;
    deserializeJson(doc, server.arg("plain"));
    int id = doc["id"] | -1;

    if (id < 0 || id >= automationCount)
    {
        server.send(404, "text/plain", "Not found");
        return;
    }

    // اجرای force (بدون edge detection)
    bool eval = evalAutomationNoTime(automations[id]);
    for (int j = 0; j < automations[id].actionCount; j++)
    {
        executeAutoAction(automations[id].actions[j]);
    }

    char resp[80];
    snprintf(resp, sizeof(resp),
             "{\"id\":%d,\"eval\":%s,\"actionsRun\":%d}",
             id, eval ? "true" : "false",
             automations[id].actionCount);
    server.send(200, "application/json", resp);
}
bool requireAuth()
{
    if (server.authenticate(ADMIN_USER, ADMIN_PASS))
        return true;
    server.requestAuthentication();
    return false;
}
void handleRoot()
{
    if (!requireAuth())
        return;
    server.send_P(200, "text/html", HTML);
}
// در webapi.cpp:

// POST /api/wifi/connect
void handleAPIWifiConnect()
{
    if (!server.hasArg("plain"))
    {
        server.send(400);
        return;
    }

    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));

    String ssid = doc["ssid"] | "";
    String pass = doc["password"] | "";

    if (ssid.isEmpty())
    {
        server.send(400, "application/json", "{\"error\":\"no ssid\"}");
        return;
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    server.send(200, "application/json", "{\"ok\":true}");
}

// GET /api/wifi/status
void handleAPIWifiStatus()
{
    StaticJsonDocument<128> doc;

    wl_status_t st = WiFi.status();
    doc["connected"] = (st == WL_CONNECTED);
    doc["failed"] = (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL);
    if (st == WL_CONNECTED)
        doc["ip"] = WiFi.localIP().toString();

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// POST /api/ota/check  →  فقط task رو راه می‌اندازه
void handleAPIOTACheck()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        server.send(200, "application/json",
                    "{\"ok\":false,\"reason\":\"not connected\"}");
        return;
    }
    otaBegin();
    server.send(200, "application/json", "{\"ok\":true}");
}

// GET /api/ota/status  →  وضعیت فعلی OTA
void handleAPIOTAStatus()
{
    StaticJsonDocument<256> doc;

    const char *statusStr[] = {
        "idle", "checking", "downloading", "success", "up_to_date", "failed"};

    doc["status"] = statusStr[(int)otaState.status];
    doc["progress"] = otaState.progress;
    doc["message"] = otaState.message;
    doc["newVersion"] = otaState.newVersion;
    doc["current"] = CURRENT_VERSION;

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}
void handleAPIMQTTStatus()
{
    String json = "{";
    json += "\"enabled\":" + String(mqttEnabled ? "true" : "false") + ",";
    json += "\"connected\":" + String(MQTT_IsConnected(&mqttClient) ? "true" : "false") + ",";
    json += "\"state\":" + String((int)MQTT_GetState(&mqttClient)) + ",";
    json += "\"broker\":\"" + String(mqttBroker) + "\",";
    json += "\"port\":" + String(mqttPort) + ",";
    json += "\"clientId\":\"" + String(mqttClientId) + "\",";
    json += "\"user\":\"" + String(mqttUser) + "\",";
    json += "\"topicStatus\":\"" + String(mqttTopicStatus) + "\",";
    json += "\"topicLog\":\"" + String(mqttTopicLog) + "\",";
    json += "\"topicCmd\":\"" + String(mqttTopicCmd) + "\",";
    json += "\"apn\":\"" + String(mqttAPN) + "\"";
    json += "}";

    server.send(200, "application/json", json);
}

void handleAPIMQTTSettings()
{
    if (!server.hasArg("enabled"))
    {
        server.send(400, "text/plain", "Missing enabled");
        return;
    }

    mqttEnabled = (server.arg("enabled") == "1" || server.arg("enabled") == "true");

    if (server.hasArg("broker"))
    {
        strncpy(mqttBroker, server.arg("broker").c_str(), sizeof(mqttBroker) - 1);
        mqttBroker[sizeof(mqttBroker) - 1] = '\0';
    }

    if (server.hasArg("port"))
        mqttPort = server.arg("port").toInt();

    if (server.hasArg("clientId"))
    {
        strncpy(mqttClientId, server.arg("clientId").c_str(), sizeof(mqttClientId) - 1);
        mqttClientId[sizeof(mqttClientId) - 1] = '\0';
    }

    if (server.hasArg("user"))
    {
        strncpy(mqttUser, server.arg("user").c_str(), sizeof(mqttUser) - 1);
        mqttUser[sizeof(mqttUser) - 1] = '\0';
    }

    if (server.hasArg("pass"))
    {
        strncpy(mqttPass, server.arg("pass").c_str(), sizeof(mqttPass) - 1);
        mqttPass[sizeof(mqttPass) - 1] = '\0';
    }

    if (server.hasArg("topicStatus"))
    {
        strncpy(mqttTopicStatus, server.arg("topicStatus").c_str(), sizeof(mqttTopicStatus) - 1);
        mqttTopicStatus[sizeof(mqttTopicStatus) - 1] = '\0';
    }

    if (server.hasArg("topicLog"))
    {
        strncpy(mqttTopicLog, server.arg("topicLog").c_str(), sizeof(mqttTopicLog) - 1);
        mqttTopicLog[sizeof(mqttTopicLog) - 1] = '\0';
    }

    if (server.hasArg("topicCmd"))
    {
        strncpy(mqttTopicCmd, server.arg("topicCmd").c_str(), sizeof(mqttTopicCmd) - 1);
        mqttTopicCmd[sizeof(mqttTopicCmd) - 1] = '\0';
    }

    if (server.hasArg("apn"))
    {
        strncpy(mqttAPN, server.arg("apn").c_str(), sizeof(mqttAPN) - 1);
        mqttAPN[sizeof(mqttAPN) - 1] = '\0';
    }

    if (server.hasArg("apnUser"))
    {
        strncpy(mqttAPNUser, server.arg("apnUser").c_str(), sizeof(mqttAPNUser) - 1);
        mqttAPNUser[sizeof(mqttAPNUser) - 1] = '\0';
    }

    if (server.hasArg("apnPass"))
    {
        strncpy(mqttAPNPass, server.arg("apnPass").c_str(), sizeof(mqttAPNPass) - 1);
        mqttAPNPass[sizeof(mqttAPNPass) - 1] = '\0';
    }

    // Save to EEPROM
    saveMQTTSettings();

    server.send(200, "text/plain", "OK - Settings saved");

    // Restart MQTT if enabled
    if (mqttEnabled)
    {
        Serial.println("[API] Reconnecting MQTT with new settings...");

        // Disconnect first
        MQTT_Disconnect(&mqttClient);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Update settings
        MQTT_SetBroker(&mqttClient, mqttBroker, mqttPort);
        MQTT_SetAuth(&mqttClient, mqttClientId, mqttUser, mqttPass);
        MQTT_SetAPN(&mqttClient, mqttAPN, mqttAPNUser, mqttAPNPass);

        // Reconnect
        MQTT_Connect(&mqttClient);
    }
}

void handleAPIMQTTConnect()
{
    if (!SIM800_IsReady(&sim800))
    {
        server.send(503, "text/plain", "SIM800 not ready");
        return;
    }

    if (!mqttEnabled)
    {
        server.send(400, "text/plain", "MQTT not enabled");
        return;
    }

    Serial.println("[API] Manual MQTT connect requested");
    MQTT_Connect(&mqttClient);

    server.send(200, "text/plain", "Connecting...");
}

void handleAPIMQTTDisconnect()
{
    Serial.println("[API] MQTT disconnect requested");
    MQTT_Disconnect(&mqttClient);

    server.send(200, "text/plain", "Disconnected");
}