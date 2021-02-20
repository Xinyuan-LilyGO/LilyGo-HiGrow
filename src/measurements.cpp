#include "measurements.h"
#include "pins.h"
#include "time_helpers.h"
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

bool takeMeasurements(BH1750 *lightMeter, DHT12 *dht12, ttgo_proto_Measurements *outMeasurements)
{
    if (lightMeter == nullptr ||   //
        dht12 == nullptr ||        //
        outMeasurements == nullptr //
    )
    {
        return false;
    }

    *outMeasurements = ttgo_proto_Measurements_init_default;

    // try to read these several times as sometimes they return NaN
    constexpr uint8_t kMaxNumAttempts = 5;
    outMeasurements->num_dht_failed_reads = 0;
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

        ++outMeasurements->num_dht_failed_reads;
        if (outMeasurements->num_dht_failed_reads >= kMaxNumAttempts)
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

    outMeasurements->timestamp = getEpochTime();

    return true;
}

void printMeasurements(Print &printer, const ttgo_proto_Measurements &measurements)
{
    char buffer[100];
    sprintf(buffer, "battery_mV: %.3f", measurements.battery_mV);
    printer.println(buffer);
    sprintf(buffer, "humidity: %.3f", measurements.lux);
    printer.println(buffer);
    sprintf(buffer, "lux: %.3f", measurements.lux);
    printer.println(buffer);
    sprintf(buffer, "salt: %.3f", measurements.salt);
    printer.println(buffer);
    sprintf(buffer, "soil: %.3f", measurements.soil);
    printer.println(buffer);
    sprintf(buffer, "temperature_C: %.3f", measurements.temperature_C);
    printer.println(buffer);
    sprintf(buffer, "timestamp: %zu", measurements.timestamp);
    printer.println(buffer);
    sprintf(buffer, "num_dht_failed_reads: %zu", measurements.num_dht_failed_reads);
    printer.println(buffer);
}