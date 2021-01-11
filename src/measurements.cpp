#include "measurements.h"
#include "pins.h"
#include "driver/adc.h"
#include <WiFiMulti.h>

uint32_t readSalt()
{
    uint8_t samples = 120;
    uint32_t humi = 0;
    uint16_t array[120];

    for (int i = 0; i < samples; i++)
    {
        array[i] = analogRead(SALT_PIN);
        delay(2);
    }
    std::sort(array, array + samples);
    for (int i = 0; i < samples; i++)
    {
        if (i == 0 || i == samples - 1)
            continue;
        humi += array[i];
    }
    humi /= samples - 2;
    return humi;
}

uint16_t readSoil()
{
    uint16_t soil = analogRead(SOIL_PIN);
    return map(soil, 0, 4095, 100, 0);
}

float readBattery()
{
    int vref = 1100;
    uint16_t volt = analogRead(BAT_ADC);
    float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref);
    return battery_voltage;
}

bool takeMeasurements(BH1750 *lightMeter, DHT12 *dht12, Measurements *outMeasurements)
{
    if (outMeasurements == nullptr)
    {
        return false;
    }
    
    outMeasurements->lux = lightMeter->readLightLevel();
    outMeasurements->temperature_C = dht12->readTemperature();
    outMeasurements->humidity = dht12->readHumidity();
    
    // turn the ADC on
    adc_power_on();
    outMeasurements->soil = readSoil();
    outMeasurements->salt = readSalt();
    outMeasurements->battery_mV = readBattery();
    adc_power_off();

    outMeasurements->rssi = WiFi.RSSI();
    return true;
}