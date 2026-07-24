#pragma once
#include <Arduino.h>

#include "storage.h"
#include "globals.h"
#include "relay.h"
#include <Preferences.h>
#include "config.h"
#include "types.h"
#include <EEPROM.h>
#define MQTT_SETTINGS_ADDR 400
// All save/load functions
void saveRelayStatesAsync()
{
    SaveCommand cmd = {1};
    xQueueSend(qSave, &cmd, pdMS_TO_TICKS(50));
}
void loadRelayStates()
{
    if (!enableStatePersistence)
        return;
    prefs.begin("relay", true);
    for (int i = 0; i < 4; i++)
    {
        bool wasActive = prefs.getBool(("r" + String(i) + "act").c_str(), false);
        if (wasActive)
        {
            setRelay(i, true);
            relays[i].isActive = true;
        }
    }
    prefs.end();
}
// ══════════════════════════════════════════════════════════════════
// Save Automations Async
// ══════════════════════════════════════════════════════════════════
void saveAutomationsAsync()
{
    SaveCommand cmd = {3}; // New type
    xQueueSend(qSave, &cmd, pdMS_TO_TICKS(50));
}
void saveAllSettings()
{
    prefs.begin("relay", false);
    prefs.clear();

    for (int i = 0; i < 4; i++)
    {
        prefs.putBool(("logic" + String(i)).c_str(), relays[i].logic);
        prefs.putBool(("rnotify" + String(i)).c_str(), relays[i].notifySMS);
    }

    prefs.putInt("numCount", allowedCount);
    for (int i = 0; i < allowedCount; i++)
        prefs.putString(("num" + String(i)).c_str(), allowedNumbers[i]);

    prefs.putInt("rfCount", rfButtonCount);
    for (int i = 0; i < rfButtonCount; i++)
    {
        String p = "rf" + String(i) + "_";
        prefs.putULong((p + "code").c_str(), rfButtons[i].code);
        prefs.putUChar((p + "proto").c_str(), rfButtons[i].protocol);
        prefs.putUShort((p + "bits").c_str(), rfButtons[i].bitLength);
        prefs.putString((p + "name").c_str(), rfButtons[i].name);
        prefs.putUChar((p + "sa").c_str(), rfButtons[i].singleAction);
        prefs.putUChar((p + "st").c_str(), rfButtons[i].singleTarget);
        prefs.putUChar((p + "da").c_str(), rfButtons[i].doubleAction);
        prefs.putUChar((p + "dt").c_str(), rfButtons[i].doubleTarget);
        prefs.putUChar((p + "la").c_str(), rfButtons[i].longAction);
        prefs.putUChar((p + "lt").c_str(), rfButtons[i].longTarget);
        prefs.putUChar((p + "ta").c_str(), rfButtons[i].tripleAction);
        prefs.putUChar((p + "tt").c_str(), rfButtons[i].tripleTarget);
        prefs.putBool((p + "active").c_str(), rfButtons[i].active);
    }

    prefs.putInt("comboCount", rfComboCount);
    for (int i = 0; i < rfComboCount; i++)
    {
        String p = "cb" + String(i) + "_";
        prefs.putULong((p + "c1").c_str(), rfCombos[i].code1);
        prefs.putULong((p + "c2").c_str(), rfCombos[i].code2);
        prefs.putString((p + "name").c_str(), rfCombos[i].name);
        prefs.putUChar((p + "type").c_str(), rfCombos[i].actionType);
        prefs.putUChar((p + "id").c_str(), rfCombos[i].actionId);
        prefs.putBool((p + "active").c_str(), rfCombos[i].active);
    }

    prefs.putInt("sceneCount", sceneCount);
    for (int i = 0; i < sceneCount; i++)
    {
        String p = "sc" + String(i) + "_";
        prefs.putUChar((p + "id").c_str(), scenes[i].id);
        prefs.putString((p + "name").c_str(), scenes[i].name);
        prefs.putBool((p + "seq").c_str(), scenes[i].isSequential);
        prefs.putBool((p + "active").c_str(), scenes[i].active);
        prefs.putUChar((p + "cnt").c_str(), scenes[i].stepCount);
        prefs.putBool((p + "te").c_str(), scenes[i].timeEnabled);
        prefs.putUChar((p + "th").c_str(), scenes[i].triggerHour);
        prefs.putUChar((p + "tm").c_str(), scenes[i].triggerMinute);
        prefs.putUChar((p + "wm").c_str(), scenes[i].weekdayMask);
        prefs.putUShort((p + "ri").c_str(), scenes[i].repeatInterval);

        for (int j = 0; j < scenes[i].stepCount; j++)
        {
            String sp = p + "s" + String(j) + "_";
            prefs.putUChar((sp + "r").c_str(), scenes[i].steps[j].relay);
            prefs.putUChar((sp + "a").c_str(), scenes[i].steps[j].action);
            prefs.putUShort((sp + "d").c_str(), scenes[i].steps[j].duration);
            prefs.putUShort((sp + "delay").c_str(), scenes[i].steps[j].delayBefore);
        }
    }

    prefs.end();
}
void loadAllSettings()
{
    prefs.begin("relay", false);

    for (int i = 0; i < 4; i++)
    {
        relays[i].logic = prefs.getBool(("logic" + String(i)).c_str(), false);
        relays[i].notifySMS = prefs.getBool(("rnotify" + String(i)).c_str(), false);
        relays[i].isActive = false;
        relays[i].duration = 0;
    }

    allowedCount = prefs.getInt("numCount", 0);
    for (int i = 0; i < allowedCount; i++)
        allowedNumbers[i] = prefs.getString(("num" + String(i)).c_str(), "");

    rfButtonCount = prefs.getInt("rfCount", 0);
    for (int i = 0; i < rfButtonCount; i++)
    {
        String p = "rf" + String(i) + "_";
        rfButtons[i].code = prefs.getULong((p + "code").c_str(), 0);
        rfButtons[i].protocol = prefs.getUChar((p + "proto").c_str(), 1);
        rfButtons[i].bitLength = prefs.getUShort((p + "bits").c_str(), 24);
        String name = prefs.getString((p + "name").c_str(), "Button");
        strncpy(rfButtons[i].name, name.c_str(), 15);
        rfButtons[i].name[15] = '\0';
        rfButtons[i].singleAction = prefs.getUChar((p + "sa").c_str(), 0);
        rfButtons[i].singleTarget = prefs.getUChar((p + "st").c_str(), 0);
        rfButtons[i].doubleAction = prefs.getUChar((p + "da").c_str(), 0);
        rfButtons[i].doubleTarget = prefs.getUChar((p + "dt").c_str(), 0);
        rfButtons[i].longAction = prefs.getUChar((p + "la").c_str(), 0);
        rfButtons[i].longTarget = prefs.getUChar((p + "lt").c_str(), 0);
        rfButtons[i].tripleAction = prefs.getUChar((p + "ta").c_str(), 0);
        rfButtons[i].tripleTarget = prefs.getUChar((p + "tt").c_str(), 0);
        rfButtons[i].active = prefs.getBool((p + "active").c_str(), true);
    }

    rfComboCount = prefs.getInt("comboCount", 0);
    for (int i = 0; i < rfComboCount; i++)
    {
        String p = "cb" + String(i) + "_";
        rfCombos[i].code1 = prefs.getULong((p + "c1").c_str(), 0);
        rfCombos[i].code2 = prefs.getULong((p + "c2").c_str(), 0);
        String name = prefs.getString((p + "name").c_str(), "Combo");
        strncpy(rfCombos[i].name, name.c_str(), 15);
        rfCombos[i].name[15] = '\0';
        rfCombos[i].actionType = prefs.getUChar((p + "type").c_str(), 0);
        rfCombos[i].actionId = prefs.getUChar((p + "id").c_str(), 0);
        rfCombos[i].active = prefs.getBool((p + "active").c_str(), true);
    }

    sceneCount = prefs.getInt("sceneCount", 0);
    for (int i = 0; i < sceneCount; i++)
    {
        String p = "sc" + String(i) + "_";
        scenes[i].id = prefs.getUChar((p + "id").c_str(), i);
        String name = prefs.getString((p + "name").c_str(), "Scene");
        strncpy(scenes[i].name, name.c_str(), 31);
        scenes[i].name[31] = '\0';
        scenes[i].isSequential = prefs.getBool((p + "seq").c_str(), false);
        scenes[i].active = prefs.getBool((p + "active").c_str(), true);
        scenes[i].stepCount = prefs.getUChar((p + "cnt").c_str(), 0);
        scenes[i].timeEnabled = prefs.getBool((p + "te").c_str(), false);
        scenes[i].triggerHour = prefs.getUChar((p + "th").c_str(), 0);
        scenes[i].triggerMinute = prefs.getUChar((p + "tm").c_str(), 0);
        scenes[i].weekdayMask = prefs.getUChar((p + "wm").c_str(), 0x7F);
        scenes[i].repeatInterval = prefs.getUShort((p + "ri").c_str(), 0);
        scenes[i].triggeredToday = false;
        scenes[i].lastRunMs = 0;

        for (int j = 0; j < scenes[i].stepCount; j++)
        {
            String sp = p + "s" + String(j) + "_";
            scenes[i].steps[j].relay = prefs.getUChar((sp + "r").c_str(), 0);
            scenes[i].steps[j].action = prefs.getUChar((sp + "a").c_str(), 0);
            scenes[i].steps[j].duration = prefs.getUShort((sp + "d").c_str(), 0);
            scenes[i].steps[j].delayBefore = prefs.getUShort((sp + "delay").c_str(), 0);
        }
    }

    prefs.end();
}
// ==================== NVS Sensors ====================
void saveRelayStates()
{
    if (!enableStatePersistence)
        return;

    prefs.begin("relay", false);

    for (int i = 0; i < 4; i++)
    {
        prefs.putBool(
            ("r" + String(i) + "act").c_str(),
            relays[i].isActive);
    }

    prefs.end();
}
void saveSensors()
{
    prefs.begin("sens", false);
    prefs.putUChar("cnt", (uint8_t)rfSensorCount);
    for (int i = 0; i < rfSensorCount; i++)
    {
        char key[8];
        snprintf(key, sizeof(key), "s%d", i);
        prefs.putBytes(key, &rfSensors[i], sizeof(RFSensor));
    }
    prefs.end();
    Serial.printf("[NVS] Saved %d sensors\n", rfSensorCount);
}

void loadSensors()
{
    prefs.begin("sens", true);
    int cnt = prefs.getUChar("cnt", 0);
    if (cnt > MAX_RF_SENSORS)
        cnt = MAX_RF_SENSORS;
    rfSensorCount = 0;

    for (int i = 0; i < cnt; i++)
    {
        char key[8];
        snprintf(key, sizeof(key), "s%d", i);
        size_t got = prefs.getBytes(key, &rfSensors[i], sizeof(RFSensor));

        if (got != sizeof(RFSensor))
        {
            Serial.printf("[NVS] Sensor %d corrupt, skipped\n", i);
            memset(&rfSensors[i], 0, sizeof(RFSensor));
            continue;
        }

        // Reset runtime fields
        rfSensors[i].hasValue = false;
        rfSensors[i].lastValue = 0.0f;
        rfSensors[i].lastUpdateMs = 0;
        rfSensors[i].rxCount = 0;

        if (!rfSensors[i].active || rfSensors[i].name[0] == '\0')
            continue;

        rfSensorCount++;
    }
    prefs.end();

    // Clear sensor histories
    memset(sensorHistory, 0, sizeof(sensorHistory));

    Serial.printf("[NVS] Loaded %d sensors\n", rfSensorCount);
}
void loadAutomations()
{
    prefs.begin("auto", true);
    int cnt = prefs.getUChar("cnt", 0);
    if (cnt > MAX_AUTOMATIONS)
        cnt = MAX_AUTOMATIONS;
    automationCount = 0;

    for (int i = 0; i < cnt; i++)
    {
        char key[8];
        snprintf(key, sizeof(key), "a%d", i);
        size_t got = prefs.getBytes(key, &automations[i], sizeof(Automation));

        if (got != sizeof(Automation))
        {
            Serial.printf("[NVS] Automation %d corrupt, skipped\n", i);
            memset(&automations[i], 0, sizeof(Automation));
            continue;
        }

        // Reset runtime fields
        automations[i].lastTriggerMs = 0;
        automations[i].lastEval = false;
        automations[i].triggered = false;
        automations[i].hysteresis.state = false;

        if (!automations[i].active && automations[i].name[0] == '\0')
            continue;
        automationCount++;
    }
    prefs.end();
    Serial.printf("[NVS] Loaded %d automations\n", automationCount);
}

void saveAutomations()
{
    prefs.begin("auto", false);

    prefs.putUChar("cnt", automationCount);

    for (int i = 0; i < automationCount; i++)
    {
        char key[8];
        snprintf(key, sizeof(key), "a%d", i);

        prefs.putBytes(
            key,
            &automations[i],
            sizeof(Automation));
    }

    prefs.end();
}
void saveAllSettingsAsync()
{
    SaveCommand cmd = {0};
    xQueueSend(qSave, &cmd, pdMS_TO_TICKS(100));
}
void saveMQTTSettings()
{
    prefs.begin("mqtt", false);
    prefs.putBool("en", mqttEnabled);
    prefs.putUShort("port", mqttPort);
    prefs.putString("broker", mqttBroker);
    prefs.putString("cid", mqttClientId);
    prefs.putString("user", mqttUser);
    prefs.putString("pass", mqttPass);
    prefs.putString("tstat", mqttTopicStatus);
    prefs.putString("tlog", mqttTopicLog);
    prefs.putString("tcmd", mqttTopicCmd);
    prefs.putString("apn", mqttAPN);
    prefs.putString("apnu", mqttAPNUser);
    prefs.putString("apnp", mqttAPNPass);
    prefs.end();
    Serial.println("[Storage] MQTT settings saved");
}

void loadMQTTSettings()
{
    prefs.begin("mqtt", true);
    if (!prefs.isKey("broker"))
    {
        prefs.end();
        Serial.println("[Storage] MQTT settings not initialized, using defaults");
        return;
    }
    mqttEnabled = prefs.getBool("en", false);
    mqttPort = prefs.getUShort("port", 1883);
    prefs.getString("broker", mqttBroker, sizeof(mqttBroker));
    prefs.getString("cid", mqttClientId, sizeof(mqttClientId));
    prefs.getString("user", mqttUser, sizeof(mqttUser));
    prefs.getString("pass", mqttPass, sizeof(mqttPass));
    prefs.getString("tstat", mqttTopicStatus, sizeof(mqttTopicStatus));
    prefs.getString("tlog", mqttTopicLog, sizeof(mqttTopicLog));
    prefs.getString("tcmd", mqttTopicCmd, sizeof(mqttTopicCmd));
    prefs.getString("apn", mqttAPN, sizeof(mqttAPN));
    prefs.getString("apnu", mqttAPNUser, sizeof(mqttAPNUser));
    prefs.getString("apnp", mqttAPNPass, sizeof(mqttAPNPass));
    prefs.end();
    Serial.printf("[Storage] MQTT loaded: enabled=%d, broker=%s, apn=%s\n",
                  mqttEnabled, mqttBroker, mqttAPN);
}