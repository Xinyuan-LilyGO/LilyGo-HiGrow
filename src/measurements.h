#include "Arduino.h"

#include <BH1750.h>
#include "DHT12_sensor_library/DHT12.h"

struct Measurements
{
    float lux = 0.f;
    float humidity = 0.f;
    float temperature_C = 0.f;
    float soil = 0.f;
    float salt = 0.f;
    float battery_mV = 0.f;
    uint32_t timestamp_ms = 0;
};

bool takeMeasurements(BH1750 *lightMeter, DHT12 *dht12, Measurements *outMeasurements);