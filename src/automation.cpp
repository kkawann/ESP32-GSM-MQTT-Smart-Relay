#include "automation.h"
#include "globals.h"
#include "relay.h"
#include "clock.h"
#include "scene.h"
#include "gsm.h"
#include "utils.h"
#include "types.h"

// ─── 1. Check Weekday Window for Hysteresis ───
bool isInDayWindow(const Automation &a)
{
    for (int i = 0; i < a.conditionCount; i++)
    {
        if (a.conditions[i].type != COND_DAY_OF_WEEK)
            continue;
        uint8_t dow = getDayOfWeek();
        bool ok = (a.conditions[i].weekdayMask & (1 << dow)) != 0;
        if (a.conditions[i].negate)
            ok = !ok;
        if (!ok)
            return false;
    }
    return true;
}

// ─── 2. Check Time Window for Hysteresis ───
bool isInTimeWindow(const Automation &a)
{
    for (int i = 0; i < a.conditionCount; i++)
    {
        if (a.conditions[i].type == COND_TIME_BETWEEN)
        {
            uint8_t h, m, s;
            if (!getCurrentTime(h, m, s))
                return false;

            uint16_t nowMin = (uint16_t)h * 60 + m;
            uint16_t startMin = (uint16_t)a.conditions[i].hourStart * 60 + a.conditions[i].minuteStart;
            uint16_t endMin = (uint16_t)a.conditions[i].hourEnd * 60 + a.conditions[i].minuteEnd;

            bool inWindow;
            if (startMin <= endMin)
                inWindow = (nowMin >= startMin && nowMin <= endMin);
            else
                inWindow = (nowMin >= startMin || nowMin <= endMin); // Midnight crossover

            if (a.conditions[i].negate)
                inWindow = !inWindow;

            if (!inWindow)
                return false;
        }
    }
    return true;
}

// ─── 3. Control and Process Hysteresis Logic ───
bool checkHysteresis(Automation &a)
{
    if (!a.hysteresis.enabled)
        return false;

    // Hysteresis acts as an independent segment, using time and day as validation gates
    if (!isInTimeWindow(a) || !isInDayWindow(a))
    {
        a.hysteresis.state = false; // Reset state when outside the valid window
        return false;
    }

    uint8_t si = a.hysteresis.sensorId;
    if (si >= rfSensorCount || !rfSensors[si].active || !rfSensors[si].hasValue)
        return false;

    float v = rfSensors[si].lastValue;
    bool changed = false;

    // Sync with actual relay state to prevent desync
    bool realRelayState = false;
    if (xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        realRelayState = relays[a.hysteresis.relayId].isActive;
        xSemaphoreGive(mutexRelay);
    }
    a.hysteresis.state = realRelayState;

    if (!a.hysteresis.state && v >= a.hysteresis.onThreshold)
    {
        a.hysteresis.state = true;
        activateRelayTS(a.hysteresis.relayId, 0);
        changed = true;
        char msg[80];
        snprintf(msg, 80, "[%s] Hyst ON: %.1f >= %.1f", a.name, v, a.hysteresis.onThreshold);
        addLog(0, 0, msg);
    }
    else if (a.hysteresis.state && v <= a.hysteresis.offThreshold)
    {
        a.hysteresis.state = false;
        deactivateRelayTS(a.hysteresis.relayId);
        changed = true;
        char msg[80];
        snprintf(msg, 80, "[%s] Hyst OFF: %.1f <= %.1f", a.name, v, a.hysteresis.offThreshold);
        addLog(0, 0, msg);
    }

    return changed;
}

// ─── Helper: get effective sensor list for a condition ───
static bool getSensorValue(uint8_t sid, float &outVal)
{
    if (sid >= rfSensorCount || !rfSensors[sid].active || !rfSensors[sid].hasValue)
        return false;
    outVal = rfSensors[sid].lastValue;
    return true;
}

// ─── 4. Evaluate Individual Conditions ───
bool evalCondition(const AutoCondition &c)
{
    bool result = false;

    switch ((ConditionType)c.type)
    {
    case COND_ALWAYS:
        result = true;
        break;

    case COND_RELAY_ON:
        if (c.relayId < 4)
        {
            if (xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                result = relays[c.relayId].isActive;
                xSemaphoreGive(mutexRelay);
            }
        }
        break;

    case COND_RELAY_OFF:
        if (c.relayId < 4)
        {
            if (xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                result = !relays[c.relayId].isActive;
                xSemaphoreGive(mutexRelay);
            }
        }
        break;

    case COND_TIME_BETWEEN:
    {
        uint8_t h, m, s;
        if (!getCurrentTime(h, m, s))
        {
            result = false;
            break;
        }
        uint16_t nowMin = (uint16_t)h * 60 + m;
        uint16_t startMin = (uint16_t)c.hourStart * 60 + c.minuteStart;
        uint16_t endMin = (uint16_t)c.hourEnd * 60 + c.minuteEnd;

        if (startMin <= endMin)
            result = (nowMin >= startMin && nowMin <= endMin);
        else
            result = (nowMin >= startMin || nowMin <= endMin);
        break;
    }

    case COND_DAY_OF_WEEK:
    {
        uint8_t h, m, s;
        if (!getCurrentTime(h, m, s))
        {
            result = false;
            break;
        }
        uint8_t dow = getDayOfWeek();
        result = (c.weekdayMask & (1 << dow)) != 0;
        break;
    }

    case COND_SENSOR_GT:
    {
        // Multi-sensor: ANY of the bound sensors satisfies the condition (OR within condition)
        uint8_t n = c.sensorIdCount;
        bool anyOk = false;
        if (n == 0)
        {
            // Legacy single-sensor fallback
            float v;
            if (getSensorValue(c.sensorId, v) && v > c.thresh1)
                anyOk = true;
        }
        else
        {
            for (uint8_t k = 0; k < n && k < MAX_SENSORS_PER_COND; k++)
            {
                float v;
                if (getSensorValue(c.sensorIds[k], v) && v > c.thresh1)
                {
                    anyOk = true;
                    break;
                }
            }
        }
        result = anyOk;
        break;
    }

    case COND_SENSOR_LT:
    {
        uint8_t n = c.sensorIdCount;
        bool anyOk = false;
        if (n == 0)
        {
            float v;
            if (getSensorValue(c.sensorId, v) && v < c.thresh1)
                anyOk = true;
        }
        else
        {
            for (uint8_t k = 0; k < n && k < MAX_SENSORS_PER_COND; k++)
            {
                float v;
                if (getSensorValue(c.sensorIds[k], v) && v < c.thresh1)
                {
                    anyOk = true;
                    break;
                }
            }
        }
        result = anyOk;
        break;
    }

    case COND_SENSOR_BETWEEN:
    {
        uint8_t n = c.sensorIdCount;
        bool anyOk = false;
        if (n == 0)
        {
            float v;
            if (getSensorValue(c.sensorId, v) && v >= c.thresh1 && v <= c.thresh2)
                anyOk = true;
        }
        else
        {
            for (uint8_t k = 0; k < n && k < MAX_SENSORS_PER_COND; k++)
            {
                float v;
                if (getSensorValue(c.sensorIds[k], v) && v >= c.thresh1 && v <= c.thresh2)
                {
                    anyOk = true;
                    break;
                }
            }
        }
        result = anyOk;
        break;
    }

    case COND_SENSOR_OUTSIDE:
    {
        uint8_t n = c.sensorIdCount;
        bool anyOk = false;
        if (n == 0)
        {
            float v;
            if (getSensorValue(c.sensorId, v) && (v < c.thresh1 || v > c.thresh2))
                anyOk = true;
        }
        else
        {
            for (uint8_t k = 0; k < n && k < MAX_SENSORS_PER_COND; k++)
            {
                float v;
                if (getSensorValue(c.sensorIds[k], v) && (v < c.thresh1 || v > c.thresh2))
                {
                    anyOk = true;
                    break;
                }
            }
        }
        result = anyOk;
        break;
    }

    case COND_SENSOR_OFFLINE:
    {
        uint8_t n = c.sensorIdCount;
        bool anyOk = false;
        if (n == 0)
        {
            if (c.sensorId < rfSensorCount && rfSensors[c.sensorId].active)
            {
                if (!rfSensors[c.sensorId].hasValue)
                    anyOk = true;
                else
                {
                    uint32_t ageMs = (uint32_t)millis() - rfSensors[c.sensorId].lastUpdateMs;
                    anyOk = (ageMs >= (uint32_t)c.offlineMinutes * 60000UL);
                }
            }
        }
        else
        {
            for (uint8_t k = 0; k < n && k < MAX_SENSORS_PER_COND; k++)
            {
                uint8_t sid = c.sensorIds[k];
                if (sid >= rfSensorCount || !rfSensors[sid].active)
                    continue;
                if (!rfSensors[sid].hasValue)
                {
                    anyOk = true;
                    break;
                }
                uint32_t ageMs = (uint32_t)millis() - rfSensors[sid].lastUpdateMs;
                if (ageMs >= (uint32_t)c.offlineMinutes * 60000UL)
                {
                    anyOk = true;
                    break;
                }
            }
        }
        result = anyOk;
        break;
    }

    default:
        result = false;
        break;
    }

    return c.negate ? !result : result;
}

// ─── 5. Comprehensive evaluation of all automation conditions (AND/OR logic) ───
bool evalAutomationNoTime(const Automation &a)
{
    if (!a.active)
        return false;
    if (a.conditionCount == 0)
        return false;

    bool anyTrue = false;
    bool allTrue = true;

    for (int i = 0; i < a.conditionCount; i++)
    {
        bool res = evalCondition(a.conditions[i]);
        if (res)
            anyTrue = true;
        if (!res)
            allTrue = false;
    }

    if ((LogicOperator)a.logicOp == LOGIC_AND)
        return allTrue;
    else
        return anyTrue;
}

// ─── 6. Main evaluation and monitoring cycle for automations ───
void checkAutomations()
{
    unsigned long now = millis();

    for (int i = 0; i < automationCount; i++)
    {
        if (!automations[i].active)
            continue;

        // Evaluate independent hysteresis system
        checkHysteresis(automations[i]);

        // Intergrated evaluation of all conditions (sensors, relays, time, day)
        bool currentEval = evalAutomationNoTime(automations[i]);

        // Detect positive edge transition (False to True status change)
        bool shouldTrigger = currentEval && !automations[i].lastEval;

        // Reset SMS sent flag when all conditions are cleared (False)
        if (!currentEval && automations[i].lastEval)
        {
            automations[i].smsSentFlag = false;
        }

        automations[i].lastEval = currentEval;

        if (!shouldTrigger)
            continue;

        // Check cooldown interval to suppress rapid sensor state changes
        if (automations[i].cooldownEnabled && automations[i].lastTriggerMs > 0)
        {
            unsigned long coolMs = (unsigned long)automations[i].cooldownMinutes * 60000UL;
            if ((now - automations[i].lastTriggerMs) < coolMs)
            {
                char msg[64];
                snprintf(msg, 64, "[%s] cooldown skip", automations[i].name);
                addLog(4, 0, msg);
                continue;
            }
        }

        // Execute trigger actions
        automations[i].lastTriggerMs = now;
        automations[i].triggered = true;

        char logMsg[64];
        snprintf(logMsg, 64, "AUTO '%s' triggered", automations[i].name);
        addLog(4, 0, logMsg);

        for (int j = 0; j < automations[i].actionCount; j++)
        {
            AutoAction &act = automations[i].actions[j];

            // Ensure only one SMS is sent per trigger cycle
            if (act.type == AACT_SEND_SMS)
            {
                if (automations[i].smsSentFlag)
                    continue;
                automations[i].smsSentFlag = true;
            }

            executeAutoAction(act);
        }
    }
}

// ─── 7. Execute automation actions (No pulse overlapping) ───
void executeAutoAction(const AutoAction &act)
{
    unsigned long delayMs = (unsigned long)act.delayBeforeMs * 1000UL;

    switch ((AutoActionType)act.type)
    {
    case AACT_RELAY_TIMED:
        if (act.targetId < 4)
        {
            if (delayMs > 0)
            {
                addToQueue(0, act.targetId, 3, delayMs, act.durationMs);
            }
            else
            {
                activateRelayTS(act.targetId, act.durationMs);
            }
        }
        break;

    case AACT_RELAY_OFF:
        if (act.targetId < 4)
        {
            if (delayMs > 0)
                addToQueue(0, act.targetId, 1, delayMs);
            else
                deactivateRelayTS(act.targetId);
        }
        break;

    case AACT_RELAY_TOGGLE:
        if (act.targetId < 4)
        {
            if (delayMs > 0)
                addToQueue(0, act.targetId, 3, delayMs);
            else
                toggleRelayTS(act.targetId);
        }
        break;

    case AACT_RELAY_PULSE:
        if (act.targetId < 4 && act.pulseCount > 0)
        {
            if (delayMs > 0)
            {
                // Queue structure enhancement may be needed for pulse delay management
            }

            // Set up non-blocking pulse phases
            pulseStates[act.targetId].active = true;
            pulseStates[act.targetId].relayId = act.targetId;
            pulseStates[act.targetId].remaining = act.pulseCount;
            pulseStates[act.targetId].onMs = act.pulseOnMs;
            pulseStates[act.targetId].offMs = act.pulseOffMs;
            pulseStates[act.targetId].phase = true; // Start status cycle with ON
            pulseStates[act.targetId].nextToggle = millis() + act.pulseOnMs;
            activateRelayTS(act.targetId, 0);
        }
        break;

    case AACT_ALL_OFF:
        for (int i = 0; i < 4; i++)
            deactivateRelayTS(i);
        break;

    case AACT_SCENE:
        if (delayMs > 0)
            addToQueue(1, act.targetId, 0, delayMs);
        else
            executeScene(act.targetId);
        break;

    case AACT_SEND_SMS:
        if (strlen(act.smsText) > 0)
        {
            for (int i = 0; i < allowedCount; i++)
            {
                if (allowedNumbers[i].length() > 5)
                {
                    sendSMSAsync(allowedNumbers[i].c_str(), act.smsText);
                }
            }
        }
        break;

    case AACT_NONE:
    default:
        break;
    }
}

// ─── 8. Background processing of non-blocking pulse states ───
void processPulseStates()
{
    unsigned long now = millis();
    for (int i = 0; i < 4; i++)
    {
        if (!pulseStates[i].active)
            continue;

        if (now < pulseStates[i].nextToggle)
            continue;

        if (pulseStates[i].phase)
        {
            // End of ON phase -> Turn OFF relay
            deactivateRelayTS(i);
            pulseStates[i].phase = false;
            pulseStates[i].nextToggle = now + pulseStates[i].offMs;
        }
        else
        {
            // End of OFF phase -> Check remaining pulse count
            pulseStates[i].remaining--;
            if (pulseStates[i].remaining == 0)
            {
                pulseStates[i].active = false;
            }
            else
            {
                // Start next pulse cycle
                activateRelayTS(i, 0);
                pulseStates[i].phase = true;
                pulseStates[i].nextToggle = now + pulseStates[i].onMs;
            }
        }
    }
}
