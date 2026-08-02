#include "globals.h"

// ---------------------------------------------------------------------------
// MQTT client & settings
// ---------------------------------------------------------------------------
MqttClient_t mqttClient;
bool mqttEnabled = true;
char mqttBroker[64] = "broker.hivemq.com";
uint16_t mqttPort = 1883;
char mqttClientId[32] = "emqx_MzYwNT";
char mqttUser[32] = "";
char mqttPass[32] = "";
char mqttTopicStatus[64] = "device/SmartRelay_001/status";
char mqttTopicCommand[64] = "device/SmartRelay_001/cmd";
char mqttTopicLog[64] = "device/SmartRelay_001/log";
char mqttTopicRelay[64] = "device/SmartRelay_001/relay";
char mqttTopicCmd[64] = "smartrelay/cmd";
char mqttAPN[32] = "mcinet";
char mqttAPNUser[32] = "";
char mqttAPNPass[32] = "";

// ---------------------------------------------------------------------------
// Hardware objects
// ---------------------------------------------------------------------------
SIM800_t sim800;
WebServer server(80);
Preferences prefs;
RCSwitch rcSwitch = RCSwitch();

// ---------------------------------------------------------------------------
// Relays, RF, sensors, scenes, automations
// ---------------------------------------------------------------------------
RelayState relays[4] = {};
RFButton rfButtons[MAX_RF_BUTTONS] = {};
int rfButtonCount = 0;
RFCombo rfCombos[MAX_RF_COMBOS] = {};
int rfComboCount = 0;

bool rfLearningMode = false;
unsigned long rfLearningStart = 0;
unsigned long rfLearnedCode = 0;
uint8_t rfLearnedProtocol = 0;
uint16_t rfLearnedBitLength = 0;
bool rfCodeReady = false;
unsigned long lastRFCode = 0;
unsigned long lastRFTime = 0;
int rfClickCount = 0;
unsigned long firstClickTime = 0;
bool longPressDetected = false;
RFEventHistory rfHistory[RF_HISTORY_SIZE] = {};
int rfHistoryIndex = 0;
uint8_t lastRFProtocol = 0;

RFSensor rfSensors[MAX_RF_SENSORS] = {};
uint8_t rfSensorCount = 0;
SensorRingBuf sensorHistory[MAX_RF_SENSORS] = {};
uint32_t s_learnedCode = 0;
uint8_t s_learnedProtocol = 0;
uint16_t s_learnedBits = 0;
bool s_learnWaiting = false;
uint32_t s_learnStartMs = 0;

AdvancedScene scenes[MAX_SCENES] = {};
int sceneCount = 0;
int currentSceneRunning = -1;
int currentSceneStep = 0;
unsigned long sceneStepStartTime = 0;

Automation automations[MAX_AUTOMATIONS] = {};
int automationCount = 0;
PulseState pulseStates[4] = {};

String allowedNumbers[MAX_PHONES];
int allowedCount = 0;

EventQueueItem eventQueue[MAX_EVENT_QUEUE] = {};
EventLog eventLogs[MAX_EVENT_LOGS] = {};
int logIndex = 0;
int logCount = 0;

// ---------------------------------------------------------------------------
// GSM / network state
// ---------------------------------------------------------------------------
bool networkReady = false;
bool wifiReady = false;
bool sim800Ready = false;
unsigned long lastGSMCheck = 0;
unsigned long gsmFailureStart = 0;
bool gsmFailureDetected = false;
int hardwareResetCount = 0;
int softwareResetCount = 0;
bool hwResetInProgress = false;
unsigned long hwResetTimer = 0;
int hwResetStage = 0;
bool softResetDone = false;
volatile unsigned long lastSIM800Activity = 0;
bool gsmBooting = false;
unsigned long gsmBootStarted = 0;

// ---------------------------------------------------------------------------
// Clock
// ---------------------------------------------------------------------------
InternalClock internalClock = {0, 0, 0, 1, 1, 2024, 0, false};
ClockSyncState clockSyncState = CSYNC_IDLE;
unsigned long clockSyncStarted = 0;
String clockSyncBuffer = "";
unsigned long lastClockSync = 0;
unsigned long lastSyncAttempt = 0;
bool pendingTimeSync = false;
unsigned long simInitDoneAt = 0;
String clockRawBuffer = "";
bool clockWaitingResp = false;

// ---------------------------------------------------------------------------
// Settings & FreeRTOS primitives
// ---------------------------------------------------------------------------
bool enableStatePersistence = true;
SemaphoreHandle_t xSensorMutex = NULL;
TaskHandle_t taskWebServer = NULL;
TaskHandle_t taskGSM = NULL;
TaskHandle_t taskRF = NULL;
TaskHandle_t taskScene = NULL;
TaskHandle_t taskSave = NULL;
TaskHandle_t taskMQTT = NULL;

QueueHandle_t qRelay = NULL;
QueueHandle_t qSave = NULL;
QueueHandle_t qSMSOut = NULL;
QueueHandle_t qSMSIn = NULL;

SemaphoreHandle_t mutexRelay = NULL;
SemaphoreHandle_t mutexClock = NULL;
SemaphoreHandle_t mutexSerial = NULL;

TimerHandle_t timerRelay[4] = {NULL};
TimerHandle_t timerGSMCheck = NULL;
TimerHandle_t timerClockSync = NULL;
TimerHandle_t timerLED = NULL;
