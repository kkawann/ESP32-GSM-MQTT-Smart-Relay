#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

void saveRelayStatesAsync();
void saveRelayStates();
void loadRelayStates();

void saveAllSettings();
void saveAllSettingsAsync();
void loadAllSettings();

void saveSensors();
void saveSensorsAsync();
void loadSensors();

void saveMQTTSettings();
void loadMQTTSettings();

void saveAutomations();
void saveAutomationsAsync();
void loadAutomations();

#endif
