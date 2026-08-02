#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <Arduino.h>

// Timer callbacks
void cbRelayTimer(TimerHandle_t t);
void cbGSMCheck(TimerHandle_t t);
void cbClockSync(TimerHandle_t t);
void cbLED(TimerHandle_t t);

// FreeRTOS tasks
void taskWebServerFn(void *p);
void taskGSMFn(void *p);
void taskRFFn(void *p);
void taskRelayFn(void *p);
void taskSceneFn(void *p);
void taskSaveFn(void *p);
void taskMQTTFn(void *param);

// MQTT callbacks
void mqttMessageCallback(const char *topic, const uint8_t *payload, uint16_t len);
void mqttConnectCallback(bool connected);
