#pragma once
#include <Arduino.h>

enum OTAStatus
{
    OTA_IDLE,
    OTA_CHECKING,
    OTA_DOWNLOADING,
    OTA_SUCCESS,
    OTA_UP_TO_DATE,
    OTA_FAILED
};

struct OTAState
{
    OTAStatus status = OTA_IDLE;
    String newVersion;
    String message;
    int progress = 0;
};

extern OTAState otaState;

void otaCheck();
void otaBegin();