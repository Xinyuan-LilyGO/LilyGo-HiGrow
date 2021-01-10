#include <algorithm>
#include <iostream>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define USE_DASH
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
#include "driver/adc.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define SOFTAP_MODE
// #define USE_18B20_TEMP_SENSOR
// #define USE_CHINESE_WEB

#define I2C_SDA 25
#define I2C_SCL 26
#define DHT12_PIN 16
#define BAT_ADC 33
#define SALT_PIN 34
#define SOIL_PIN 32
#define BOOT_PIN 0
#define POWER_CTRL 4
#define USER_BUTTON 35

BH1750 lightMeter(0x23); //0x23
DHT12 dht12(DHT12_PIN, true);
AsyncWebServer server(80);
Button2 button(BOOT_PIN);
Button2 useButton(USER_BUTTON);
WiFiMulti multi;

constexpr uint32_t kUpdateTime_ms = 5000;
constexpr long kGmtOffset_s = 0;        // offset between GMT and your local time
constexpr int kDaylightOffset_s = 3600; // offset for daylight saving
constexpr char kNtpServer[] = "pool.ntp.org";

bool g_i2cInited = false;
bool g_gotStartupTime = false;
tm g_startupTime;
uint32_t g_startupTime_millis = 0;

bool tryToGetGlobalTime()
{
    configTime(kGmtOffset_s, kDaylightOffset_s, kNtpServer);
    if (!getLocalTime(&g_startupTime))
    {
        Serial.println("Failed to get local time");
        return false;
    }

    g_gotStartupTime = true;
    g_startupTime_millis = millis();

    return true;
}

uint32_t formatTimeAsNumber()
{
    uint32_t now = millis();
    uint32_t diff_ms = now - g_startupTime_millis;
    uint32_t diff_s = diff_ms / 1000;
    uint32_t diffPart_hrs = diff_s / 3600;
    diff_s -= diffPart_hrs * 3600;
    uint32_t diffPart_mins = diff_s / 60;
    diff_s -= diffPart_mins * 60;
    uint32_t diffPart_s = diff_s;
    int hours = g_startupTime.tm_hour + diffPart_hrs;
    int mins = g_startupTime.tm_min + diffPart_mins;
    int secs = g_startupTime.tm_sec + diffPart_s;

    // some parts might have overflowed so need to adjust
    uint32_t extraMins = secs / 60;
    mins += extraMins;
    secs -= extraMins * 60;

    uint32_t extraHours = mins / 60;
    hours += extraHours;
    hours %= 24;
    mins -= extraHours * 60;

    Serial.print(hours);
    Serial.print(":");
    Serial.print(mins);
    Serial.print(":");
    Serial.println(secs);

    uint32_t timeNumber = //
        hours * 10000     //
        + mins * 100      //
        + secs;
    Serial.println(timeNumber);

    return timeNumber;
}

void initNVS()
{
    Serial.println("initNVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        Serial.println("NVS error: ");
        Serial.println(err);
    }
    else
    {
        Serial.println("NVS OK");
    }
}

#define MAX_SSID_LENGTH 20
#define MAX_PWD_LENGTH 30

typedef uint32_t nvs_handle_t;

/// @param[out] ssid should be an char array of MAX_SSID_LENGTH elements
/// @param[out] pwd should be an char array of MAX_PWD_LENGTH elements
/// returns true if values were loaded from memory, false otherwise
bool tryReadSSIDPW(char *ssid, char *pwd)
{
    Serial.println("tryReadSSIDPW");
    nvs_handle_t storage;
    esp_err_t err = nvs_open("store", NVS_READWRITE, &storage);
    if (err != ESP_OK)
    {
        Serial.print("nvs_open: ");
        Serial.println(esp_err_to_name(err));
        return false;
    }
    else
    {
        // Read ssid
        size_t length = MAX_SSID_LENGTH;
        err = nvs_get_str(storage, "ssid", ssid, &length);
        switch (err)
        {
        case ESP_OK:
            Serial.print("SSID was: ");
            Serial.println(ssid);
            break;
        default:
            Serial.print("nvs_get_str(ssid): ");
            Serial.println(esp_err_to_name(err));
            nvs_close(storage);
            return false;
        }

        if (length > MAX_SSID_LENGTH)
        {
            Serial.println("Stored SSID too long");
            nvs_close(storage);
            return false;
        }

        // read pwd
        length = MAX_PWD_LENGTH;
        err = nvs_get_str(storage, "pwd", pwd, &length);
        switch (err)
        {
        case ESP_OK:
            Serial.print("pwd was: ");
            Serial.println(pwd);
            break;
        default:
            Serial.print("nvs_get_str(pwd): ");
            Serial.println(esp_err_to_name(err));
            nvs_close(storage);
            return false;
        }

        if (length > MAX_PWD_LENGTH)
        {
            Serial.println("Stored PWD too long");
            nvs_close(storage);
            return false;
        }

        return true;
    }
}

/// @param[in] ssid should be an char array of maximum MAX_SSID_LENGTH elements.  If null, then the key is erased.
/// @param[in] pwd should be an char array of maximum MAX_PWD_LENGTH elements.  If null, then the key is erased.
/// returns true if values were written to memory, false otherwise
bool writeSSIDPW(const char *ssid, const char *pwd)
{
    Serial.println("writeSSIDPW");
    nvs_handle_t storage;
    esp_err_t err = nvs_open("store", NVS_READWRITE, &storage);
    if (err != ESP_OK)
    {
        Serial.print("nvs_open: ");
        Serial.println(esp_err_to_name(err));
        return false;
    }
    else
    {
        // Write SSID
        if (ssid == nullptr)
        {
            err = nvs_erase_key(storage, "ssid");
            if (err != ESP_OK)
            {
                Serial.print("nvs_erase_key(ssid): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }
        else
        {
            err = nvs_set_str(storage, "ssid", ssid);
            if (err != ESP_OK)
            {
                Serial.print("nvs_set_str(ssid): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }

        // write password
        if (pwd == nullptr)
        {
            err = nvs_erase_key(storage, "pwd");
            if (err != ESP_OK)
            {
                Serial.print("nvs_erase_key(pwd): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }
        else
        {
            err = nvs_set_str(storage, "pwd", pwd);
            if (err != ESP_OK)
            {
                Serial.print("nvs_set_str(pwd): ");
                Serial.println(esp_err_to_name(err));
                nvs_close(storage);
                return false;
            }
        }

        // commit
        err = nvs_commit(storage);
        if (err != ESP_OK)
        {
            Serial.print("nvs_commit: ");
            Serial.println(esp_err_to_name(err));
            nvs_close(storage);
            return false;
        }

        nvs_close(storage);
        return true;
    }
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

void sleepHandler(Button2 &b)
{
    Serial.println("Enter Deepsleep ...");
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
    delay(1000);
    esp_deep_sleep_start();
}

bool serverBegin()
{
    static bool isBegin = false;
    if (isBegin)
    {
        return true;
    }

#ifdef USE_DASH
    ESPDash.init(server);
#endif

    isBegin = true;
    if (MDNS.begin("soil"))
    {
        Serial.println("MDNS responder started");
    }
    // Add Respective Cards
#ifdef USE_DASH
#ifdef USE_CHINESE_WEB
    ESPDash.addTemperatureCard("temp2", "DHT12传感器温度/C", 0, 0);
    ESPDash.addHumidityCard("hum2", "DHT12传感器湿度/%", 0);
    ESPDash.addNumberCard("lux", "BH1750传感器亮度/lx", 0);
    ESPDash.addHumidityCard("soil", "土壤湿度", 0);
    ESPDash.addNumberCard("salt", "水分百分比", 0);
    ESPDash.addNumberCard("batt", "电池电压/mV", 0);
#else
    ESPDash.addTemperatureCard("temp2", "DHT Temperature/C", 0, 0);
    ESPDash.addHumidityCard("hum2", "DHT Humidity/%", 0);
    ESPDash.addNumberCard("lux", "BH1750/lx", 0);
    ESPDash.addHumidityCard("soil", "Soil", 0);
    ESPDash.addNumberCard("salt", "Salt", 0);
    ESPDash.addNumberCard("batt", "Battery/mV", 0);
    ESPDash.addHumidityCard("rssi", "WiFi Strength/dBm", 0); // can't be "NumberCard" because a number card has a value stored as uint16_t (unsigned)
    ESPDash.addHumidityCard("time", "Last update time (HHMMSS)", 0);
#endif
#endif
    server.begin();
    MDNS.addService("http", "tcp", 80);
    return true;
}

void setup()
{
    Serial.begin(115200);

    initNVS();

    // turn on WiFi power saving
    WiFi.setSleep(true);

    // set to low frequency
    setCpuFrequencyMhz(80);
    Serial.print("CPU frequency set to ");
    Serial.print(getCpuFrequencyMhz());
    Serial.println(" MHz");

    // try to read stored SSID and PWD
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
        }
    }

    button.setLongClickHandler(smartConfigStart);
    useButton.setLongClickHandler(sleepHandler);
    useButton.setDoubleClickHandler(smartConfigStart);

    //! Sensor power control pin , use deteced must set high
    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, 1);
    delay(1000);

    g_i2cInited = tryInitI2CAndDevices();
    if (!g_i2cInited)
    {
        Serial.println("Couldn't init I2C");
    }
}

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

void loop()
{
    static uint64_t timestamp;
    button.loop();
    useButton.loop();

    if (millis() - timestamp > kUpdateTime_ms)
    {
        timestamp = millis();
        // if (WiFi.status() == WL_CONNECTED) {
        if (serverBegin())
        {
            // try to get global time if we haven't already
            if (!g_gotStartupTime)
            {
                g_gotStartupTime = tryToGetGlobalTime();
                ESPDash.updateHumidityCard("time", 0);
            }
            else
            {
                uint32_t time = formatTimeAsNumber();
                ESPDash.updateHumidityCard("time", time);
            }

            // try to initi I2C if we didn't already
            if (!g_i2cInited)
            {
                g_i2cInited = tryInitI2CAndDevices();
            }

            // if I2C was initialised, read sensors
            if (g_i2cInited)
            {
                Serial.println("Reading I2C sensors");
                float lux = lightMeter.readLightLevel();
                float temperature = dht12.readTemperature();
                // Read temperature as Fahrenheit (isFahrenheit = true)
                float humidity = dht12.readHumidity();

#ifdef USE_DASH
                if (!isnan(temperature) && !isnan(humidity))
                {
                    ESPDash.updateTemperatureCard("temp2", (int)temperature);
                    ESPDash.updateHumidityCard("hum2", (int)humidity);
                }
                ESPDash.updateNumberCard("lux", (int)lux);
#endif
            }
            // turn the ADC on
            adc_power_on();
            uint16_t soil = readSoil();
            uint32_t salt = readSalt();
            float bat = readBattery();
            adc_power_off();

            int rssi = (int)WiFi.RSSI();
            Serial.print("RSSI: ");
            Serial.println(rssi);
#ifdef USE_DASH
            ESPDash.updateHumidityCard("soil", (int)soil);
            ESPDash.updateNumberCard("salt", (int)salt);
            ESPDash.updateNumberCard("batt", (int)bat);
            ESPDash.updateHumidityCard("rssi", rssi);
#else
            Serial.print("soil ");
            Serial.println(soil);
            Serial.print("salt ");
            Serial.println(salt);
            Serial.print("batt ");
            Serial.println(bat);
#endif
        }
    }
}