#pragma once
#include "types.h"
#include <Arduino.h>
void setRelay(int index, bool state);
void _activateRelay(int idx, uint32_t duration);
void _deactivateRelay(int idx);
void _toggleRelay(int idx);
void activateRelayTS(int idx, uint32_t duration);
void deactivateRelayTS(int idx);
void toggleRelayTS(int idx);
void sendRelayNotification(int index, bool isOn, const char *reason);