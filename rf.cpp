#include <Arduino.h>
#include "rf.h"
#include "globals.h"
#include "scene.h"
#include "relay.h"
#include "utils.h"

// Detect RF remote spam / burst repeats so each press is handled once
bool isRFSpam(unsigned long code)
{
    unsigned long now = millis();
    int idx = -1;

    for (int i = 0; i < 5; i++)
    {
        if (rfHistory[i].code == code)
        {
            idx = i;
            break;
        }
    }

    // First time seeing this code
    if (idx < 0)
    {
        idx = rfHistoryIndex;
        rfHistory[idx].code = code;
        rfHistory[idx].count = 1;
        rfHistory[idx].timestamp = now;
        rfHistory[idx].firstSeen = now;
        rfHistoryIndex = (rfHistoryIndex + 1) % 5;
        return false;
    }

    unsigned long sinceFirst = now - rfHistory[idx].firstSeen;
    unsigned long sinceLast = now - rfHistory[idx].timestamp;

    rfHistory[idx].timestamp = now;

    // Burst: remotes often retransmit every ~10–50 ms
    if (sinceLast < RF_BURST_WINDOW)
    {
        rfHistory[idx].count++;
        return true; // drop burst duplicate
    }

    // New press (gap large enough)
    rfHistory[idx].count = 1;
    rfHistory[idx].firstSeen = now;
    return false;
}

int findButtonByCode(unsigned long code)
{
    for (int i = 0; i < rfButtonCount; i++)
    {
        if (rfButtons[i].active && rfButtons[i].code == code)
            return i;
    }
    return -1;
}

int findComboIndex(unsigned long code1, unsigned long code2)
{
    for (int i = 0; i < rfComboCount; i++)
    {
        if (!rfCombos[i].active)
            continue;
        if ((rfCombos[i].code1 == code1 && rfCombos[i].code2 == code2) ||
            (rfCombos[i].code1 == code2 && rfCombos[i].code2 == code1))
        {
            return i;
        }
    }
    return -1;
}

void handleRFEvent(int buttonIdx, RFEventType eventType)
{
    if (buttonIdx < 0)
        return;
    uint8_t action = 0;
    uint8_t target = 0;
    const char *eventName = "";

    switch (eventType)
    {
    case RF_SINGLE_CLICK:
        action = rfButtons[buttonIdx].singleAction;
        target = rfButtons[buttonIdx].singleTarget;
        eventName = "SINGLE";
        break;
    case RF_DOUBLE_CLICK:
        action = rfButtons[buttonIdx].doubleAction;
        target = rfButtons[buttonIdx].doubleTarget;
        eventName = "DOUBLE";
        break;
    case RF_TRIPLE_CLICK:
        action = rfButtons[buttonIdx].tripleAction;
        target = rfButtons[buttonIdx].tripleTarget;
        eventName = "TRIPLE";
        break;
    case RF_LONG_PRESS:
        action = rfButtons[buttonIdx].longAction;
        target = rfButtons[buttonIdx].longTarget;
        eventName = "LONG";
        break;
    }

    char logMsg[64];
    snprintf(logMsg, 64, "%s: %s", rfButtons[buttonIdx].name, eventName);
    addLog(2, 0, logMsg);

    if (action > 0)
        executeAction(action, target);
}

void handleRFCode(unsigned long code, uint8_t protocol, uint16_t bitLength)
{
    if (code == 0)
        return;

    // Learning mode: capture next code
    if (rfLearningMode)
    {
        rfLearnedCode = code;
        rfLearnedProtocol = protocol;
        rfLearnedBitLength = bitLength;
        rfCodeReady = true;
        rfLearningMode = false;
        lastRFCode = 0;
        lastRFTime = 0;
        rfClickCount = 0;
        longPressDetected = false;
        Serial.printf("[RF] LEARNED: 0x%08lX (P%d, %dbit)\n", code, protocol, bitLength);
        return;
    }

    if (isRFSpam(code))
        return;

    unsigned long now = millis();

    // Two-button combo within RF_COMBO_WINDOW
    if (lastRFCode != 0 && lastRFCode != code && (now - lastRFTime) < RF_COMBO_WINDOW)
    {
        int comboIdx = findComboIndex(lastRFCode, code);
        if (comboIdx >= 0)
        {
            Serial.printf("[RF] COMBO: %s\n", rfCombos[comboIdx].name);
            executeAction(rfCombos[comboIdx].actionType, rfCombos[comboIdx].actionId);
            lastRFCode = 0;
            return;
        }
    }

    // Known button: track multi-click
    int btnIdx = findButtonByCode(code);
    if (btnIdx < 0)
    {
        lastRFCode = code;
        lastRFTime = now;
        return;
    }

    if (code == lastRFCode && (now - lastRFTime) < RF_DOUBLE_CLICK_WINDOW)
    {
        rfClickCount++;
    }
    else
    {
        rfClickCount = 1;
        firstClickTime = now;
    }

    lastRFCode = code;
    lastRFTime = now;
    longPressDetected = false;

    if (rfClickCount >= 3 && (now - firstClickTime) < RF_TRIPLE_CLICK_WINDOW)
    {
        handleRFEvent(btnIdx, RF_TRIPLE_CLICK);
        rfClickCount = 0;
        lastRFCode = 0;
    }
}

void processRFClickDetection()
{
    if (lastRFCode == 0)
        return;
    unsigned long now = millis();
    unsigned long elapsed = now - lastRFTime;
    int btnIdx = findButtonByCode(lastRFCode);
    if (btnIdx < 0)
    {
        lastRFCode = 0;
        return;
    }
    if (!longPressDetected && elapsed >= RF_LONG_PRESS_TIME)
    {
        handleRFEvent(btnIdx, RF_LONG_PRESS);
        longPressDetected = true;
        lastRFCode = 0;
        rfClickCount = 0;
        return;
    }
    if (elapsed >= RF_DOUBLE_CLICK_WINDOW && rfClickCount == 2)
    {
        handleRFEvent(btnIdx, RF_DOUBLE_CLICK);
        lastRFCode = 0;
        rfClickCount = 0;
        return;
    }
    if (elapsed >= RF_DOUBLE_CLICK_WINDOW && rfClickCount == 1 && !longPressDetected)
    {
        handleRFEvent(btnIdx, RF_SINGLE_CLICK);
        lastRFCode = 0;
        rfClickCount = 0;
        return;
    }
}

void startRFLearning()
{
    lastRFCode = 0;
    lastRFTime = 0;
    rfClickCount = 0;
    longPressDetected = false;
    firstClickTime = 0;
    rfLearnedCode = 0;
    rfLearnedProtocol = 0;
    rfLearnedBitLength = 0;
    rfCodeReady = false;
    rfLearningStart = millis();
    rfLearningMode = true;
}

void stopRFLearning()
{
    rfLearningMode = false;
    rfLearnedCode = 0;
    rfCodeReady = false;
    lastRFCode = 0;
    lastRFTime = 0;
    rfClickCount = 0;
    longPressDetected = false;
}
int findSensorByCode(uint32_t code)
{
    for (int i = 0; i < rfSensorCount; i++)
    {
        if (!rfSensors[i].active)
            continue;
        if ((code & rfSensors[i].baseMask) == rfSensors[i].baseCode)
            return i;
    }
    return -1;
}

float applyEMA(float prev, float newVal, float alpha)
{
    if (alpha <= 0.0f || alpha >= 1.0f)
        return newVal;
    return alpha * newVal + (1.0f - alpha) * prev;
}
