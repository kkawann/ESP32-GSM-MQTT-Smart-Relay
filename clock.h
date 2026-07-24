#pragma once
#include <Arduino.h>

uint8_t getDayOfWeek();
bool isWeekdayAllowed(uint8_t mask);
void updateInternalClock();
bool getCurrentTime(uint8_t &h, uint8_t &m, uint8_t &s);
void startClockSync();
void processClockSync();