#include "scene.h"
#include "globals.h"
#include "relay.h"
#include "clock.h"
#include "utils.h"

void addToQueue(uint8_t type, uint8_t id, uint8_t action, unsigned long delayMs, uint32_t duration)
{
    for (int i = 0; i < MAX_EVENT_QUEUE; i++)
    {
        if (!eventQueue[i].active)
        {
            eventQueue[i].type = type;
            eventQueue[i].id = id;
            eventQueue[i].action = action;
            eventQueue[i].executeAt = millis() + delayMs;
            eventQueue[i].duration = duration; // No longer always zero
            eventQueue[i].active = true;
            return;
        }
    }
}

void processEventQueue()
{
    unsigned long now = millis();
    for (int i = 0; i < MAX_EVENT_QUEUE; i++)
    {
        if (!eventQueue[i].active)
            continue;
        if (now >= eventQueue[i].executeAt)
        {
            if (eventQueue[i].type == 0)
            {
                switch (eventQueue[i].action)
                {
                case 1:
                    deactivateRelayTS(eventQueue[i].id);
                    break;
                case 2:
                    activateRelayTS(eventQueue[i].id, 0);
                    break;
                case 3:
                    activateRelayTS(eventQueue[i].id,
                                    eventQueue[i].duration);
                    break; // duration added
                }
            }
            else if (eventQueue[i].type == 1)
            {
                executeScene(eventQueue[i].id);
            }
            eventQueue[i].active = false;
        }
    }
}

void executeAction(uint8_t actionType, uint8_t actionId)
{
    switch (actionType)
    {
    case ACTION_NONE:
        break;
    case ACTION_SCENE:
        executeScene(actionId);
        break;
    case ACTION_RELAY_TOGGLE:
        if (actionId < 4)
            toggleRelayTS(actionId);
        break;
    case ACTION_RELAY_ON:
        if (actionId < 4)
            activateRelayTS(actionId, 0);
        break;
    case ACTION_RELAY_OFF:
        if (actionId < 4)
            deactivateRelayTS(actionId);
        break;
    case ACTION_ALL_OFF:
        for (int i = 0; i < 4; i++)
            deactivateRelayTS(i);
        break;
    }
}
void executeScene(int sceneId)
{
    if (sceneId < 0 || sceneId >= sceneCount)
        return;
    if (!scenes[sceneId].active)
        return;

    char logMsg[64];
    snprintf(logMsg, 64, "Scene '%s' started", scenes[sceneId].name);
    addLog(1, 0, logMsg);

    if (!scenes[sceneId].isSequential)
    {
        for (int i = 0; i < scenes[sceneId].stepCount; i++)
        {
            SceneStep *step = &scenes[sceneId].steps[i];
            if (step->relay > 3)
                continue;

            // Add all steps to the queue with cumulative delay
            unsigned long delayMs = (unsigned long)step->delayBefore * 1000UL;
            delayMs += i * 10UL; // 10ms interval between steps instead of delay(50)

            switch (step->action)
            {
            case 1:
                addToQueue(0, step->relay, 1, delayMs);
                break; // Off
            case 2:
                addToQueue(0, step->relay, 2, delayMs);
                break; // On
            case 3:
                addToQueue(0, step->relay, 3, delayMs, (uint32_t)step->duration * 1000UL);
                break; // Timed
            }
        }
    }
    else
    {
        scenes[sceneId].stepExecuted = false;
        currentSceneRunning = sceneId;
        currentSceneStep = 0;
        sceneStepStartTime = millis();
    }
}
void processSequentialScene()
{
    if (currentSceneRunning < 0)
        return;
    AdvancedScene *scene = &scenes[currentSceneRunning];
    if (currentSceneStep >= scene->stepCount)
    {
        scene->stepExecuted = false;
        currentSceneRunning = -1;
        return;
    }
    SceneStep *step = &scene->steps[currentSceneStep];
    unsigned long elapsed = (millis() - sceneStepStartTime) / 1000UL;
    if (elapsed < step->delayBefore)
        return;
    if (!scene->stepExecuted)
    {
        switch (step->action)
        {
        case 1:
            deactivateRelayTS(step->relay);
            break;
        case 2:
            activateRelayTS(step->relay, 0);
            break;
        case 3:
            activateRelayTS(step->relay, step->duration * 1000UL);
            break;
        }
        scene->stepExecuted = true;
        if (step->action == 3)
            return;
    }
    if (step->action == 3)
    {
        if (relays[step->relay].isActive)
            return;
    }
    currentSceneStep++;
    sceneStepStartTime = millis();
    scene->stepExecuted = false;
}

void checkScheduledScenes()
{
    uint8_t h, m, s;
    if (!getCurrentTime(h, m, s))
        return;

    unsigned long now = millis();
    static uint8_t lastMinute = 255;
    static uint8_t lastHour = 255;

    if (h == 0 && m == 0 && lastHour == 23)
    {
        for (int i = 0; i < sceneCount; i++)
        {
            scenes[i].triggeredToday = false;
        }
    }
    lastHour = h;

    for (int i = 0; i < sceneCount; i++)
    {
        if (!scenes[i].active || !scenes[i].timeEnabled)
            continue;

        if (scenes[i].repeatInterval > 0)
        {
            unsigned long intervalMs = (unsigned long)scenes[i].repeatInterval * 60000UL;
            bool firstRun = (scenes[i].lastRunMs == 0);
            bool timeReached = !firstRun && (now - scenes[i].lastRunMs >= intervalMs);
            if (firstRun || timeReached)
            {
                if (!isWeekdayAllowed(scenes[i].weekdayMask))
                    continue;
                executeScene(i);
                scenes[i].lastRunMs = now;
            }
            continue;
        }

        if (m == lastMinute)
            continue;
        if (scenes[i].triggeredToday)
            continue;
        if (h != scenes[i].triggerHour || m != scenes[i].triggerMinute)
            continue;
        if (!isWeekdayAllowed(scenes[i].weekdayMask))
            continue;

        executeScene(i);
        scenes[i].triggeredToday = true;
        scenes[i].lastRunMs = now;
    }

    if (m != lastMinute)
        lastMinute = m;
}