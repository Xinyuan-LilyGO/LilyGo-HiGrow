#include "measurements.h"
#include "pins.h"
#include "driver/adc.h"

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
    if (lightMeter == nullptr ||   //
        dht12 == nullptr ||        //
        outMeasurements == nullptr //
    )
    {
        return false;
    }

    // try to read these several times as sometimes they return NaN
    constexpr uint8_t kMaxNumAttempts = 10;
    uint8_t numAttempts = 0;
    constexpr uint32_t kTimeBetweenAttempts_ms = 1000;
    while (true)
    {
        outMeasurements->temperature_C = dht12->readTemperature();
        outMeasurements->humidity = dht12->readHumidity();
        if (!isnan(outMeasurements->temperature_C) && //
            !isnan(outMeasurements->humidity))
        {
            break;
        }

        ++numAttempts;
        if (numAttempts >= kMaxNumAttempts)
        {
            Serial.println("Measurements failed.");
            return false;
        }

        Serial.println("DHT12 measurements failed, retrying...");
        delay(kTimeBetweenAttempts_ms);
    }

    // we have to read twice as the first is always 0
    outMeasurements->lux = lightMeter->readLightLevel();
    delay(1000);
    outMeasurements->lux = lightMeter->readLightLevel();

    // turn the ADC on to take other measurements
    adc_power_on();
    outMeasurements->soil = readSoil();
    outMeasurements->salt = readSalt();
    outMeasurements->battery_mV = readBattery();
    adc_power_off();

    outMeasurements->timestamp_ms = millis();
    return true;
}