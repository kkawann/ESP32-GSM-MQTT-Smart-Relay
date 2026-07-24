#pragma once

#include <Arduino.h>
#include "types.h"

void addToQueue(uint8_t type, uint8_t id, uint8_t action, unsigned long delayMs, uint32_t duration = 0);
void processEventQueue();
void executeAction(uint8_t actionType, uint8_t actionId);
void executeScene(int sceneId);
void processSequentialScene();
void checkScheduledScenes();
