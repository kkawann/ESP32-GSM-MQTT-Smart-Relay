#include <Arduino.h>
#include "ota.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "globals.h"

OTAState otaState;

// Background task wrapper
static void otaTask(void *pv)
{
    otaCheck();
    vTaskDelete(NULL);
}

void otaBegin()
{
    if (otaState.status == OTA_CHECKING || otaState.status == OTA_DOWNLOADING)
        return;
    xTaskCreate(otaTask, "OTA", 8192, NULL, 1, NULL);
}

// Check server for a newer firmware and install if available
void otaCheck()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        otaState.status = OTA_FAILED;
        otaState.message = "WiFi not connected";
        Serial.println("[OTA] WiFi not connected");
        return;
    }

    // Step 1: ask server for update info
    otaState.status = OTA_CHECKING;
    otaState.progress = 0;
    otaState.message = "Checking server...";

    HTTPClient http;
    String url = "http://" + String(SERVER_IP) + "/api/check/" + String(DEVICE_TYPE);
    Serial.println("[OTA] GET " + url);

    http.begin(url);
    http.addHeader("x-device-version", CURRENT_VERSION);
    http.setTimeout(10000);

    int code = http.GET();
    Serial.println("[OTA] HTTP code: " + String(code));

    if (code == 304)
    {
        http.end();
        otaState.status = OTA_UP_TO_DATE;
        otaState.message = "Already up to date";
        Serial.println("[OTA] Up to date");
        return;
    }

    if (code != 200)
    {
        http.end();
        otaState.status = OTA_FAILED;
        otaState.message = "Server error: " + String(code);
        Serial.println("[OTA] Server error: " + String(code));
        return;
    }

    String payload = http.getString();
    http.end();
    Serial.println("[OTA] Payload: " + payload);

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload))
    {
        otaState.status = OTA_FAILED;
        otaState.message = "JSON parse error";
        return;
    }

    String fwUrl = doc["url"] | "";
    String newVer = doc["version"] | "?";

    if (fwUrl.isEmpty())
    {
        otaState.status = OTA_FAILED;
        otaState.message = "No URL in response";
        return;
    }

    otaState.newVersion = newVer;
    otaState.message = "Downloading " + newVer + "...";

    // Step 2: download and flash
    otaState.status = OTA_DOWNLOADING;
    otaState.progress = 0;

    HTTPClient http2;
    http2.begin(fwUrl);
    http2.setTimeout(60000);

    int code2 = http2.GET();
    if (code2 != 200)
    {
        http2.end();
        otaState.status = OTA_FAILED;
        otaState.message = "Download error: " + String(code2);
        Serial.println("[OTA] Download error: " + String(code2));
        return;
    }

    int contentLength = http2.getSize();
    Serial.println("[OTA] Firmware size: " + String(contentLength));

    if (contentLength <= 0)
    {
        http2.end();
        otaState.status = OTA_FAILED;
        otaState.message = "Unknown content length";
        return;
    }

    if (!Update.begin(contentLength))
    {
        http2.end();
        otaState.status = OTA_FAILED;
        otaState.message = "Not enough space";
        Update.printError(Serial);
        return;
    }

    WiFiClient *stream = http2.getStreamPtr();
    size_t written = 0;
    uint8_t buf[512];

    while (written < (size_t)contentLength)
    {
        int avail = stream->available();
        if (avail <= 0)
        {
            delay(1);
            continue;
        }

        int toRead = min(avail, (int)sizeof(buf));
        int r = stream->readBytes(buf, toRead);
        if (r <= 0)
            break;

        Update.write(buf, r);
        written += r;
        otaState.progress = (written * 100) / contentLength;
    }

    http2.end();

    if (Update.end() && Update.isFinished())
    {
        otaState.status = OTA_SUCCESS;
        otaState.progress = 100;
        otaState.message = "Update OK — Rebooting";
        Serial.println("[OTA] Update OK — Rebooting");
        delay(500);
        ESP.restart();
    }
    else
    {
        otaState.status = OTA_FAILED;
        otaState.message = "Write failed";
        Update.printError(Serial);
    }
}