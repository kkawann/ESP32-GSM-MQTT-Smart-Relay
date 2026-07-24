#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Hardware pins (ESP32-C3 Super Mini)
// ---------------------------------------------------------------------------
#define RELAY1_PIN 5
#define RELAY2_PIN 6
#define RELAY3_PIN 9
#define RELAY4_PIN 10

#define RF_RX_PIN 4

#define SIM800_RST_PIN 7
#define STATUS_LED_PIN 1

// SIM800 on Serial1: RX=2, TX=3 (see main.cpp)
#define SIM800Serial Serial1

// ---------------------------------------------------------------------------
// Limits
// ---------------------------------------------------------------------------
#define MAX_RF_BUTTONS 30
#define MAX_RF_COMBOS 15
#define MAX_SCENES 20
#define MAX_RULES 20
#define MAX_AUTOMATIONS 15
#define MAX_RF_SENSORS 10
#define MAX_PHONES 10
#define MAX_EVENT_QUEUE 30
#define MAX_EVENT_LOGS 50

#define CURRENT_VERSION "1.0.0"
#define DEVICE_TYPE "esp32"
#define SERVER_IP "188.121.124.97"

// ---------------------------------------------------------------------------
// Timing constants (ms)
// ---------------------------------------------------------------------------
#define RF_DOUBLE_CLICK_WINDOW 400
#define RF_TRIPLE_CLICK_WINDOW 1000
#define RF_LONG_PRESS_TIME 1000
#define RF_COMBO_WINDOW 500
#define RF_SPAM_LIMIT 3
#define RF_LEARNING_TIMEOUT 20000
#define RF_BURST_WINDOW 120UL

#define GSM_CHECK_INTERVAL 10000
#define GSM_SOFT_RESET_TIMEOUT 30000
#define GSM_HARD_RESET_TIMEOUT 120000
#define SIM800_COMM_WATCHDOG 60000
#define SIM800_BOOT_GRACE 30000

#define CLOCK_SYNC_INTERVAL 3600000 // 1 hour
#define CSYNC_TIMEOUT 10000

// ---------------------------------------------------------------------------
// WiFi Access Point
// ---------------------------------------------------------------------------
#define AP_SSID "SmartRelay_Pro"
#define AP_PASS "12345678"

// ---------------------------------------------------------------------------
// Web authentication
// ---------------------------------------------------------------------------
#define ADMIN_USER "admin"
#define ADMIN_PASS "admin"

#endif
