#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <RCSwitch.h>
#include "SIM800_Arduino.h"
#include "types.h"
#include "config.h"
#include "SIM800_MQTT.h"

// ---------------------------------------------------------------------------
// MQTT
// ---------------------------------------------------------------------------
extern MqttClient_t mqttClient;
extern bool mqttEnabled;
extern char mqttBroker[64];
extern uint16_t mqttPort;
extern char mqttClientId[32];
extern char mqttUser[32];
extern char mqttPass[32];
extern char mqttTopicStatus[64];
extern char mqttTopicLog[64];
extern char mqttTopicCmd[64];
extern char mqttTopicCommand[64];
extern char mqttTopicRelay[64];
extern char mqttAPN[32];
extern char mqttAPNUser[32];
extern char mqttAPNPass[32];

// ---------------------------------------------------------------------------
// Hardware objects
// ---------------------------------------------------------------------------
extern SIM800_t sim800;
extern WebServer server;
extern Preferences prefs;
extern RCSwitch rcSwitch;
extern SemaphoreHandle_t xSensorMutex;

// ---------------------------------------------------------------------------
// State arrays
// ---------------------------------------------------------------------------
extern RelayState relays[4];
extern RFButton rfButtons[MAX_RF_BUTTONS];
extern int rfButtonCount;
extern RFCombo rfCombos[MAX_RF_COMBOS];
extern int rfComboCount;
extern RFSensor rfSensors[MAX_RF_SENSORS];
extern uint8_t rfSensorCount;
extern SensorRingBuf sensorHistory[MAX_RF_SENSORS];
extern AdvancedScene scenes[MAX_SCENES];
extern int sceneCount;
extern Automation automations[MAX_AUTOMATIONS];
extern int automationCount;
extern PulseState pulseStates[4];
extern String allowedNumbers[MAX_PHONES];
extern int allowedCount;
extern EventQueueItem eventQueue[MAX_EVENT_QUEUE];
extern EventLog eventLogs[MAX_EVENT_LOGS];
extern int logIndex;
extern int logCount;

// ---------------------------------------------------------------------------
// RF state
// ---------------------------------------------------------------------------
extern bool rfLearningMode;
extern unsigned long rfLearningStart;
extern unsigned long rfLearnedCode;
extern uint8_t rfLearnedProtocol;
extern uint16_t rfLearnedBitLength;
extern bool rfCodeReady;
extern unsigned long lastRFCode;
extern unsigned long lastRFTime;
extern int rfClickCount;
extern unsigned long firstClickTime;
extern bool longPressDetected;
extern RFEventHistory rfHistory[5];
extern int rfHistoryIndex;

// ---------------------------------------------------------------------------
// Sensor learning
// ---------------------------------------------------------------------------
extern uint32_t s_learnedCode;
extern uint8_t s_learnedProtocol;
extern uint16_t s_learnedBits;
extern bool s_learnWaiting;
extern uint32_t s_learnStartMs;

// ---------------------------------------------------------------------------
// Scene runtime
// ---------------------------------------------------------------------------
extern int currentSceneRunning;
extern int currentSceneStep;
extern unsigned long sceneStepStartTime;

// ---------------------------------------------------------------------------
// GSM state
// ---------------------------------------------------------------------------
extern bool networkReady;
extern bool wifiReady;
extern bool sim800Ready;
extern unsigned long lastGSMCheck;
extern unsigned long gsmFailureStart;
extern bool gsmFailureDetected;
extern int hardwareResetCount;
extern int softwareResetCount;
extern bool hwResetInProgress;
extern unsigned long hwResetTimer;
extern int hwResetStage;
extern bool softResetDone;
extern volatile unsigned long lastSIM800Activity;
extern bool gsmBooting;
extern unsigned long gsmBootStarted;

// ---------------------------------------------------------------------------
// Clock
// ---------------------------------------------------------------------------
extern InternalClock internalClock;
extern ClockSyncState clockSyncState;
extern unsigned long clockSyncStarted;
extern String clockSyncBuffer;
extern unsigned long lastClockSync;
extern unsigned long lastSyncAttempt;
extern bool pendingTimeSync;
extern unsigned long simInitDoneAt;
extern String clockRawBuffer;
extern bool clockWaitingResp;

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
extern bool enableStatePersistence;

// ---------------------------------------------------------------------------
// FreeRTOS: tasks, queues, mutexes, timers
// ---------------------------------------------------------------------------
extern TaskHandle_t taskWebServer;
extern TaskHandle_t taskGSM;
extern TaskHandle_t taskRF;
extern TaskHandle_t taskScene;
extern TaskHandle_t taskSave;
extern TaskHandle_t taskMQTT;

extern QueueHandle_t qRelay;
extern QueueHandle_t qSave;
extern QueueHandle_t qSMSOut;
extern QueueHandle_t qSMSIn;

extern SemaphoreHandle_t mutexRelay;
extern SemaphoreHandle_t mutexClock;
extern SemaphoreHandle_t mutexSerial;

extern TimerHandle_t timerRelay[4];
extern TimerHandle_t timerGSMCheck;
extern TimerHandle_t timerClockSync;
extern TimerHandle_t timerLED;

// Default cellular APN (Iran: mcinet for MCI / Irancell)
#define MQTT_APN "mcinet"

#endif
