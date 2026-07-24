#ifndef GSM_H
#define GSM_H

#include <Arduino.h>
#include "SIM800_Arduino.h"
#include "SIM800_MQTT.h"

struct SMSInMessage
{
    char sender[20];
    char text[160];
};

void smsCallback(const char *text, const char *sender);
void initCallback(bool success);
void sendRelayNotification(int index, bool isOn, const char *reason);
void handleGSMFailure();
void processSMSCommand(String cmd, const char *sender);
bool isNumberAllowed(const char *number);
unsigned long parseTime(String timeStr);
String getStatus();
String getFullStatus();
void sendSMSAsync(const char *number, const char *text);
void sendLongSMS(const char *number, String text);
String normalizeNumber(String n);

#endif
