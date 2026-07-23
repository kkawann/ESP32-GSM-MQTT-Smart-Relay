#include "sensor.h"
#include "globals.h"
#include "scene.h"
#include "utils.h"
#include "storage.h"

uint32_t extractRawValue(uint32_t code, uint32_t valueMask)
{
    if (valueMask == 0)
        return 0;
    uint32_t masked = code & valueMask;
    uint32_t shift = 0;
    uint32_t tmp = valueMask;
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
    return (code & s.baseMask) == (s.baseCode & s.baseMask);
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
    static const char *units[] = {"%", "\xC2\xB0"
                                       "C",
                                  "%RH", "cm", "V", ""};
    static const int unitsCnt = (int)(sizeof(units) / sizeof(units[0]));

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
            valueType = rfSensors[i].valueType;

            xSemaphoreGive(xSensorMutex);
        }
        else
        {
            continue;
        }

        addToSensorHistory(i, physical);

        int unitIdx = ((int)valueType < unitsCnt) ? (int)valueType
                                                  : (unitsCnt - 1);

        char msg[64];
        snprintf(msg, sizeof(msg), "Sensor '%s' = %.1f%s",
                 sensorName, physical, units[unitIdx]);
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