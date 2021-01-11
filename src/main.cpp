#include <algorithm>
#include <iostream>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

//#define USE_DASH
#ifdef USE_DASH
#include <ESPDash.h>
#endif

#include <ESPmDNS.h>
#include <Button2.h>
#include <Wire.h>
#include <BH1750.h>
#include "DHT12_sensor_library/DHT12.h"
#include <WiFiMulti.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "measurements.h"
#include "nvs_utils.h"
#include "PubSubClient.h"
#include "time_helpers.h"
#include "pins.h"

#define SOFTAP_MODE
// #define USE_18B20_TEMP_SENSOR
// #define USE_CHINESE_WEB

BH1750 lightMeter(0x23); //0x23
DHT12 dht12(DHT12_PIN, true);
AsyncWebServer server(80);
Button2 button(BOOT_PIN);
Button2 useButton(USER_BUTTON);
WiFiMulti multi;
PubSubClient mqttClient;

constexpr char * kMQTTBroker = "test.mosquitto.org";
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
    
    static constexpr uint32_t kTimeBetweenMeasurements_ms = 10000;
    TmAndMillis globalTimeReference;
    bool hasGlobalTimeReference = false;

} g_workingData;

void makeTopicString(char *buffer, size_t bufferSize, char *topicRoot, char* subTopic)
{
    memset(buffer, 0, bufferSize);
    strcat(buffer, topicRoot);
    strcat(buffer, "/");
    strcat(buffer, subTopic);
}

bool tryInitI2CAndDevices()
{
    Serial.println("tryInitI2CAndDevices");
    if (!Wire.begin(I2C_SDA, I2C_SCL))
    {
        return false;
    }

    dht12.begin();

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    {
        Serial.println(F("BH1750 Advanced begin"));
    }
    else
    {
        Serial.println(F("Error initialising BH1750"));
    }

    Serial.println("tryInitI2CAndDevices: done");
    return true;
}

void smartConfigStart(Button2 &b)
{
    Serial.println("smartConfigStart...");

    Serial.println("Erasing stored details");
    writeSSIDPW(nullptr, nullptr);

    WiFi.disconnect();
    WiFi.beginSmartConfig();
    while (!WiFi.smartConfigDone())
    {
        Serial.print(".");
        delay(200);
    }
    WiFi.stopSmartConfig();
    Serial.println();
    Serial.print("smartConfigStop Connected:");
    Serial.print("Got SSID: ");
    Serial.println(WiFi.SSID().c_str());
    Serial.print("Got PSK: ");
    Serial.println(WiFi.psk().c_str());
    if (!writeSSIDPW(WiFi.SSID().c_str(), WiFi.psk().c_str()))
    {
        Serial.println("Failed to store details");
    }
    else
    {
        char ssid[MAX_SSID_LENGTH];
        char pwd[MAX_PWD_LENGTH];
        memset(ssid, 0, MAX_SSID_LENGTH);
        memset(pwd, 0, MAX_PWD_LENGTH);
        if (tryReadSSIDPW(ssid, pwd))
        {
            Serial.print("SSID: ");
            Serial.println(ssid);
            Serial.print("PSK: ");
            Serial.println(pwd);
        }
        else
        {
            Serial.println("Failed to readback details");
        }
    }
}

void setup()
{
    // set CPU to low frequency
    setCpuFrequencyMhz(80);
    Serial.print("CPU frequency set to ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");

    Serial.begin(115200);

    // first of all, take a measurement
    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, 1);
    delay(1000);
    Measurements *nextMeasurement = &g_workingData.measurements[g_workingData.numMeasurementsRecorded];
    if (tryInitI2CAndDevices() && takeMeasurements(&lightMeter, &dht12, nextMeasurement))
    {
        g_workingData.numMeasurementsRecorded++;
    }
    
    // if we still have more measurements to take, then go back to sleep
    if (g_workingData.numMeasurementsRecorded < WorkingData::kNumMeasurementsToTakeBeforeSending)
    {
        esp_sleep_enable_timer_wakeup(WorkingData::kTimeBetweenMeasurements_ms * 1000);
        esp_deep_sleep_start();
    }

    // if we got this far, it's time to transmit data over wifi

    // low power wifi
    WiFi.setSleep(true);

    // try to read stored SSID and PWD from NVS in order to connect
    initNVS();

    char ssid[MAX_SSID_LENGTH];
    char pwd[MAX_PWD_LENGTH];
    memset(ssid, 0, MAX_SSID_LENGTH);
    memset(pwd, 0, MAX_PWD_LENGTH);
    const bool hasStoredDetails = tryReadSSIDPW(ssid, pwd);
    if (!hasStoredDetails)
    {
        Serial.println("No saved details, starting access point...");

        uint8_t mac[6];
        char buff[128];
        esp_wifi_get_mac(WIFI_IF_AP, mac);
        sprintf(buff, "T-Higrow-%02X:%02X", mac[4], mac[5]);
        WiFi.softAP(buff);
        Serial.println(buff);
    }
    else
    {
        WiFi.mode(WIFI_STA);
        wifi_config_t current_conf;
        esp_wifi_get_config(WIFI_IF_STA, &current_conf);
        int ssidlen = strlen((char *)(current_conf.sta.ssid));
        int passlen = strlen((char *)(current_conf.sta.password));
        if (ssidlen == 0 || passlen == 0)
        {
            multi.addAP(ssid, pwd);
            Serial.println("Connect to defalut ssid, you can long press BOOT button enter smart config mode");
            while (multi.run() != WL_CONNECTED)
            {
                Serial.print('.');
            }
        }
        else
        {
            WiFi.begin();
        }
        if (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
            Serial.printf("WiFi connect fail!,please restart retry,or long press BOOT button enter smart config mode\n");
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());

            // get global time if necessary
            if (!g_workingData.hasGlobalTimeReference)
            {
                if (tryToGetGlobalTime())
                {
                    g_workingData.globalTimeReference = globalTimeReference();
                    g_workingData.hasGlobalTimeReference = true;
                }
            }

            // now send them all
            mqttClient.setServer(kMQTTBroker, kMQTTBrokerPort);
            Serial.println("Connecting MQTT client...");
            while (!mqttClient.connected())
            {
                if (mqttClient.connect(g_mqttName))
                {
                    Serial.print("MQTT client connected as '");
                    Serial.print(g_mqttName);
                    Serial.println("'");
                    break;
                }

                Serial.print("Failed to connect MQTT client.  State = ");
                Serial.print(mqttClient.state());
                Serial.println(". Retrying in 5s...");
                delay(5000);
            }

            // we're connected, now send!
            char topicBuffer[100];
            for (size_t i = 0; i < g_workingData.numMeasurementsRecorded; ++i)
            {
                makeTopicString(topicBuffer, 100, g_mqttTopicRoot, "lux");
                mqttClient.publish(topicBuffer, "0.0");
            }

            // mark all as sent so we'll measure a new batch
            g_workingData.numMeasurementsRecorded = 0;

            mqttClient.disconnect();
            WiFi.disconnect();
        }
    }

    // finally, go back to sleep
    esp_sleep_enable_timer_wakeup(WorkingData::kTimeBetweenMeasurements_ms * 1000);
    esp_deep_sleep_start();
}

void loop()
{
}