#include "sensor.h"
#include "globals.h"
#include "scene.h"
#include "utils.h"
#include "storage.h"

uint32_t extractRawValue(uint32_t code, uint32_t valueMask)
{
    uint32_t cleanMask = valueMask & 0x0FFFFFFFUL; // strip type-tag bits
    if (cleanMask == 0)
        return 0;
    uint32_t masked = code & cleanMask;
    uint32_t shift = 0;
    uint32_t tmp = cleanMask;
    while (tmp && !(tmp & 1))
    {
        tmp >>= 1;
        shift++;
    }
    return masked >> shift;
}

bool sensorMatchesCode(const RFSensor &s, uint32_t code,
                       uint8_t proto, uint16_t bits)
{
    if (!s.active)
        return false;
    if (s.protocol != proto)
        return false;
    if (s.bitLength != bits)
        return false;
    uint32_t cleanMask = s.baseMask & 0x0FFFFFFFUL; // strip type-tag bits
    return (code & cleanMask) == (s.baseCode & cleanMask);
}

void addToSensorHistory(int idx, float value)
{
    if (idx < 0 || idx >= MAX_RF_SENSORS)
        return;

    if (xSensorMutex && xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        SensorRingBuf &buf = sensorHistory[idx];
        buf.entries[buf.head] = {value, (uint32_t)millis()};
        buf.head = (buf.head + 1) % SENSOR_HIST_SZ;
        if (buf.count < SENSOR_HIST_SZ)
            buf.count++;
        xSemaphoreGive(xSensorMutex);
    }
}
float extractValue(const RFSensor &s, uint32_t code)
{
    if (s.valueBits == 0)
        return 1.0f;

    uint32_t mask = (s.valueBits >= 32) ? 0xFFFFFFFFUL
                                        : ((1UL << s.valueBits) - 1);
    uint32_t raw = code & mask;
    return (float)raw * s.scale + s.offset;
}

bool processSensorCode(uint32_t code, uint8_t protocol, uint16_t bitLen)
{
    bool found = false;

    for (int i = 0; i < rfSensorCount; i++)
    {
        if (!sensorMatchesCode(rfSensors[i], code, protocol, bitLen))
            continue;

        float physical = extractValue(rfSensors[i], code);

        char sensorName[32] = "";
        uint8_t valueType = 0;

        if (xSensorMutex &&
            xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            rfSensors[i].rxCount++;
            rfSensors[i].lastUpdateMs = (uint32_t)millis();
            rfSensors[i].lastValue = physical;
            rfSensors[i].hasValue = true;

            strncpy(sensorName, rfSensors[i].name, sizeof(sensorName) - 1);
            sensorName[sizeof(sensorName) - 1] = '\0';
            // Read type from signature (upper 4 bits of baseMask)
            valueType = getSensorTypeFromMask(rfSensors[i].baseMask);

            xSemaphoreGive(xSensorMutex);
        }
        else
        {
            continue;
        }

        addToSensorHistory(i, physical);

        // Auto-unit from type signature
        static const char *typeUnits[] = {"%","\xC2\xB0""C","%RH","cm","V","bool","bool",""};
        const char *unit = (valueType < 8) ? typeUnits[valueType] : "";

        char msg[64];
        snprintf(msg, sizeof(msg), "Sensor '%s' = %.1f%s",
                 sensorName, physical, unit);
        addLog(2, 0, msg);

        found = true;
    }

    return found;
}
void saveSensorsAsync()
{
    SaveCommand cmd = {2};
    xQueueSend(qSave, &cmd, pdMS_TO_TICKS(50));
}

// ─── Pack SensorTypeId into upper 4 bits of baseMask ───
void packSensorTypeIntoMask(RFSensor &s, uint8_t sensorTypeId)
{
    s.baseMask = (s.baseMask & 0x0FFFFFFFUL) | (((uint32_t)(sensorTypeId & 0x0F)) << 28);
}

uint8_t getSensorTypeFromMask(uint32_t mask)
{
    return (uint8_t)((mask >> 28) & 0x0F);
}