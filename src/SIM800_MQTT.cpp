#include "SIM800_MQTT.h"

// ============================================================================
// Private helpers
// ============================================================================
static uint16_t _mqtt_encode_length(uint8_t *buf, uint16_t len);
static uint16_t _mqtt_decode_length(const uint8_t *buf, uint16_t *value);
static uint16_t _mqtt_build_connect(MqttClient_t *mqtt, uint8_t *buf);
static uint16_t _mqtt_build_subscribe(MqttClient_t *mqtt, uint8_t *buf, const char *topic);
static uint16_t _mqtt_build_publish(MqttClient_t *mqtt, uint8_t *buf, const char *topic, const uint8_t *payload, uint16_t len, bool retain);
static uint16_t _mqtt_build_pingreq(uint8_t *buf);
static void _mqtt_handle_packet(MqttClient_t *mqtt, const uint8_t *packet, uint16_t len);
static void _mqtt_on_tcp_data(const uint8_t *data, uint16_t len);
static bool _mqtt_rx_buffer_complete(MqttClient_t *mqtt);
static void _mqtt_on_tcp_connect(bool connected);
static void _mqtt_on_gprs_connect(bool connected);

// Global pointer for callbacks (single instance only)
static MqttClient_t *_g_mqtt = NULL;

// ============================================================================
// Init
// ============================================================================
void MQTT_ClientInit(MqttClient_t *mqtt, SIM800_t *sim)
{
    if (!mqtt || !sim)
        return;

    memset(mqtt, 0, sizeof(MqttClient_t));
    mqtt->sim = sim;
    mqtt->state = MQTT_IDLE;
    mqtt->keepalive = MQTT_KEEPALIVE;
    mqtt->port = 1883;
    mqtt->topic_count = 0;

    strcpy(mqtt->client_id, "ESP32_SIM800");

    _g_mqtt = mqtt;

    Serial.println("[MQTT] Client initialized");
}

void MQTT_SetBroker(MqttClient_t *mqtt, const char *host, uint16_t port)
{
    if (!mqtt || !host)
        return;

    strncpy(mqtt->broker, host, sizeof(mqtt->broker) - 1);
    mqtt->broker[sizeof(mqtt->broker) - 1] = '\0';
    mqtt->port = port;

    Serial.print("[MQTT] Broker: ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);
}

void MQTT_SetAuth(MqttClient_t *mqtt, const char *client_id, const char *user, const char *pass)
{
    if (!mqtt)
        return;

    if (client_id)
    {
        strncpy(mqtt->client_id, client_id, sizeof(mqtt->client_id) - 1);
        mqtt->client_id[sizeof(mqtt->client_id) - 1] = '\0';
    }

    if (user)
    {
        strncpy(mqtt->username, user, sizeof(mqtt->username) - 1);
        mqtt->username[sizeof(mqtt->username) - 1] = '\0';
    }

    if (pass)
    {
        strncpy(mqtt->password, pass, sizeof(mqtt->password) - 1);
        mqtt->password[sizeof(mqtt->password) - 1] = '\0';
    }
}

void MQTT_SetAPN(MqttClient_t *mqtt, const char *apn, const char *user, const char *pass)
{
    if (!mqtt || !apn)
        return;

    strncpy(mqtt->apn, apn, sizeof(mqtt->apn) - 1);
    mqtt->apn[sizeof(mqtt->apn) - 1] = '\0';

    if (user)
    {
        strncpy(mqtt->apn_user, user, sizeof(mqtt->apn_user) - 1);
        mqtt->apn_user[sizeof(mqtt->apn_user) - 1] = '\0';
    }

    if (pass)
    {
        strncpy(mqtt->apn_pass, pass, sizeof(mqtt->apn_pass) - 1);
        mqtt->apn_pass[sizeof(mqtt->apn_pass) - 1] = '\0';
    }
}

// ============================================================================
// Connect
// ============================================================================
void MQTT_Connect(MqttClient_t *mqtt)
{
    if (!mqtt)
        return;

    if (mqtt->state != MQTT_IDLE && mqtt->state != MQTT_ERROR)
    {
        Serial.println("[MQTT] Already connecting/connected");
        return;
    }

    if (strlen(mqtt->broker) == 0 || strlen(mqtt->apn) == 0)
    {
        Serial.println("[MQTT] Broker or APN not set");
        return;
    }

    Serial.println("[MQTT] Starting connection...");

    // Set callbacks
    SIM800_SetGprsCallback(mqtt->sim, _mqtt_on_gprs_connect);
    SIM800_SetTcpConnectCallback(mqtt->sim, _mqtt_on_tcp_connect);
    SIM800_SetTcpDataCallback(mqtt->sim, _mqtt_on_tcp_data);

    // Start GPRS
    mqtt->state = MQTT_GPRS_CONNECTING;
    SIM800_GprsConnect(mqtt->sim, mqtt->apn, mqtt->apn_user, mqtt->apn_pass);
}

// ============================================================================
// Disconnect
// ============================================================================
void MQTT_Disconnect(MqttClient_t *mqtt)
{
    if (!mqtt)
        return;

    Serial.println("[MQTT] Disconnecting...");

    // Send DISCONNECT packet
    uint8_t buf[2] = {MQTT_DISCONNECT, 0x00};
    SIM800_TcpSend(mqtt->sim, buf, 2);

    mqtt->state = MQTT_DISCONNECTING;

    // Close TCP after small delay
    delay(100);
    SIM800_TcpClose(mqtt->sim);

    mqtt->state = MQTT_IDLE;
}

// ============================================================================
// Process
// ============================================================================
void MQTT_Process(MqttClient_t *mqtt)
{
    if (!mqtt)
        return;

    // ✅ پردازش بافر دریافتی — پکت‌ها بایت به بایت جمع میشن
    while (_mqtt_rx_buffer_complete(mqtt))
    {
        // پیدا کردن طول متغیر برای تعیین اندازه کل پکت
        uint16_t pos = 1;
        uint32_t remaining_len = 0;
        uint32_t multiplier = 1;
        while (pos < mqtt->rx_len)
        {
            uint8_t byte = mqtt->rx_buffer[pos];
            remaining_len += (byte & 0x7F) * multiplier;
            multiplier *= 128;
            pos++;
            if ((byte & 0x80) == 0)
                break;
        }
        uint16_t total = pos + remaining_len;

        Serial.print("[MQTT] RX complete packet: ");
        Serial.print(total);
        Serial.println("B");

        _mqtt_handle_packet(mqtt, mqtt->rx_buffer, total);

        // جابجایی باقیمانده بافر
        if (mqtt->rx_len > total)
        {
            memmove(mqtt->rx_buffer, mqtt->rx_buffer + total, mqtt->rx_len - total);
            mqtt->rx_len -= total;
        }
        else
        {
            mqtt->rx_len = 0;
        }
    }

    uint32_t now = millis();

    switch (mqtt->state)
    {
    case MQTT_GPRS_CONNECTING:
        // Wait for GPRS callback
        break;

    case MQTT_GPRS_CONNECTED:
        // GPRS ready, connect TCP
        if (SIM800_GprsIsConnected(mqtt->sim))
        {
            Serial.println("[MQTT] GPRS ready, connecting TCP...");
            mqtt->state = MQTT_TCP_CONNECTING;
            SIM800_TcpConnect(mqtt->sim, mqtt->broker, mqtt->port);
        }
        break;

    case MQTT_TCP_CONNECTING:
        // Wait for TCP callback
        break;

    case MQTT_TCP_CONNECTED:
        // TCP ready, send CONNECT packet
        Serial.println("[MQTT] TCP ready, sending CONNECT...");
        mqtt->tx_len = _mqtt_build_connect(mqtt, mqtt->tx_buffer);

        if (SIM800_TcpSend(mqtt->sim, mqtt->tx_buffer, mqtt->tx_len) == SIM800_OK)
        {
            mqtt->state = MQTT_WAIT_CONNACK;
            mqtt->last_activity = now;
        }
        else
        {
            Serial.println("[MQTT] Failed to send CONNECT");
            mqtt->state = MQTT_ERROR;
        }
        break;

    case MQTT_WAIT_CONNACK:
        // Wait for CONNACK (handled in _mqtt_handle_packet)
        if (now - mqtt->last_activity > 10000)
        {
            Serial.println("[MQTT] CONNACK timeout");
            mqtt->state = MQTT_ERROR;
        }
        break;

    case MQTT_CONNECTED:
        // Send PINGREQ if needed
        if (now - mqtt->last_ping > (mqtt->keepalive * 1000UL) / 2)
        {
            mqtt->last_ping = now;
            mqtt->tx_len = _mqtt_build_pingreq(mqtt->tx_buffer);
            SIM800_TcpSend(mqtt->sim, mqtt->tx_buffer, mqtt->tx_len);
            Serial.println("[MQTT] PINGREQ sent");
        }
        break;

    case MQTT_ERROR:
        // Auto-reconnect after 30s
        if (now - mqtt->last_activity > 30000)
        {
            Serial.println("[MQTT] Retrying...");
            mqtt->state = MQTT_IDLE;
            MQTT_Connect(mqtt);
        }
        break;

    default:
        break;
    }
}

// ============================================================================
// Status
// ============================================================================
bool MQTT_IsConnected(MqttClient_t *mqtt)
{
    return mqtt && (mqtt->state == MQTT_CONNECTED);
}

MqttState_t MQTT_GetState(MqttClient_t *mqtt)
{
    return mqtt ? mqtt->state : MQTT_IDLE;
}

// ============================================================================
// Subscribe
// ============================================================================
bool MQTT_Subscribe(MqttClient_t *mqtt, const char *topic)
{
    if (!mqtt || !topic)
        return false;

    if (mqtt->state != MQTT_CONNECTED)
    {
        Serial.println("[MQTT] Not connected");
        return false;
    }

    if (mqtt->topic_count >= MQTT_MAX_TOPICS)
    {
        Serial.println("[MQTT] Max topics reached");
        return false;
    }

    // Save topic
    strncpy(mqtt->topics[mqtt->topic_count], topic, sizeof(mqtt->topics[0]) - 1);
    mqtt->topics[mqtt->topic_count][sizeof(mqtt->topics[0]) - 1] = '\0';
    mqtt->topic_count++;

    // Build SUBSCRIBE packet
    mqtt->tx_len = _mqtt_build_subscribe(mqtt, mqtt->tx_buffer, topic);

    Serial.print("[MQTT] Subscribing: ");
    Serial.println(topic);

    return (SIM800_TcpSend(mqtt->sim, mqtt->tx_buffer, mqtt->tx_len) == SIM800_OK);
}

// ============================================================================
// Publish
// ============================================================================
bool MQTT_Publish(MqttClient_t *mqtt, const char *topic, const uint8_t *payload, uint16_t len, bool retain)
{
    if (!mqtt || !topic || !payload)
        return false;

    if (mqtt->state != MQTT_CONNECTED)
    {
        Serial.println("[MQTT] Not connected");
        return false;
    }

    mqtt->tx_len = _mqtt_build_publish(mqtt, mqtt->tx_buffer, topic, payload, len, retain);

    Serial.print("[MQTT] Publishing to ");
    Serial.print(topic);
    Serial.print(" (");
    Serial.print(len);
    Serial.println(" bytes)");

    return (SIM800_TcpSend(mqtt->sim, mqtt->tx_buffer, mqtt->tx_len) == SIM800_OK);
}

bool MQTT_PublishString(MqttClient_t *mqtt, const char *topic, const char *payload, bool retain)
{
    return MQTT_Publish(mqtt, topic, (const uint8_t *)payload, strlen(payload), retain);
}

// ============================================================================
// Callbacks
// ============================================================================
void MQTT_SetConnectCallback(MqttClient_t *mqtt, MqttConnectCallback_t callback)
{
    if (mqtt)
        mqtt->connect_callback = callback;
}

void MQTT_SetMessageCallback(MqttClient_t *mqtt, MqttMessageCallback_t callback)
{
    if (mqtt)
        mqtt->message_callback = callback;
}

// ============================================================================
// Private: GPRS callback
// ============================================================================
static void _mqtt_on_gprs_connect(bool connected)
{
    if (!_g_mqtt)
        return;

    if (connected)
    {
        Serial.println("[MQTT] GPRS connected");
        _g_mqtt->state = MQTT_GPRS_CONNECTED;
    }
    else
    {
        Serial.println("[MQTT] GPRS failed");
        _g_mqtt->state = MQTT_ERROR;
        _g_mqtt->last_activity = millis();
    }
}

// ============================================================================
// Private: TCP callback
// ============================================================================
static void _mqtt_on_tcp_connect(bool connected)
{
    if (!_g_mqtt)
        return;

    if (connected)
    {
        Serial.println("[MQTT] TCP connected");
        _g_mqtt->state = MQTT_TCP_CONNECTED;
    }
    else
    {
        Serial.println("[MQTT] TCP disconnected");

        if (_g_mqtt->state == MQTT_CONNECTED)
        {
            // Connection lost
            if (_g_mqtt->connect_callback)
                _g_mqtt->connect_callback(false);
        }

        _g_mqtt->state = MQTT_ERROR;
        _g_mqtt->last_activity = millis();
    }
}

// ============================================================================
// Private: TCP data callback — بافر جمع‌کننده پکت
// ============================================================================
static void _mqtt_on_tcp_data(const uint8_t *data, uint16_t len)
{
    if (!_g_mqtt || !data || len == 0)
        return;

    // هر بایت رو به بافر اضافه کن
    MqttClient_t *mqtt = _g_mqtt;
    for (uint16_t i = 0; i < len && mqtt->rx_len < MQTT_MAX_PACKET_SIZE; i++)
    {
        mqtt->rx_buffer[mqtt->rx_len++] = data[i];
    }
}

// ============================================================================
// بررسی کامل بودن پکت در بافر
// ============================================================================
static bool _mqtt_rx_buffer_complete(MqttClient_t *mqtt)
{
    if (mqtt->rx_len < 2)
        return false;

    // Decode remaining length (variable-length encoding)
    uint16_t pos = 1;
    uint32_t remaining_len = 0;
    uint32_t multiplier = 1;

    while (pos < mqtt->rx_len)
    {
        uint8_t byte = mqtt->rx_buffer[pos];
        remaining_len += (byte & 0x7F) * multiplier;
        multiplier *= 128;
        pos++;
        if ((byte & 0x80) == 0)
            break;
        if (multiplier > 128 * 128 * 128)
            return false; // malformed
    }

    // آیا کل پکت (header + remaining) رسیده؟
    uint16_t total = pos + remaining_len;
    return (mqtt->rx_len >= total);
}

// ============================================================================
// Private: Handle incoming MQTT packet
// ============================================================================
static void _mqtt_handle_packet(MqttClient_t *mqtt, const uint8_t *packet, uint16_t len)
{
    if (len < 2)
        return;

    uint8_t type = packet[0] & 0xF0;

    Serial.print("[MQTT] RX packet type: 0x");
    Serial.println(type, HEX);

    switch (type)
    {
    case MQTT_CONNACK:
    {
        if (len < 4)
            break;

        uint8_t return_code = packet[3];

        if (return_code == 0x00)
        {
            Serial.println("[MQTT] ✓ CONNACK - Connected!");
            mqtt->state = MQTT_CONNECTED;
            mqtt->last_ping = millis();

            if (mqtt->connect_callback)
                mqtt->connect_callback(true);
        }
        else
        {
            Serial.print("[MQTT] ✗ CONNACK failed, code: ");
            Serial.println(return_code);
            mqtt->state = MQTT_ERROR;

            if (mqtt->connect_callback)
                mqtt->connect_callback(false);
        }
        break;
    }

    case MQTT_SUBACK:
        Serial.println("[MQTT] SUBACK received");
        break;

    case MQTT_PUBLISH:
    {
        // Parse PUBLISH packet
        uint16_t pos = 1;
        uint16_t remaining_len;
        pos += _mqtt_decode_length(packet + pos, &remaining_len);

        // Topic length
        uint16_t topic_len = (packet[pos] << 8) | packet[pos + 1];
        pos += 2;

        // Topic
        char topic[64];
        if (topic_len < sizeof(topic))
        {
            memcpy(topic, packet + pos, topic_len);
            topic[topic_len] = '\0';
            pos += topic_len;

            // Payload
            uint16_t payload_len = len - pos;

            Serial.print("[MQTT] PUBLISH [");
            Serial.print(topic);
            Serial.print("]: ");
            Serial.write(packet + pos, payload_len);
            Serial.println();

            if (mqtt->message_callback)
            {
                mqtt->message_callback(topic, packet + pos, payload_len);
            }
        }
        break;
    }

    case MQTT_PINGRESP:
        Serial.println("[MQTT] PINGRESP");
        break;

    default:
        Serial.print("[MQTT] Unknown packet: 0x");
        Serial.println(type, HEX);
        break;
    }
}

// ============================================================================
// MQTT Protocol Helpers
// ============================================================================

static uint16_t _mqtt_encode_length(uint8_t *buf, uint16_t len)
{
    uint16_t pos = 0;
    do
    {
        uint8_t byte = len % 128;
        len /= 128;
        if (len > 0)
            byte |= 0x80;
        buf[pos++] = byte;
    } while (len > 0);
    return pos;
}

static uint16_t _mqtt_decode_length(const uint8_t *buf, uint16_t *value)
{
    uint16_t multiplier = 1;
    uint16_t len = 0;
    uint16_t pos = 0;
    uint8_t byte;

    do
    {
        byte = buf[pos++];
        len += (byte & 0x7F) * multiplier;
        multiplier *= 128;
    } while ((byte & 0x80) != 0);

    *value = len;
    return pos;
}

static uint16_t _mqtt_build_connect(MqttClient_t *mqtt, uint8_t *buf)
{
    uint16_t pos = 0;

    // Fixed header
    buf[pos++] = MQTT_CONNECT;

    // Remaining length placeholder
    uint16_t len_pos = pos;
    pos++;

    // Variable header
    buf[pos++] = 0x00; // Protocol Name Length MSB
    buf[pos++] = 0x04; // Protocol Name Length LSB
    buf[pos++] = 'M';
    buf[pos++] = 'Q';
    buf[pos++] = 'T';
    buf[pos++] = 'T';
    buf[pos++] = 0x04; // Protocol Level (MQTT 3.1.1)

    // Connect flags
    uint8_t flags = 0x02; // Clean Session
    if (strlen(mqtt->username) > 0)
    {
        flags |= 0x80; // Username flag
        if (strlen(mqtt->password) > 0)
            flags |= 0x40; // Password flag
    }
    buf[pos++] = flags;

    // Keep Alive
    buf[pos++] = (mqtt->keepalive >> 8) & 0xFF;
    buf[pos++] = mqtt->keepalive & 0xFF;

    // Payload: Client ID
    uint16_t id_len = strlen(mqtt->client_id);
    buf[pos++] = (id_len >> 8) & 0xFF;
    buf[pos++] = id_len & 0xFF;
    memcpy(buf + pos, mqtt->client_id, id_len);
    pos += id_len;

    // Username
    if (flags & 0x80)
    {
        uint16_t user_len = strlen(mqtt->username);
        buf[pos++] = (user_len >> 8) & 0xFF;
        buf[pos++] = user_len & 0xFF;
        memcpy(buf + pos, mqtt->username, user_len);
        pos += user_len;
    }

    // Password
    if (flags & 0x40)
    {
        uint16_t pass_len = strlen(mqtt->password);
        buf[pos++] = (pass_len >> 8) & 0xFF;
        buf[pos++] = pass_len & 0xFF;
        memcpy(buf + pos, mqtt->password, pass_len);
        pos += pass_len;
    }

    // Fill remaining length
    uint16_t remaining = pos - len_pos - 1;
    buf[len_pos] = remaining; // Simple encoding (works for len < 128)

    return pos;
}

static uint16_t _mqtt_build_subscribe(MqttClient_t *mqtt, uint8_t *buf, const char *topic)
{
    uint16_t pos = 0;
    uint16_t topic_len = strlen(topic);

    // Fixed header
    buf[pos++] = MQTT_SUBSCRIBE;

    // Remaining length
    uint16_t remaining = 2 + 2 + topic_len + 1; // Packet ID + Topic + QoS
    buf[pos++] = remaining;

    // Packet ID
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;

    // Topic
    buf[pos++] = (topic_len >> 8) & 0xFF;
    buf[pos++] = topic_len & 0xFF;
    memcpy(buf + pos, topic, topic_len);
    pos += topic_len;

    // QoS
    buf[pos++] = 0x00; // QoS 0

    return pos;
}

static uint16_t _mqtt_build_publish(MqttClient_t *mqtt, uint8_t *buf, const char *topic, const uint8_t *payload, uint16_t len, bool retain)
{
    uint16_t pos = 0;
    uint16_t topic_len = strlen(topic);

    // Fixed header
    uint8_t flags = 0x00;
    if (retain)
        flags |= 0x01;
    buf[pos++] = MQTT_PUBLISH | flags;

    // Remaining length
    uint16_t remaining = 2 + topic_len + len;
    buf[pos++] = remaining; // Simple (len < 128)

    // Topic
    buf[pos++] = (topic_len >> 8) & 0xFF;
    buf[pos++] = topic_len & 0xFF;
    memcpy(buf + pos, topic, topic_len);
    pos += topic_len;

    // Payload
    memcpy(buf + pos, payload, len);
    pos += len;

    return pos;
}

static uint16_t _mqtt_build_pingreq(uint8_t *buf)
{
    buf[0] = MQTT_PINGREQ;
    buf[1] = 0x00;
    return 2;
}