#pragma once

#include <Arduino.h>

bool requireAuth();

// Core
void handleRoot();
void handleAPIStatus();
void handleAPIRelay();

// RF remote
void handleAPIRFLearn();
void handleAPIRFLearned();
void handleAPIRFCancel();
void handleAPIRFSave();
void handleAPIRFButtons();
void handleAPIRFDelete();
void handleAPICombos();
void handleAPIComboSave();
void handleAPIComboDelete();

// Scenes
void handleAPIScenes();
void handleAPISceneSave();
void handleAPISceneRun();
void handleAPISceneDelete();

// MQTT
void handleAPIMQTTStatus();
void handleAPIMQTTSettings();
void handleAPIMQTTConnect();
void handleAPIMQTTDisconnect();

// Relay settings & system
void handleAPIRelaySettings();
void handleAPIRelaySettingsSave();
void handleAPIResetSoft();
void handleAPIResetHard();
void handleAPIClear();
void handleAPIPhones();
void handleAPIPhoneSave();
void handleAPIPhoneDelete();
void handleAPILogs();
void handleAPILogsClear();

// Sensors
void handleAPISensors();
void handleAPISensorSave();
void handleAPISensorDelete();
void handleAPISensorLearnStart();
void handleAPISensorLearnStatus();
void handleAPISensorLearnCancel();
void handleAPISensorValues();

// Automations
void handleAPIAutomations();
void handleAPIAutomationSave();
void handleAPIAutomationDelete();
void handleAPIAutomationToggle();
void handleAPIAutomationTest();

// WiFi / OTA
void handleAPIWifiConnect();
void handleAPIWifiStatus();
void handleAPIOTACheck();
void handleAPIOTAStatus();
