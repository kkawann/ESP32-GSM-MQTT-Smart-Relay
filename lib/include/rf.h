#pragma once

#include <Arduino.h>
#include "types.h"

bool isRFSpam(unsigned long code);
int findButtonByCode(unsigned long code);
int findButtonByCodeAndProto(unsigned long code, uint8_t protocol);
int findComboIndex(unsigned long code1, unsigned long code2);
void handleRFEvent(int buttonIdx, RFEventType eventType);
void handleRFCode(unsigned long code, uint8_t protocol, uint16_t bitLength);
void processRFClickDetection();
void startRFLearning();
void stopRFLearning();
