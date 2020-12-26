#include <algorithm>
#include <iostream>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <ESPmDNS.h>
#include <Button2.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT12.h>
#include <Adafruit_BME280.h>
#include <WiFiMulti.h>
#include "esp_wifi.h"

#define SOFTAP_MODE
// #define USE_18B20_TEMP_SENSOR
// #define USE_CHINESE_WEB



// Simple ds18b20 class
class DS18B20
{
public:
    DS18B20(int gpio)
    {
        pin = gpio;
    }

    float temp()
    {
        uint8_t arr[2] = {0};
        if (reset()) {
            wByte(0xCC);
            wByte(0x44);
            delay(750);
            reset();
            wByte(0xCC);
            wByte(0xBE);
            arr[0] = rByte();
            arr[1] = rByte();
            reset();
            return (float)(arr[0] + (arr[1] * 256)) / 16;
        }
        return 0;
    }
private:
    int pin;

    void write(uint8_t bit)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        delayMicroseconds(5);
        if (bit)digitalWrite(pin, HIGH);
        delayMicroseconds(80);
        digitalWrite(pin, HIGH);
    }

    uint8_t read()
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        delayMicroseconds(2);
        digitalWrite(pin, HIGH);
        delayMicroseconds(15);
        pinMode(pin, INPUT);
        return digitalRead(pin);
    }

    void wByte(uint8_t bytes)
    {
        for (int i = 0; i < 8; ++i) {
            write((bytes >> i) & 1);
        }
        delayMicroseconds(100);
    }

    uint8_t rByte()
    {
        uint8_t r = 0;
        for (int i = 0; i < 8; ++i) {
            if (read()) r |= 1 << i;
            delayMicroseconds(15);
        }
        return r;
    }

    bool reset()
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        delayMicroseconds(500);
        digitalWrite(pin, HIGH);
        pinMode(pin, INPUT);
        delayMicroseconds(500);
        return digitalRead(pin);
    }
};



#define I2C_SDA             25
#define I2C_SCL             26
#define DHT12_PIN           16
#define BAT_ADC             33
#define SALT_PIN            34
#define SOIL_PIN            32
#define BOOT_PIN            0
#define POWER_CTRL          4
#define USER_BUTTON         35
#define DS18B20_PIN         21                  //18b20 data pin


BH1750 lightMeter(0x23); //0x23
Adafruit_BME280 bmp;     //0x77
DHT12 dht12(DHT12_PIN, true);
AsyncWebServer server(80);
Button2 button(BOOT_PIN);
Button2 useButton(USER_BUTTON);
WiFiMulti multi;
DS18B20 temp18B20(DS18B20_PIN);

#define WIFI_SSID   "your wifi ssid"
#define WIFI_PASSWD "you wifi password"



bool bme_found = false;

void smartConfigStart(Button2 &b)
{
    Serial.println("smartConfigStart...");
    WiFi.disconnect();
    WiFi.beginSmartConfig();
    while (!WiFi.smartConfigDone()) {
        Serial.print(".");
        delay(200);
    }
    WiFi.stopSmartConfig();
    Serial.println();
    Serial.print("smartConfigStop Connected:");
    Serial.print(WiFi.SSID());
    Serial.print("PSW: ");
    Serial.println(WiFi.psk());
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
    if (isBegin) {
        return true;
    }

    ESPDash.init(server);

    isBegin = true;
    if (MDNS.begin("soil")) {
        Serial.println("MDNS responder started");
    }
    // Add Respective Cards
    if (bme_found) {
#ifdef USE_CHINESE_WEB
        ESPDash.addTemperatureCard("temp", "BME传感器温度/C", 0, 0);
        ESPDash.addNumberCard("press", "BME传感器压力/hPa", 0);
        ESPDash.addNumberCard("alt", "BME传感器高度/m", 0);
#else
        ESPDash.addTemperatureCard("temp", "BME Temperature/C", 0, 0);
        ESPDash.addNumberCard("press", "BME Pressure/hPa", 0);
        ESPDash.addNumberCard("alt", "BME Altitude/m", 0);
#endif
    }
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
#endif


#ifdef USE_18B20_TEMP_SENSOR
    ESPDash.addTemperatureCard("temp3", "18B20温度/C", 0, 0);
#endif
    ESPDash.addTemperatureCard("temp3", "18B20 Temperature/C", 0, 0);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    return true;
}

void setup()
{
    Serial.begin(115200);

#ifdef SOFTAP_MODE
    Serial.println("Configuring access point...");
    uint8_t mac[6];
    char buff[128];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    sprintf(buff, "T-Higrow-%02X:%02X", mac[4], mac[5]);
    WiFi.softAP(buff);
#else
    WiFi.mode(WIFI_STA);
    wifi_config_t current_conf;
    esp_wifi_get_config(WIFI_IF_STA, &current_conf);
    int ssidlen = strlen((char *)(current_conf.sta.ssid));
    int passlen = strlen((char *)(current_conf.sta.password));
    if (ssidlen == 0 || passlen == 0) {
        multi.addAP(WIFI_SSID, WIFI_PASSWD);
        Serial.println("Connect to defalut ssid, you can long press BOOT button enter smart config mode");
        while (multi.run() != WL_CONNECTED) {
            Serial.print('.');
        }
    } else {
        WiFi.begin();
    }
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi connect fail!,please restart retry,or long press BOOT button enter smart config mode\n");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
#endif
    button.setLongClickHandler(smartConfigStart);
    useButton.setLongClickHandler(sleepHandler);

    Wire.begin(I2C_SDA, I2C_SCL);

    dht12.begin();

    //! Sensor power control pin , use deteced must set high
    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, 1);
    delay(1000);

    if (!bmp.begin()) {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
        bme_found = false;
    } else {
        bme_found = true;
    }

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println(F("BH1750 Advanced begin"));
    } else {
        Serial.println(F("Error initialising BH1750"));
    }


}


uint32_t readSalt()
{
    uint8_t samples = 120;
    uint32_t humi = 0;
    uint16_t array[120];

    for (int i = 0; i < samples; i++) {
        array[i] = analogRead(SALT_PIN);
        delay(2);
    }
    std::sort(array, array + samples);
    for (int i = 0; i < samples; i++) {
        if (i == 0 || i == samples - 1)continue;
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
    if (millis() - timestamp > 1000 ) {
        timestamp = millis();
        // if (WiFi.status() == WL_CONNECTED) {
        if (serverBegin()) {
            float lux = lightMeter.readLightLevel();

            if (bme_found) {
                float bme_temp = bmp.readTemperature();
                float bme_pressure = (bmp.readPressure() / 100.0F);
                float bme_altitude = bmp.readAltitude(1013.25);
                ESPDash.updateTemperatureCard("temp", (int)bme_temp);
                ESPDash.updateNumberCard("press", (int)bme_pressure);
                ESPDash.updateNumberCard("alt", (int)bme_altitude);
            }

            float t12 = dht12.readTemperature();
            // Read temperature as Fahrenheit (isFahrenheit = true)
            float h12 = dht12.readHumidity();


            if (!isnan(t12) && !isnan(h12) ) {
                ESPDash.updateTemperatureCard("temp2", (int)t12);
                ESPDash.updateHumidityCard("hum2", (int)h12);
            }
            ESPDash.updateNumberCard("lux", (int)lux);

            uint16_t soil = readSoil();
            uint32_t salt = readSalt();
            float bat = readBattery();
            ESPDash.updateHumidityCard("soil", (int)soil);
            ESPDash.updateNumberCard("salt", (int)salt);
            ESPDash.updateNumberCard("batt", (int)bat);

#ifdef USE_18B20_TEMP_SENSOR
            //Single data stream upload
            float temp = temp18B20.temp();
            ESPDash.updateTemperatureCard("temp3", (int)temp);
#endif
        }
    }
}