#pragma once

#include <Arduino.h>
#include "rf.h"

uint32_t extractRawValue(uint32_t code, uint32_t valueMask);
float extractValue(const RFSensor &s, uint32_t code);
bool sensorMatchesCode(const RFSensor &s, uint32_t code, uint8_t proto, uint16_t bits);
bool processSensorCode(uint32_t code, uint8_t protocol, uint16_t bitLen);
void addToSensorHistory(int idx, float value);
int findSensorByCode(uint32_t code);
float applyEMA(float prev, float newVal, float alpha);
void saveSensorsAsync();

// ── Auto-type signature: pack/unpack SensorTypeId in baseMask upper 4 bits ──
void packSensorTypeIntoMask(RFSensor &s, uint8_t sensorTypeId);
uint8_t getSensorTypeFromMask(uint32_t mask);

void handleAPISensors();
void handleAPISensorSave();
void handleAPISensorDelete();
void handleAPISensorLearnStart();
void handleAPISensorLearnStatus();
void handleAPISensorLearnCancel();
void handleAPISensorValues();
