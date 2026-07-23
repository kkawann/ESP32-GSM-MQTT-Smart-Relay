#ifndef SIM800_MQTT_H
#define SIM800_MQTT_H

#include <Arduino.h>
#include "SIM800_Arduino.h"

// ============================================================================
// Configuration
// ============================================================================
#define MQTT_MAX_PACKET_SIZE 256
#define MQTT_KEEPALIVE 60
#define MQTT_SOCKET_TIMEOUT 15
#define MQTT_MAX_TOPICS 4

// ============================================================================
// MQTT Packet Types
// ============================================================================
#define MQTT_CONNECT 0x10
#define MQTT_CONNACK 0x20
#define MQTT_PUBLISH 0x30
#define MQTT_PUBACK 0x40
#define MQTT_SUBSCRIBE 0x82
#define MQTT_SUBACK 0x90
#define MQTT_PINGREQ 0xC0
#define MQTT_PINGRESP 0xD0
#define MQTT_DISCONNECT 0xE0

// ============================================================================
// MQTT States
// ============================================================================
typedef enum
{
    MQTT_IDLE,
    MQTT_GPRS_CONNECTING,
    MQTT_GPRS_CONNECTED,
    MQTT_TCP_CONNECTING,
    MQTT_TCP_CONNECTED,
    MQTT_SENDING_CONNECT,
    MQTT_WAIT_CONNACK,
    MQTT_CONNECTED,
    MQTT_SUBSCRIBING,
    MQTT_PUBLISHING,
    MQTT_DISCONNECTING,
    MQTT_ERROR
} MqttState_t;

// ============================================================================
// Callbacks
// ============================================================================
typedef void (*MqttConnectCallback_t)(bool success);
typedef void (*MqttMessageCallback_t)(const char *topic, const uint8_t *payload, uint16_t len);

// ============================================================================
// MQTT Client Structure
// ============================================================================
typedef struct
{
    SIM800_t *sim;

    // Connection
    char broker[64];
    uint16_t port;
    char client_id[32];
    char username[32];
    char password[32];

    // State
    MqttState_t state;
    uint32_t last_activity;
    uint32_t last_ping;
    uint16_t keepalive;

    // Subscriptions
    char topics[MQTT_MAX_TOPICS][64];
    uint8_t topic_count;

    // TX/RX Buffers
    uint8_t tx_buffer[MQTT_MAX_PACKET_SIZE];
    uint16_t tx_len;

    uint8_t rx_buffer[MQTT_MAX_PACKET_SIZE];
    uint16_t rx_len;
    uint16_t rx_expected_len;
    bool rx_collecting;

    // Callbacks
    MqttConnectCallback_t connect_callback;
    MqttMessageCallback_t message_callback;

    // Config
    char apn[32];
    char apn_user[32];
    char apn_pass[32];

} MqttClient_t;

// ============================================================================
// Public API
// ============================================================================
void MQTT_ClientInit(MqttClient_t *mqtt, SIM800_t *sim);
void MQTT_SetBroker(MqttClient_t *mqtt, const char *host, uint16_t port);
void MQTT_SetAuth(MqttClient_t *mqtt, const char *client_id, const char *user, const char *pass);
void MQTT_SetAPN(MqttClient_t *mqtt, const char *apn, const char *user, const char *pass);

void MQTT_Connect(MqttClient_t *mqtt);
void MQTT_Disconnect(MqttClient_t *mqtt);
void MQTT_Process(MqttClient_t *mqtt);

bool MQTT_IsConnected(MqttClient_t *mqtt);
MqttState_t MQTT_GetState(MqttClient_t *mqtt);

bool MQTT_Subscribe(MqttClient_t *mqtt, const char *topic);
bool MQTT_Publish(MqttClient_t *mqtt, const char *topic, const uint8_t *payload, uint16_t len, bool retain);
bool MQTT_PublishString(MqttClient_t *mqtt, const char *topic, const char *payload, bool retain);

void MQTT_SetConnectCallback(MqttClient_t *mqtt, MqttConnectCallback_t callback);
void MQTT_SetMessageCallback(MqttClient_t *mqtt, MqttMessageCallback_t callback);

#endif // SIM800_MQTT_H