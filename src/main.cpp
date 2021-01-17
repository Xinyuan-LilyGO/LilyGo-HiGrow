#include <algorithm>
#include <iostream>
#include <Arduino.h>
#include <WiFi.h>

#include <Wire.h>
#include <BH1750.h>
#include "DHT12_sensor_library/DHT12.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "measurements.h"
#include "nvs_utils.h"
#include "PubSubClient.h"
#include "time_helpers.h"
#include "pins.h"

BH1750 lightMeter(0x23); //0x23
DHT12 dht12(DHT12_PIN, true);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

constexpr char kMQTTBroker[] = "test.mosquitto.org";
constexpr uint16_t kMQTTBrokerPort = 1883;
char g_mqttName[1024] = "otdevice";
char g_mqttTopicRoot[1024] = "otsensor";

// working data stored in the RTC memory
RTC_DATA_ATTR struct WorkingData
{
    // storage space for measurements
    static constexpr uint8_t kNumMeasurementsToTakeBeforeSending = 2;
    Measurements measurements[kNumMeasurementsToTakeBeforeSending];
    uint8_t numMeasurementsRecorded = 0;

    static constexpr uint32_t kTimeBetweenMeasurements_ms = 2000;
    //TmAndMillis globalTimeReference;
    bool hasGlobalTimeReference = false;

} g_workingData;

#define PRINT(x) Serial.print(x)
#define PRINTLN(x) Serial.println(x)

bool tryInitI2CAndDevices()
{
    PRINTLN("tryInitI2CAndDevices");
    if (!Wire.begin(I2C_SDA, I2C_SCL))
    {
        PRINTLN("Failed to start I2C");
        return false;
    }

    dht12.begin();

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    {
        PRINTLN(F("BH1750 Advanced begin"));
    }
    else
    {
        PRINTLN(F("Error initialising BH1750"));
    }

    delay(1000); //  delay to make things reliable
    PRINTLN("tryInitI2CAndDevices: done");
    return true;
}

void smartConfigStart()
{
    PRINTLN("smartConfigStart...");

    WiFi.disconnect();
    WiFi.beginSmartConfig();
    while (!WiFi.smartConfigDone())
    {
        PRINT(".");
        delay(200);
    }
    WiFi.stopSmartConfig();
    PRINTLN();
    PRINTLN("smartConfigStop Connected:");
    PRINT("Got SSID: ");
    PRINTLN(WiFi.SSID().c_str());
    PRINT("Got PSK: ");
    PRINTLN(WiFi.psk().c_str());

    if (!writeSSIDPW(WiFi.SSID().c_str(), WiFi.psk().c_str()))
    {
        PRINTLN("Failed to save SSID and Password");
    }
}

template <typename T>
void publishMessage(const char *subTopic, const char *valueFormat, T value)
{
    static char topicBuffer[100];
    static char valueBuffer[100];
    sprintf(topicBuffer, "%s/%s", g_mqttTopicRoot, subTopic);
    sprintf(valueBuffer, valueFormat, value);
    PRINT(topicBuffer);
    PRINT(" ");
    PRINTLN(valueBuffer);
    mqttClient.publish(topicBuffer, valueBuffer);
}

void enterDeepSleep()
{
    //inspired by https://www.reddit.com/r/esp32/comments/exgi32/esp32_ultralow_power_mode/
    PRINTLN("Powering down...");
    digitalWrite(POWER_CTRL, LOW);
    WiFi.disconnect(true); // Keeps WiFi APs happy
    WiFi.mode(WIFI_OFF);   // Switch WiFi off
    esp_sleep_enable_timer_wakeup(WorkingData::kTimeBetweenMeasurements_ms * 1000);
    esp_deep_sleep_start();
}

void setup()
{
    // setup GPIOs
    pinMode(POWER_CTRL, OUTPUT);
    pinMode(USER_BUTTON, INPUT);
    pinMode(SOIL_PIN, ANALOG);
    pinMode(SALT_PIN, ANALOG);
    pinMode(BAT_ADC, ANALOG);

    Serial.begin(115200);
    delay(100);

    // disable saving wifi details into Flash as it wears it down and is anyway unreliable
    // so we store details in NVS instead by ourselves
    WiFi.persistent(false);

    // if started up with button held down, then go into smart config mode
    if (digitalRead(USER_BUTTON) == LOW)
    {
        smartConfigStart();
    }

    // set CPU to low frequency
    setCpuFrequencyMhz(80);
    PRINT("CPU frequency set to ");
    PRINT(getCpuFrequencyMhz());
    PRINTLN(" MHz");

    // if we need to, take a measurment
    if (g_workingData.numMeasurementsRecorded < WorkingData::kNumMeasurementsToTakeBeforeSending)
    {
        PRINTLN("Powering on to take measurement");
        digitalWrite(POWER_CTRL, HIGH);
        delay(1000);

        // initialise I2C devices
        bool connected = tryInitI2CAndDevices();
        while (!connected)
        {
            delay(200);
            connected = tryInitI2CAndDevices();
        }

        // take measurements
        Measurements *nextMeasurement = &g_workingData.measurements[g_workingData.numMeasurementsRecorded];
        if (takeMeasurements(&lightMeter, &dht12, nextMeasurement))
        {
#if 0
            PRINTLN(nextMeasurement->lux);
            PRINTLN(nextMeasurement->humidity);
            PRINTLN(nextMeasurement->salt);
            PRINTLN(nextMeasurement->soil);
            PRINTLN(nextMeasurement->temperature_C);
#endif
            ++g_workingData.numMeasurementsRecorded;
        }

        digitalWrite(POWER_CTRL, LOW);
    }

    PRINT("Taken measurements ");
    PRINT(g_workingData.numMeasurementsRecorded);
    PRINT("/");
    PRINTLN(WorkingData::kNumMeasurementsToTakeBeforeSending);

    // if we still have more measurements to take, then go back to sleep
    if (g_workingData.numMeasurementsRecorded < WorkingData::kNumMeasurementsToTakeBeforeSending)
    {
        enterDeepSleep();
    }

    // if we got this far, it's time to transmit data over wifi

    char ssid[1024];
    char password[1024];
    initNVS();
    const bool hasDetails = tryReadSSIDPW(ssid, password);
    if (hasDetails)
    {
        PRINT("MAC Address: ");
        PRINTLN(WiFi.macAddress());

        PRINT("Connecting to ");
        PRINT(ssid);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        PRINTLN("");
    }
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        PRINTLN("WiFi connect fail!,please restart retry,or long press BOOT button enter smart config mode\n");
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        PRINT("IP Address: ");
        PRINTLN(WiFi.localIP());

        // // get global time if necessary
        // if (!g_workingData.hasGlobalTimeReference)
        // {
        //     if (tryToGetGlobalTime())
        //     {
        //         g_workingData.globalTimeReference = globalTimeReference();
        //         g_workingData.hasGlobalTimeReference = true;
        //     }
        // }

        // now send them all
        mqttClient.setServer(kMQTTBroker, kMQTTBrokerPort);
        PRINTLN("Connecting MQTT client...");
        while (!mqttClient.connected())
        {
            if (mqttClient.connect(g_mqttName))
            {
                PRINT("MQTT client connected as '");
                PRINT(g_mqttName);
                PRINTLN("'");
                break;
            }

            PRINT("Failed to connect MQTT client.  State = ");
            PRINT(mqttClient.state());
            PRINTLN(". Retrying in 5s...");
            delay(5000);
        }

        // we're connected, now send!
        for (size_t i = 0; i < g_workingData.numMeasurementsRecorded; ++i)
        {
            const Measurements &measurements = g_workingData.measurements[i];
            publishMessage<float>("battery_mV", "%.3f", measurements.battery_mV);
            publishMessage<float>("humidity", "%.3f", measurements.humidity);
            publishMessage<float>("lux", "%.3f", measurements.lux);
            publishMessage<float>("salt", "%.3f", measurements.salt);
            publishMessage<float>("soil", "%.3f", measurements.soil);
            publishMessage<float>("temperature_C", "%.3f", measurements.temperature_C);
            publishMessage<float>("timestamp_ms", "%i", measurements.timestamp_ms);
        }

        // mark all as sent so we'll measure a new batch
        g_workingData.numMeasurementsRecorded = 0;

        mqttClient.disconnect();
        //WiFi.disconnect();
    }

    // finally, go back to sleep
    enterDeepSleep();
}

void loop()
{
}