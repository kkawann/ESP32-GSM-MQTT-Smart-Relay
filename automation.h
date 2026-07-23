#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "types.h"
#include <Arduino.h>
void checkAutomations();
bool evalAutomationNoTime(const Automation &a);
bool evalCondition(const AutoCondition &c);
void executeAutoAction(const AutoAction &act);
void processPulseStates();
bool checkHysteresis(Automation &a);
bool isInTimeWindow(const Automation &a);
bool isInDayWindow(const Automation &a);

#endif