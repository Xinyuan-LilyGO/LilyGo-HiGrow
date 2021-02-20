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
#include "pb_encode.h"
#include "nvs_utils.h"
#include "PubSubClient.h"
#include "time_helpers.h"
#include "pins.h"

//#define TTGO_DEBUG_PRINT

BH1750 lightMeter(0x23); //0x23
DHT12 dht12(DHT12_PIN, true);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

constexpr char kMQTTBroker[] = "ttgo-server";
constexpr uint16_t kMQTTBrokerPort = 1883;
char g_mqttName[1024] = "sensor0";
char g_mqttTopicRoot[1024] = "sensor0";

// working data stored in RTC memory
constexpr uint32_t kTimeBetweenMeasurements_ms = 2 * 60 * 1000;
constexpr uint8_t kNumMeasurementsToTakeBeforeSending = 5;
RTC_DATA_ATTR ttgo_proto_Measurements g_measurements[kNumMeasurementsToTakeBeforeSending];
RTC_DATA_ATTR uint8_t g_numMeasurementsRecorded = 0;
constexpr uint32_t kTimeBetweenRTCUpdates_ms = 1 * 60 * 60 * 1000;          // how often is the real time clock updated using NTC server
RTC_DATA_ATTR uint32_t g_timeSinceRTCUpdate_ms = kTimeBetweenRTCUpdates_ms; // set to time limit to update once at the start

#define PRINT(x)         \
    if (Serial)          \
    {                    \
        Serial.print(x); \
    }
#define PRINTLN(x)         \
    if (Serial)            \
    {                      \
        Serial.println(x); \
    }

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
void publishMessage(const char *subTopic, T value)
{
    static char topicBuffer[100];
    sprintf(topicBuffer, "%s/%s", g_mqttTopicRoot, subTopic);
    mqttClient.publish(topicBuffer, reinterpret_cast<uint8_t *>(&value), sizeof(T));
}

void publishMessage(const char *subTopic, uint8_t *data, uint32_t numBytes)
{
    static char topicBuffer[100];
    sprintf(topicBuffer, "%s/%s", g_mqttTopicRoot, subTopic);
    mqttClient.publish(topicBuffer, data, numBytes);
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

template <typename T>
void publishMessage(const char *subTopic, const char *dataTypeString, const T *data)
{
    static char topicBuffer[100];
    sprintf(topicBuffer, "%s/%s", g_mqttTopicRoot, subTopic);
    const uint8_t *dataStart = (uint8_t *)data;
    PRINT(topicBuffer);
    PRINT(" ");
    PRINTLN(*data);
    mqttClient.publish(topicBuffer, dataStart, sizeof(T));
}

void enterDeepSleep()
{
    //inspired by https://www.reddit.com/r/esp32/comments/exgi32/esp32_ultralow_power_mode/
    PRINT("Powering down for ");
    PRINT(kTimeBetweenMeasurements_ms / 1000);
    PRINTLN(" seconds...");
    digitalWrite(POWER_CTRL, LOW);
    WiFi.disconnect(true); // Keeps WiFi APs happy
    WiFi.mode(WIFI_OFF);   // Switch WiFi off
    esp_sleep_enable_timer_wakeup(kTimeBetweenMeasurements_ms * 1000);
    esp_deep_sleep_start();
}

bool connectToWifi()
{
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
        return false;
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        PRINT("IP Address: ");
        PRINTLN(WiFi.localIP());
        return true;
    }
    return false;
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

    PRINTLN("Firmware version ");
    PRINT(FW_VERSION_MAJOR);
    PRINT(".");
    PRINT(FW_VERSION_MINOR);
    PRINT(".");
    PRINT(FW_VERSION_PATCH);
    PRINT(" Build time ");
    PRINTLN(BUILD_TIME);

    // set CPU to low frequency
    setCpuFrequencyMhz(80);
    PRINT("CPU frequency set to ");
    PRINT(getCpuFrequencyMhz());
    PRINTLN(" MHz");

    // update rtc if necessesary
    bool wifiConnected = false;
    if (g_timeSinceRTCUpdate_ms >= kTimeBetweenRTCUpdates_ms)
    {
        wifiConnected = connectToWifi();
        if (wifiConnected && tryToUpdateAbsoluteTime())
        {
            g_timeSinceRTCUpdate_ms = 0;
            char timeString[100];
            getLocalTimeString(timeString, 100);
            PRINT("Updated RTC: ");
            PRINTLN(timeString);
        }
        else
        {
            PRINTLN("Failed to update RTC.");
        }
    }

    // if we need to, take a measurment
    if (g_numMeasurementsRecorded < kNumMeasurementsToTakeBeforeSending)
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

        // DHT12 takes a long time, delay till it's ready
        delay(2500);

        // take measurements
        ttgo_proto_Measurements *nextMeasurement = &g_measurements[g_numMeasurementsRecorded];
        if (takeMeasurements(&lightMeter, &dht12, nextMeasurement))
        {
#ifdef TTGO_DEBUG_PRINT
            PRINTLN(nextMeasurement->lux);
            PRINTLN(nextMeasurement->humidity);
            PRINTLN(nextMeasurement->salt);
            PRINTLN(nextMeasurement->soil);
            PRINTLN(nextMeasurement->temperature_C);
#endif
            ++g_numMeasurementsRecorded;
        }
        else
        {
            PRINTLN("Failed measurement");
        }
        digitalWrite(POWER_CTRL, LOW);
    }

    PRINT("Taken measurements ");
    PRINT(g_numMeasurementsRecorded);
    PRINT("/");
    PRINTLN(kNumMeasurementsToTakeBeforeSending);

    // if we still have more measurements to take, then go back to sleep
    if (g_numMeasurementsRecorded < kNumMeasurementsToTakeBeforeSending)
    {
        enterDeepSleep();
    }

    // if we got this far, it's time to transmit data over wifi

    // if we're not already connected, connect
    if (!wifiConnected)
    {
        wifiConnected = connectToWifi();
    }

    // if we are connected, try and send data
    if (wifiConnected)
    {
        // get absolute time if necessary
        if (g_timeSinceRTCUpdate_ms >= kTimeBetweenRTCUpdates_ms)
        {
            if (tryToUpdateAbsoluteTime())
            {
                g_timeSinceRTCUpdate_ms = 0;
                char timeString[100];
                getLocalTimeString(timeString, 100);
                PRINT("Updated RTC: ");
                PRINTLN(timeString);
            }
            else
            {
                PRINTLN("Failed to update RTC.");
            }
        }

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
        for (size_t i = 0; i < g_numMeasurementsRecorded; ++i)
        {
            PRINT("Sending measurement ");
            PRINT(i + 1);
            PRINT("/");
            PRINTLN(g_numMeasurementsRecorded);

            ttgo_proto_Measurements &measurements = g_measurements[i];

            // fill in version info
            measurements.fw_version_major = FW_VERSION_MAJOR;
            measurements.fw_version_minor = FW_VERSION_MINOR;
            measurements.fw_version_patch = FW_VERSION_PATCH;

            // encode protobuf
            uint8_t protoBuffer[ttgo_proto_Measurements_size];
            pb_ostream_t stream = pb_ostream_from_buffer(protoBuffer, sizeof(protoBuffer));
            const bool encodeSuccess = pb_encode(&stream, ttgo_proto_Measurements_fields, &measurements);
            if (!encodeSuccess)
            {
                PRINTLN("Failed to encode.  Skipping.");
                continue;
            }

            // send
            const size_t message_length = stream.bytes_written;
            publishMessage("measurement", protoBuffer, message_length);

            // print out for debug
            printMeasurements(Serial, measurements);
        }

        // mark all as sent so we'll measure a new batch
        g_numMeasurementsRecorded = 0;

        mqttClient.disconnect();
    }

    // finally, go back to sleep
    g_timeSinceRTCUpdate_ms += (kTimeBetweenMeasurements_ms * kNumMeasurementsToTakeBeforeSending);
    enterDeepSleep();
}

void loop()
{
}