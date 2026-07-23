#include "tasks.h"
#include "globals.h"
#include "relay.h"
#include "gsm.h"
#include "clock.h"
#include "rf.h"
#include "sensor.h"
#include "scene.h"
#include "automation.h"
#include "storage.h"
#include "utils.h"
#include <WebServer.h>
#include "esp_task_wdt.h"
#include "SIM800_MQTT.h"

// Timer callbacks
void cbRelayTimer(TimerHandle_t t)
{
    uint32_t idx = (uint32_t)pvTimerGetTimerID(t);
    RelayCommand cmd = {(uint8_t)idx, 0, 0};
    xQueueSend(qRelay, &cmd, 0);
}

void cbGSMCheck(TimerHandle_t t)
{
    if (taskGSM)
        xTaskNotify(taskGSM, 0x01, eSetBits);
}

void cbClockSync(TimerHandle_t t)
{
    if (taskGSM)
        xTaskNotify(taskGSM, 0x02, eSetBits);
}

void cbLED(TimerHandle_t t)
{
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
}
void mqttMessageCallback(const char *topic, const uint8_t *payload, uint16_t len)
{
    Serial.print("[MQTT RX] Topic: ");
    Serial.println(topic);

    Serial.print("   Payload: ");
    Serial.write(payload, len);
    Serial.println();

    char message[256];
    if (len < sizeof(message))
    {
        memcpy(message, payload, len);
        message[len] = '\0';
    }
    else
    {
        Serial.println("[MQTT] Payload too large");
        return;
    }

    if (strcmp(topic, mqttTopicCommand) == 0)
    {
        if (strstr(message, "R1:ON"))
        {
            RelayCommand cmd = {0, 1, 0};
            xQueueSend(qRelay, &cmd, 0);
            Serial.println("[MQTT] Relay 1 ON");
        }
        else if (strstr(message, "R1:OFF"))
        {
            RelayCommand cmd = {0, 0, 0};
            xQueueSend(qRelay, &cmd, 0);
            Serial.println("[MQTT] Relay 1 OFF");
        }
        else if (strstr(message, "R2:ON"))
        {
            RelayCommand cmd = {1, 1, 0};
            xQueueSend(qRelay, &cmd, 0);
        }
        else if (strstr(message, "R2:OFF"))
        {
            RelayCommand cmd = {1, 0, 0};
            xQueueSend(qRelay, &cmd, 0);
        }
        else if (strstr(message, "R3:ON"))
        {
            RelayCommand cmd = {2, 1, 0};
            xQueueSend(qRelay, &cmd, 0);
        }
        else if (strstr(message, "R3:OFF"))
        {
            RelayCommand cmd = {2, 0, 0};
            xQueueSend(qRelay, &cmd, 0);
        }
        else if (strstr(message, "R4:ON"))
        {
            RelayCommand cmd = {3, 1, 0};
            xQueueSend(qRelay, &cmd, 0);
        }
        else if (strstr(message, "R4:OFF"))
        {
            RelayCommand cmd = {3, 0, 0};
            xQueueSend(qRelay, &cmd, 0);
        }
        else if (strstr(message, "STATUS"))
        {
            if (taskMQTT)
            {
                xTaskNotify(taskMQTT, 0x01, eSetBits);
            }
        }
        else if (strstr(message, "RESTART"))
        {
            Serial.println("[MQTT] Remote restart requested");
            addLog(4, 2, "MQTT restart cmd");
            delay(500);
            ESP.restart();
        }
    }
    else if (strstr(topic, "/relay/") != NULL)
    {
        char *relayNumStr = strrchr(topic, '/') + 1;
        int relayNum = atoi(relayNumStr);

        if (relayNum >= 1 && relayNum <= 4)
        {
            bool turnOn = (strcmp(message, "ON") == 0 || strcmp(message, "1") == 0);
            RelayCommand cmd = {(uint8_t)(relayNum - 1), (uint8_t)(turnOn ? 1 : 0), 0};
            xQueueSend(qRelay, &cmd, 0);
            Serial.printf("[MQTT] Relay %d %s\n", relayNum, turnOn ? "ON" : "OFF");
        }
    }

    char logMsg[128];
    snprintf(logMsg, sizeof(logMsg), "MQTT: %.30s", message);
    addLog(5, 0, logMsg);
}

void mqttConnectCallback(bool connected)
{
    if (connected)
    {
        Serial.println("[MQTT] Connected to broker");
        addLog(5, 0, "MQTT connected");

        MQTT_Subscribe(&mqttClient, mqttTopicCommand);
        Serial.printf("[MQTT] Subscribed: %s\n", mqttTopicCommand);

        for (int i = 1; i <= 4; i++)
        {
            char relayTopic[80];
            snprintf(relayTopic, sizeof(relayTopic), "%s/%d", mqttTopicRelay, i);
            MQTT_Subscribe(&mqttClient, relayTopic);
            Serial.printf("[MQTT] Subscribed: %s\n", relayTopic);
        }

        MQTT_PublishString(&mqttClient, mqttTopicLog, "Device online", true);
    }
    else
    {
        Serial.println("[MQTT] Disconnected");
        addLog(5, 1, "MQTT disconnected");
    }
}

void taskMQTTFn(void *param)
{
    Serial.println("[MQTT Task] Started");
    esp_task_wdt_add(NULL);

    MQTT_SetMessageCallback(&mqttClient, mqttMessageCallback);
    MQTT_SetConnectCallback(&mqttClient, mqttConnectCallback);

    MQTT_SetAPN(&mqttClient, "internet", "", "");
    MQTT_SetBroker(&mqttClient, mqttBroker, mqttPort);

    if (strlen(mqttUser) > 0)
    {
        MQTT_SetAuth(&mqttClient, mqttClientId, mqttUser, mqttPass);
    }
    else
    {
        MQTT_SetAuth(&mqttClient, mqttClientId, "", "");
    }

    Serial.printf("[MQTT] Broker: %s:%d\n", mqttBroker, mqttPort);
    Serial.printf("[MQTT] Client ID: %s\n", mqttClientId);

    TickType_t lastPublish = 0;
    const TickType_t publishInterval = pdMS_TO_TICKS(30000);
    uint32_t notifyVal = 0;

    while (true)
    {
        esp_task_wdt_reset();
        MQTT_Process(&mqttClient);

        if (MQTT_IsConnected(&mqttClient))
        {
            TickType_t now = xTaskGetTickCount();

            if (now - lastPublish >= publishInterval)
            {
                lastPublish = now;

                char status[300];
                snprintf(status, sizeof(status),
                         "{\"relays\":[%d,%d,%d,%d],\"rssi\":%d,\"heap\":%u,\"uptime\":%lu,\"network\":%d}",
                         relays[0].isActive,
                         relays[1].isActive,
                         relays[2].isActive,
                         relays[3].isActive,
                         SIM800_GetSignalStrength(&sim800),
                         ESP.getFreeHeap(),
                         millis() / 1000,
                         networkReady ? 1 : 0);

                if (MQTT_PublishString(&mqttClient, mqttTopicStatus, status, false))
                {
                    Serial.println("📤 [MQTT] Status published");
                }
            }
        }

        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, 0) == pdTRUE)
        {
            if (notifyVal & 0x01)
            {
                lastPublish = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ==================== Task: WebServer ====================
void taskWebServerFn(void *p)
{
    esp_task_wdt_add(NULL);
    for (;;)
    {
        esp_task_wdt_reset();
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// ==================== Task: GSM ====================
// ==================== Task: GSM ====================
// ==================== Task: GSM ====================
// ==================== Task: GSM ====================
void taskGSMFn(void *p)
{
    esp_task_wdt_add(NULL);
    uint32_t notifyVal = 0;
    unsigned long lastHealthCheck = 0;

    bool mqttAutoConnectDone = false;
    unsigned long mqttConnectAttemptTime = 0;

    for (;;)
    {
        esp_task_wdt_reset();
        unsigned long now = millis();

        // 1. پردازش دیتای سریال
        if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            if (clockSyncState != CSYNC_IDLE)
                processClockSync();
            else
                SIM800_Process(&sim800);
            xSemaphoreGive(mutexSerial);
        }

        // 2. مدیریت اتصال MQTT
        if (mqttEnabled && !mqttAutoConnectDone && !hwResetInProgress &&
            SIM800_IsReady(&sim800) && SIM800_IsNetworkRegistered(&sim800))
        {
            if (mqttConnectAttemptTime == 0)
            {
                mqttConnectAttemptTime = now;
                Serial.println("[MQTT] SIM ready, will connect in 5s...");
            }
            else if ((now - mqttConnectAttemptTime) >= 5000)
            {
                mqttAutoConnectDone = true;
                Serial.println("[MQTT] Starting connection...");
                MQTT_Connect(&mqttClient);
                addLog(4, 0, "MQTT auto-connect started");
            }
        }

        if (mqttAutoConnectDone && !networkReady)
        {
            mqttAutoConnectDone = false;
            mqttConnectAttemptTime = 0;
            Serial.println("[MQTT] Network lost, will retry when available");
        }

        // 3. ارسال پیامک‌های خروجی (فقط وقتی شبکه وصل است)
        if (sim800Ready && networkReady && !hwResetInProgress)
        {
            SMSOutMessage sms;
            if (xQueueReceive(qSMSOut, &sms, 0) == pdTRUE)
            {
                if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(500)) == pdTRUE)
                {
                    SIM800_SendSMS(&sim800, sms.number, sms.text);
                    lastSIM800Activity = millis();
                    xSemaphoreGive(mutexSerial);
                }
                vTaskDelay(pdMS_TO_TICKS(300));
            }
        }
        else
        {
            // شبکه قطع است - صف ارسال را خالی کن
            SMSOutMessage discard;
            uint8_t drained = 0;
            while (xQueueReceive(qSMSOut, &discard, 0) == pdTRUE && ++drained < 10)
                ;
            if (drained > 0)
            {
                char msg[48];
                snprintf(msg, 48, "Dropped %d SMS (SIM down)", drained);
                addLog(3, 1, msg);
            }
        }

        // Process inbound SMS commands (works even during network init)
        {
            SMSInMessage incomingSMS;
            if (xQueueReceive(qSMSIn, &incomingSMS, 0) == pdTRUE)
            {
                Serial.printf("[SMS CMD] Processing msg from: %s\n", incomingSMS.sender);
                if (isNumberAllowed(incomingSMS.sender))
                {
                    processSMSCommand(incomingSMS.text, incomingSMS.sender);
                }
                else
                {
                    Serial.println("[SMS] Unauthorized number ignored");
                    addLog(3, 1, "Unauthorized SMS blocked");
                }
            }
        }

        // Clock sync after SIM init
        if (pendingTimeSync && networkReady && !hwResetInProgress)
        {
            if ((now - simInitDoneAt) >= 5000UL)
            {
                pendingTimeSync = false;
                if (clockSyncState == CSYNC_IDLE)
                {
                    if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        startClockSync();
                        xSemaphoreGive(mutexSerial);
                    }
                }
            }
        }

        // 6. Health Check
        if (!hwResetInProgress && !gsmBooting &&
            (now - lastHealthCheck) >= GSM_CHECK_INTERVAL)
        {
            lastHealthCheck = now;

            if (sim800Ready && sim800.initialized && (now - simInitDoneAt) < 15000)
            {
                // Grace Period - 15 ثانیه بعد از init صبر کن
                networkReady = true;
                gsmFailureDetected = false;
                gsmFailureStart = 0;
            }
            else
            {
                int sig = SIM800_GetSignalStrength(&sim800);
                int state = SIM800_GetInitState(&sim800);
                bool reg = SIM800_IsNetworkRegistered(&sim800);

                sim800Ready = (state == INIT_COMPLETE);
                networkReady = sim800Ready && reg;

                bool commAlive = (lastSIM800Activity == 0) ||
                                 ((now - lastSIM800Activity) < SIM800_COMM_WATCHDOG);

                bool healthy = networkReady && (sig > 0 && sig < 99) && commAlive;

                if (!healthy)
                {
                    if (!gsmFailureDetected)
                    {
                        gsmFailureDetected = true;
                        gsmFailureStart = now;
                        softResetDone = false;
                        char msg[64];
                        snprintf(msg, 64, "GSM fail: rdy=%d net=%d sig=%d comm=%d",
                                 sim800Ready, networkReady, sig, (int)commAlive);
                        addLog(4, 1, msg);
                    }
                    else
                    {
                        unsigned long dur = now - gsmFailureStart;

                        if (dur >= GSM_HARD_RESET_TIMEOUT && !hwResetInProgress)
                        {
                            addLog(4, 2, "GSM hard reset triggered");
                            hwResetInProgress = true;
                            hwResetStage = 0;
                            gsmFailureDetected = false;
                            softResetDone = false;
                            hardwareResetCount++;

                            if (MQTT_IsConnected(&mqttClient))
                                MQTT_Disconnect(&mqttClient);

                            mqttAutoConnectDone = false;
                            mqttConnectAttemptTime = 0;

                            digitalWrite(SIM800_RST_PIN, LOW);
                            xTaskNotify(taskGSM, 0x04, eSetBits);
                        }
                        else if (dur >= GSM_SOFT_RESET_TIMEOUT && !softResetDone)
                        {
                            addLog(4, 1, "GSM soft reset");
                            if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(200)) == pdTRUE)
                            {
                                SIM800_ForceReinit(&sim800);
                                xSemaphoreGive(mutexSerial);
                            }
                            sim800Ready = false;
                            networkReady = false;
                            softResetDone = true;
                            softwareResetCount++;
                            lastSIM800Activity = millis();
                            mqttAutoConnectDone = false;
                            mqttConnectAttemptTime = 0;
                        }
                    }
                }
                else
                {
                    if (gsmFailureDetected)
                    {
                        gsmFailureDetected = false;
                        gsmFailureStart = 0;
                        softResetDone = false;
                        addLog(4, 0, "GSM recovered");
                    }
                }
            }

            if (timerLED)
                xTimerChangePeriod(timerLED, pdMS_TO_TICKS(sim800Ready ? 1000 : 200), 0);
        }

        // 7. پایان زمان بوت
        if (gsmBooting && (now - gsmBootStarted) >= SIM800_BOOT_GRACE)
        {
            gsmBooting = false;
            lastHealthCheck = now;
            lastSIM800Activity = now;
            addLog(4, 0, "SIM800 boot grace done, monitoring...");
        }

        // 8. دریافت سیگنال‌های تسک
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notifyVal, 0) == pdTRUE)
        {
            if (notifyVal & 0x02)
            {
                if (clockSyncState == CSYNC_IDLE && networkReady && !hwResetInProgress)
                {
                    if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        startClockSync();
                        xSemaphoreGive(mutexSerial);
                    }
                }
            }

            if (notifyVal & 0x04)
            {
                addLog(4, 1, "SIM800 RST LOW, holding 300ms...");
                vTaskDelay(pdMS_TO_TICKS(300));

                digitalWrite(SIM800_RST_PIN, HIGH);
                addLog(4, 1, "SIM800 RST HIGH, waiting boot...");
                vTaskDelay(pdMS_TO_TICKS(5000));

                if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(500)) == pdTRUE)
                {
                    SIM800_ForceReinit(&sim800);
                    xSemaphoreGive(mutexSerial);
                }

                sim800Ready = false;
                networkReady = false;
                hwResetInProgress = false;
                hwResetStage = 0;
                internalClock.isValid = false;
                pendingTimeSync = true;
                simInitDoneAt = millis();
                gsmBooting = true;
                gsmBootStarted = millis();
                mqttAutoConnectDone = false;
                mqttConnectAttemptTime = 0;

                addLog(4, 1, "SIM800 hard reset complete, grace period started");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
// ==================== Task: RF ====================
void taskRFFn(void *p)
{
    esp_task_wdt_add(NULL);
    for (;;)
    {
        esp_task_wdt_reset();

        if (rcSwitch.available())
        {
            unsigned long code = rcSwitch.getReceivedValue();
            uint8_t proto = (uint8_t)rcSwitch.getReceivedProtocol();
            uint16_t bits = rcSwitch.getReceivedBitlength();
            rcSwitch.resetAvailable();

            if (code != 0)
            {
                if (rfLearningMode)
                {
                    rfLearnedCode = code;
                    rfLearnedProtocol = proto;
                    rfLearnedBitLength = bits;
                    rfCodeReady = true;
                    rfLearningMode = false;
                    lastRFCode = 0;
                    lastRFTime = 0;
                    rfClickCount = 0;
                    longPressDetected = false;
                    Serial.printf("[RF] LEARNED BUTTON: 0x%08lX\n", code);
                    goto rf_done;
                }

                if (s_learnWaiting)
                {
                    s_learnedCode = code;
                    s_learnedProtocol = proto;
                    s_learnedBits = bits;
                    s_learnWaiting = false;
                    Serial.printf("[RF] LEARNED SENSOR: 0x%08lX\n", code);
                    goto rf_done;
                }

                if (processSensorCode((uint32_t)code, proto, bits))
                    goto rf_done;

                handleRFCode(code, proto, bits);

            rf_done:;
            }
        }

        if (!rfLearningMode)
        {
            processRFClickDetection();
        }

        if (rfLearningMode &&
            (millis() - rfLearningStart) >= RF_LEARNING_TIMEOUT)
        {
            rfLearningMode = false;
            rfCodeReady = false;
            lastRFCode = 0;
            rfClickCount = 0;
            longPressDetected = false;
        }

        RelayCommand cmd;
        while (xQueueReceive(qRelay, &cmd, 0) == pdTRUE)
        {
            if (xSemaphoreTake(mutexRelay, pdMS_TO_TICKS(20)) == pdTRUE)
            {
                switch (cmd.mode)
                {
                case 0:
                    _deactivateRelay(cmd.index);
                    break;
                case 1:
                    _activateRelay(cmd.index, cmd.duration);
                    break;
                case 2:
                    _toggleRelay(cmd.index);
                    break;
                }
                xSemaphoreGive(mutexRelay);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ==================== Task: Scene ====================
void taskSceneFn(void *p)
{
    esp_task_wdt_add(NULL);
    TickType_t lastSchedule = xTaskGetTickCount();
    TickType_t lastAutomation = xTaskGetTickCount();

    for (;;)
    {
        esp_task_wdt_reset();
        TickType_t now = xTaskGetTickCount();

        processEventQueue();
        processSequentialScene();
        processPulseStates();

        if ((now - lastSchedule) >= pdMS_TO_TICKS(1000))
        {
            lastSchedule = now;
            if (internalClock.isValid)
                checkScheduledScenes();
            updateInternalClock();
        }

        if ((now - lastAutomation) >= pdMS_TO_TICKS(2000))
        {
            lastAutomation = now;
            if (automationCount > 0)
                checkAutomations();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void taskSaveFn(void *p)
{
    esp_task_wdt_add(NULL);
    SaveCommand cmd;

    Serial.println("[Save Task] Storage worker initialized");

    for (;;)
    {
        esp_task_wdt_reset();

        if (xQueueReceive(qSave, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            Serial.printf("[Save] Type=%d execution start\n", cmd.type);

            switch (cmd.type)
            {
            case 0:
                saveAllSettings();
                break;
            case 1:
                saveRelayStates();
                break;
            case 2:
                saveSensors();
                break;
            case 3:
                saveAutomations();
                break;
            case 4:
                saveMQTTSettings();
                break;
            default:
                Serial.printf("[Save] Unknown command type: %d\n", cmd.type);
                break;
            }

            Serial.printf("[Save] Type=%d operation successful\n", cmd.type);

            // ریست مجدد واچ‌داگ بلافاصله پس از اتمام عملیات سنگین فلش
            esp_task_wdt_reset();

            // یک تنفس کوتاه به سیستم برای جابجایی تسک‌ها در صورت وجود دستورات متوالی در صف
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        // وقتی صف خالی است، تابع xQueueReceive خودش ۱ ثانیه تسک را بلاک کرده و CPU را آزاد نگه می‌دارد.
    }
}