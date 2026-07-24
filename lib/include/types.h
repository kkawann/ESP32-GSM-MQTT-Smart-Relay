#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ══════════════════════════════════════════════════════════════════
// Enums
// ══════════════════════════════════════════════════════════════════

enum RFEventType
{
    RF_SINGLE_CLICK = 0,
    RF_DOUBLE_CLICK = 1,
    RF_TRIPLE_CLICK = 2,
    RF_LONG_PRESS = 3
};

enum ConditionType
{
    COND_ALWAYS = 0,
    COND_RELAY_ON = 1,
    COND_RELAY_OFF = 2,
    COND_TIME_BETWEEN = 3,
    COND_SENSOR_GT = 4,
    COND_SENSOR_LT = 5,
    COND_SENSOR_BETWEEN = 6,
    COND_SENSOR_OUTSIDE = 7,
    COND_SENSOR_OFFLINE = 8,
    COND_DAY_OF_WEEK = 9
};

enum AutoActionType
{
    AACT_NONE = 0,
    AACT_RELAY_ON = 1,
    AACT_RELAY_OFF = 2,
    AACT_RELAY_TOGGLE = 3,
    AACT_RELAY_TIMED = 4,
    AACT_ALL_OFF = 5,
    AACT_SCENE = 6,
    AACT_SEND_SMS = 7,
    AACT_RELAY_PULSE = 8
};

enum LogicOperator
{
    LOGIC_AND = 0,
    LOGIC_OR = 1
};

enum SensorCondType
{
    SCOND_GT = 0,
    SCOND_LT = 1,
    SCOND_BETWEEN = 2,
    SCOND_OUTSIDE = 3
};

enum ActionType
{
    ACTION_NONE = 0,
    ACTION_SCENE = 1,
    ACTION_RELAY_TOGGLE = 2,
    ACTION_RELAY_ON = 3,
    ACTION_RELAY_OFF = 4,
    ACTION_ALL_OFF = 5
};

enum ClockSyncState
{
    CSYNC_IDLE = 0,
    CSYNC_SENT = 1
};

// ══════════════════════════════════════════════════════════════════
// Structs
// ══════════════════════════════════════════════════════════════════

struct RelayState
{
    bool isActive;
    unsigned long startTime;
    unsigned long duration;
    bool logic;     // true = Active LOW
    bool notifySMS; // send SMS on state change
};

struct RFButton
{
    unsigned long code;
    uint8_t protocol;
    uint16_t bitLength;
    char name[16];
    uint8_t singleAction;
    uint8_t singleTarget;
    uint8_t doubleAction;
    uint8_t doubleTarget;
    uint8_t longAction;
    uint8_t longTarget;
    uint8_t tripleAction;
    uint8_t tripleTarget;
    bool active;
};

struct RFCombo
{
    unsigned long code1;
    unsigned long code2;
    char name[16];
    uint8_t actionType;
    uint8_t actionId;
    bool active;
};

struct RFEventHistory
{
    unsigned long code;
    unsigned long timestamp;
    unsigned long firstSeen;
    int count;
};

struct RFSensor
{
    char name[32];
    uint8_t valueType; // 0=percent, 1=temp, 2=humidity, 3=distance, 4=voltage, 5=raw
    uint32_t baseCode; // base code without value bits
    uint32_t baseMask; // mask to separate base from value
    uint8_t valueBits; // number of value bits
    float scale;       // conversion scale
    float offset;      // conversion offset
    uint8_t protocol;
    uint16_t bitLength;
    bool active;

    // Runtime fields
    bool hasValue;
    float lastValue;
    uint32_t lastUpdateMs;
    uint32_t rxCount;
};

struct SensorHistEntry
{
    float value;
    uint32_t timestamp;
};

#define SENSOR_HIST_SZ 20
struct SensorRingBuf
{
    SensorHistEntry entries[SENSOR_HIST_SZ];
    uint8_t head;
    uint8_t count;
};

struct SceneStep
{
    uint8_t relay;
    uint8_t action;       // 0=none, 1=off, 2=on, 3=timed
    uint16_t duration;    // seconds
    uint16_t delayBefore; // seconds
};

#define MAX_SCENE_STEPS 10
struct AdvancedScene
{
    uint8_t id;
    char name[32];
    bool isSequential;
    bool active;
    uint8_t stepCount;
    SceneStep steps[MAX_SCENE_STEPS];
    bool stepExecuted;

    // Time-based trigger
    bool timeEnabled;
    uint8_t triggerHour;
    uint8_t triggerMinute;
    uint8_t weekdayMask;     // bit 0=Sunday .. 6=Saturday
    uint16_t repeatInterval; // minutes; 0 = once per day
    bool triggeredToday;
    unsigned long lastRunMs;
};

struct Rule
{
    uint8_t id;
    char name[32];
    uint8_t condType;
    uint8_t condRelay;
    uint8_t condHourStart;
    uint8_t condHourEnd;
    uint8_t actionType;
    uint8_t actionId;
    bool active;

    // Sensor fields
    uint8_t sensorId;
    uint8_t sensorCondType;
    float sensorThresh1;
    float sensorThresh2;
};

struct AutoCondition
{
    uint8_t type; // ConditionType
    uint8_t relayId;
    uint8_t sensorId;
    float thresh1;
    float thresh2;
    uint8_t hourStart;
    uint8_t minuteStart;
    uint8_t hourEnd;
    uint8_t minuteEnd;
    uint8_t weekdayMask;
    uint16_t offlineMinutes;
    bool negate; // NOT operator
};

struct AutoAction
{
    uint8_t type;     // AutoActionType
    uint8_t targetId; // relay id or scene id
    uint32_t durationMs;
    char smsText[80];
    uint16_t pulseOnMs;
    uint16_t pulseOffMs;
    uint8_t pulseCount;
    uint16_t delayBeforeMs;
};

struct Hysteresis
{
    bool enabled;
    float onThreshold;
    float offThreshold;
    uint8_t sensorId;
    uint8_t relayId;
    bool state; // runtime: false=off, true=on
};

#define MAX_CONDITIONS 8
#define MAX_ACTIONS 8
struct Automation
{
    uint8_t id;
    char name[32];
    bool active;
    uint8_t logicOp; // LogicOperator

    uint8_t conditionCount;
    AutoCondition conditions[MAX_CONDITIONS];

    uint8_t actionCount;
    AutoAction actions[MAX_ACTIONS];

    Hysteresis hysteresis;

    bool cooldownEnabled;
    uint16_t cooldownMinutes;

    // Runtime fields
    unsigned long lastTriggerMs;
    bool lastEval;
    bool triggered;
    bool smsSentFlag;
};

struct PulseState
{
    bool active;
    uint8_t relayId;
    uint8_t remaining;
    uint16_t onMs;
    uint16_t offMs;
    bool phase; // true=ON, false=OFF
    unsigned long nextToggle;
};

struct EventQueueItem
{
    uint8_t type; // 0=relay, 1=scene
    uint8_t id;
    uint8_t action;
    unsigned long executeAt;
    bool active;
    uint32_t duration;
};

struct EventLog
{
    unsigned long timestamp;
    uint8_t type;  // 0=relay, 1=scene, 2=rf, 3=sms, 4=sys
    uint8_t level; // 0=info, 1=warn, 2=error
    char message[64];
    bool active;
};

struct RelayCommand
{
    uint8_t index;
    uint8_t mode; // 0=off, 1=on, 2=toggle
    uint32_t duration;
};

struct SaveCommand
{
    uint8_t type; // 0=all, 1=relay, 2=sensors, 3=automations
};

struct SMSOutMessage
{
    char number[20];
    char text[160];
};

struct InternalClock
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    unsigned long lastUpdate;
    bool isValid;
};

#endif