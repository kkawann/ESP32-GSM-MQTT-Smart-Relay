#include <Arduino.h>
#include <WiFi.h>
#include "nvs_flash.h"
#include "globals.h"
#include "config.h"
#include "relay.h"
#include "tasks.h"
#include "gsm.h"
#include "clock.h"
#include "storage.h"
#include "webapi.h"
#include "html_page.h"
#include <HTTPClient.h>
#include <Update.h>
#include "ota.h"
#include "SIM800_MQTT.h"
#include "utils.h"
#include "esp_task_wdt.h"

void setup()
{
    // ── Watchdog FIRST — before anything can block the boot ──
    esp_task_wdt_deinit();
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 20000,
        .idle_core_mask = 0,
        .trigger_panic = true};
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL); // register setup()/loop() (runs on core 1)

    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n=== Smart Relay Pro v3.1 (with MQTT) ===");

    // ── NVS corruption recovery (protects against power-loss-during-write) ──
    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        Serial.println("[NVS] Corrupt/incompatible, erasing...");
        nvs_flash_erase();
        nvsErr = nvs_flash_init();
    }
    if (nvsErr != ESP_OK)
    {
        Serial.printf("[NVS] Init failed: %d (continuing anyway)\n", nvsErr);
    }
    esp_task_wdt_reset();

    // GPIO
    qSMSIn = xQueueCreate(10, sizeof(SMSInMessage));
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(RELAY4_PIN, OUTPUT);
    pinMode(SIM800_RST_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    sim800.last_command_success_time = millis();
    digitalWrite(SIM800_RST_PIN, HIGH);
    digitalWrite(STATUS_LED_PIN, LOW);

    for (int i = 0; i < 4; i++)
    {
        relays[i].isActive = false;
        relays[i].duration = 0;
        relays[i].logic = false;
        relays[i].notifySMS = false;
        setRelay(i, false);
    }
    esp_task_wdt_reset();

    // WiFi SoftAP
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PASS);
    wifiReady = true;
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    esp_task_wdt_reset();

    // Load settings from NVS (each wrapped + watchdog reset in case one is corrupt/slow)
    loadAllSettings();
    esp_task_wdt_reset();
    loadSensors();
    esp_task_wdt_reset();
    loadAutomations();
    esp_task_wdt_reset();
    loadRelayStates();
    esp_task_wdt_reset();
    loadMQTTSettings();
    esp_task_wdt_reset();

    // 433 MHz RF receiver
    rcSwitch.enableReceive(digitalPinToInterrupt(RF_RX_PIN));
    Serial.printf("RF RX on pin %d\n", RF_RX_PIN);

    // SIM800C (Serial1: RX=2, TX=3)
    Serial1.begin(9600, SERIAL_8N1, 2, 3);
    SIM800_Init(&sim800, &Serial1);
    SIM800_SetSmsCallback(&sim800, smsCallback);
    SIM800_SetInitCallback(&sim800, initCallback);
    esp_task_wdt_reset();

    lastSIM800Activity = millis();
    gsmBooting = true;
    gsmBootStarted = millis();
    Serial.println("SIM800 initialized on Serial1 (RX=2, TX=3)");

    // MQTT over GPRS
    MQTT_ClientInit(&mqttClient, &sim800);

    if (mqttEnabled && strlen(mqttBroker) > 0)
    {
        MQTT_SetBroker(&mqttClient, mqttBroker, mqttPort);
        MQTT_SetAuth(&mqttClient, mqttClientId, mqttUser, mqttPass);
        MQTT_SetAPN(&mqttClient, mqttAPN, mqttAPNUser, mqttAPNPass);
        MQTT_SetConnectCallback(&mqttClient, mqttConnectCallback);
        MQTT_SetMessageCallback(&mqttClient, mqttMessageCallback);

        Serial.printf("[MQTT] Configured: %s:%d (enabled=%d)\n",
                      mqttBroker, mqttPort, mqttEnabled);
    }
    else
    {
        Serial.println("[MQTT] Disabled or not configured");
    }
    esp_task_wdt_reset();

    // FreeRTOS: mutexes
    mutexRelay = xSemaphoreCreateMutex();
    mutexClock = xSemaphoreCreateMutex();
    mutexSerial = xSemaphoreCreateMutex();
    xSensorMutex = xSemaphoreCreateMutex();

    // FreeRTOS: queues
    qRelay = xQueueCreate(10, sizeof(RelayCommand));
    qSave = xQueueCreate(5, sizeof(SaveCommand));
    qSMSOut = xQueueCreate(10, sizeof(SMSOutMessage));

    // FreeRTOS: timers
    for (int i = 0; i < 4; i++)
    {
        timerRelay[i] = xTimerCreate(
            ("relay" + String(i)).c_str(),
            pdMS_TO_TICKS(1000),
            pdFALSE,
            (void *)(uintptr_t)i,
            cbRelayTimer);
    }

    timerGSMCheck = xTimerCreate(
        "GSMCheck",
        pdMS_TO_TICKS(GSM_CHECK_INTERVAL),
        pdTRUE,
        NULL,
        cbGSMCheck);

    timerClockSync = xTimerCreate(
        "ClockSync",
        pdMS_TO_TICKS(CLOCK_SYNC_INTERVAL),
        pdTRUE,
        NULL,
        cbClockSync);

    timerLED = xTimerCreate(
        "LED",
        pdMS_TO_TICKS(200),
        pdTRUE,
        NULL,
        cbLED);

    xTimerStart(timerGSMCheck, 0);
    xTimerStart(timerClockSync, 0);
    xTimerStart(timerLED, 0);
    esp_task_wdt_reset();

    // FreeRTOS: tasks
    xTaskCreatePinnedToCore(taskGSMFn, "GSM", 8192, NULL, 2, &taskGSM, 0);
    xTaskCreatePinnedToCore(taskRFFn, "RF", 4096, NULL, 3, &taskRF, 0);
    xTaskCreatePinnedToCore(taskSceneFn, "Scene", 4096, NULL, 1, &taskScene, 0);
    xTaskCreatePinnedToCore(taskSaveFn, "Save", 8192, NULL, 1, &taskSave, 0);
    xTaskCreatePinnedToCore(taskWebServerFn, "WebServer", 8192, NULL, 1, &taskWebServer, 0);

    if (mqttEnabled)
    {
        xTaskCreatePinnedToCore(taskMQTTFn, "MQTT", 6144, NULL, 2, &taskMQTT, 0);
        Serial.println("[MQTT] Task created on Core 0");
    }
    xTaskCreatePinnedToCore(taskRelayFn, "Relay", 4096, NULL, 4, NULL, 0);
    esp_task_wdt_reset();

    // HTTP routes
    server.on("/", handleRoot);
    server.on("/api/status", handleAPIStatus);
    server.on("/api/relay", HTTP_POST, handleAPIRelay);

    server.on("/api/rf/learn", HTTP_POST, handleAPIRFLearn);
    server.on("/api/rf/learned", handleAPIRFLearned);
    server.on("/api/rf/cancel", HTTP_POST, handleAPIRFCancel);
    server.on("/api/rf/save", HTTP_POST, handleAPIRFSave);
    server.on("/api/rf/buttons", handleAPIRFButtons);
    server.on("/api/rf/delete", HTTP_POST, handleAPIRFDelete);

    server.on("/api/combos", handleAPICombos);
    server.on("/api/combo/save", HTTP_POST, handleAPIComboSave);
    server.on("/api/combo/delete", HTTP_POST, handleAPIComboDelete);

    server.on("/api/scenes", handleAPIScenes);
    server.on("/api/scene/save", HTTP_POST, handleAPISceneSave);
    server.on("/api/scene/run", HTTP_POST, handleAPISceneRun);
    server.on("/api/scene/delete", HTTP_POST, handleAPISceneDelete);

    server.on("/api/relay/settings", handleAPIRelaySettings);
    server.on("/api/relay/settings/save", HTTP_POST, handleAPIRelaySettingsSave);

    server.on("/api/reset/soft", HTTP_POST, handleAPIResetSoft);
    server.on("/api/reset/hard", HTTP_POST, handleAPIResetHard);
    server.on("/api/clear", HTTP_POST, handleAPIClear);

    server.on("/api/phones", handleAPIPhones);
    server.on("/api/phone/save", HTTP_POST, handleAPIPhoneSave);
    server.on("/api/phone/delete", HTTP_POST, handleAPIPhoneDelete);

    server.on("/api/logs", handleAPILogs);
    server.on("/api/logs/clear", HTTP_POST, handleAPILogsClear);

    server.on("/api/sensors", handleAPISensors);
    server.on("/api/sensors/save", HTTP_POST, handleAPISensorSave);
    server.on("/api/sensors/delete", HTTP_POST, handleAPISensorDelete);
    server.on("/api/sensors/learn/start", HTTP_POST, handleAPISensorLearnStart);
    server.on("/api/sensors/learn/status", handleAPISensorLearnStatus);
    server.on("/api/sensors/learn/cancel", HTTP_POST, handleAPISensorLearnCancel);
    server.on("/api/sensors/values", handleAPISensorValues);

    server.on("/api/automations", handleAPIAutomations);
    server.on("/api/automations/save", HTTP_POST, handleAPIAutomationSave);
    server.on("/api/automations/delete", HTTP_POST, handleAPIAutomationDelete);
    server.on("/api/automations/toggle", HTTP_POST, handleAPIAutomationToggle);
    server.on("/api/automations/test", HTTP_POST, handleAPIAutomationTest);

    server.on("/api/wifi/connect", HTTP_POST, handleAPIWifiConnect);
    server.on("/api/wifi/status", handleAPIWifiStatus);
    server.on("/api/ota/check", HTTP_POST, handleAPIOTACheck);
    server.on("/api/ota/status", handleAPIOTAStatus);

    server.on("/api/mqtt/status", handleAPIMQTTStatus);
    server.on("/api/mqtt/settings", HTTP_POST, handleAPIMQTTSettings);
    server.on("/api/mqtt/connect", HTTP_POST, handleAPIMQTTConnect);
    server.on("/api/mqtt/disconnect", HTTP_POST, handleAPIMQTTDisconnect);

    server.begin();
    esp_task_wdt_reset();

    // setup()/loop() no longer needs individual WDT monitoring once tasks are up;
    // each FreeRTOS task registers/resets its own watchdog inside tasks.cpp.
    esp_task_wdt_delete(NULL);

    Serial.println("System Ready!");
}

void loop()
{
    delay(1000);
}