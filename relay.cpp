#include "relay.h"
#include "globals.h"
#include "config.h"
#include "utils.h"
#include "storage.h"

void setRelay(int index, bool state)
{
    if (index < 0 || index > 3)
        return;
    int pins[] = {RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN};
    bool actualState = relays[index].logic ? !state : state;
    digitalWrite(pins[index], actualState ? HIGH : LOW);
}
void _activateRelay(int idx, uint32_t duration)
{
    if (idx < 0 || idx > 3)
        return;

    unsigned long now = millis();
    relays[idx].isActive = true;
    relays[idx].startTime = now;
    relays[idx].duration = duration;
    setRelay(idx, true);

    if (duration > 0 && timerRelay[idx])
    {
        xTimerChangePeriod(timerRelay[idx], pdMS_TO_TICKS(duration), 0);
        xTimerStart(timerRelay[idx], 0);
    }
    else if (timerRelay[idx])
    {
        xTimerStop(timerRelay[idx], 0);
    }

    saveRelayStatesAsync();

    char reason[32];
    if (duration > 0)
        snprintf(reason, sizeof(reason), "timer %lus", duration / 1000);
    else
        strncpy(reason, "manual", sizeof(reason));
    sendRelayNotification(idx, true, reason);

    char logMsg[64];
    if (duration > 0)
        snprintf(logMsg, 64, "R%d ON for %lus", idx + 1, duration / 1000);
    else
        snprintf(logMsg, 64, "R%d ON (manual)", idx + 1);
    addLog(0, 0, logMsg);
}
void _deactivateRelay(int idx)
{
    if (idx < 0 || idx > 3)
        return;
    relays[idx].isActive = false;
    relays[idx].duration = 0;
    setRelay(idx, false);

    if (timerRelay[idx])
        xTimerStop(timerRelay[idx], 0);

    saveRelayStatesAsync();
    sendRelayNotification(idx, false, "");

    char logMsg[24];
    snprintf(logMsg, 24, "R%d OFF", idx + 1);
    addLog(0, 0, logMsg);
}

void _toggleRelay(int idx)
{
    if (relays[idx].isActive)
        _deactivateRelay(idx);
    else
        _activateRelay(idx, 0);
}

void activateRelayTS(int idx, uint32_t duration)
{
    RelayCommand cmd = {(uint8_t)idx, 1, duration};
    xQueueSend(qRelay, &cmd, pdMS_TO_TICKS(100));
}

void deactivateRelayTS(int idx)
{
    RelayCommand cmd = {(uint8_t)idx, 0, 0};
    xQueueSend(qRelay, &cmd, pdMS_TO_TICKS(100));
}

void toggleRelayTS(int idx)
{
    RelayCommand cmd = {(uint8_t)idx, 2, 0};
    xQueueSend(qRelay, &cmd, pdMS_TO_TICKS(100));
}
