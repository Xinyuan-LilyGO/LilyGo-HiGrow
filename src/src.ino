#include <algorithm>
#include <iostream>
#include <WiFi.h>
#include <AsyncTCP.h>           //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>  //https://github.com/me-no-dev/ESPAsyncWebServer
#include <ESPDash.h>            //https://github.com/ayushsharma82/ESP-DASH
#include <Button2.h>            //https://github.com/LennartHennigs/Button2
#include <BH1750.h>             //https://github.com/claws/BH1750
#include <cJSON.h>
#include <OneWire.h>
#include <LoRa.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_NeoPixel.h>
#include "DHT.h"
#include "configuration.h"


typedef enum {
    BME280_SENSOR_ID,
    DHTxx_SENSOR_ID,
    SHT3x_SENSOR_ID,
    BHT1750_SENSOR_ID,
    SOIL_SENSOR_ID,
    SALT_SENSOR_ID,
    DS18B20_SENSOR_ID,
    VOLTAGE_SENSOR_ID,
} sensor_id_t;

typedef struct {
    uint32_t timestamp;     /**< time is in milliseconds */
    float temperature;      /**< temperature is in degrees centigrade (Celsius) */
    float light;            /**< light in SI lux units */
    float pressure;         /**< pressure in hectopascal (hPa) */
    float humidity;         /**<  humidity in percent */
    float altitude;         /**<  altitude in m */
    float voltage;           /**< voltage in volts (V) */
    uint8_t soli;           //Percentage of soil
    uint8_t salt;           //Percentage of salt
} higrow_sensors_event_t;


AsyncWebServer      server(80);
ESPDash             dashboard(&server);

Button2             button(BOOT_PIN);
Button2             useButton(USER_BUTTON);

BH1750              lightMeter(OB_BH1750_ADDRESS);  //0x23
DHT                 dht(DHT1x_PIN, DHTTYPE);
OneWire             ds;
Adafruit_SHT31      sht31 = Adafruit_SHT31(&Wire1);
Adafruit_BME280     bme;                            //0x77


Adafruit_NeoPixel *pixels = NULL;

bool                    has_lora_shield = false;
bool                    has_bmeSensor   = false;
bool                    has_lightSensor = false;
bool                    has_dhtSensor   = false;
bool                    has_sht3xSensor = false;
bool                    has_ds18b20     = false;
bool                    has_dht11       = false;

uint64_t                timestamp       = 0;
uint8_t                 ds18b20Addr[8];
uint8_t                 ds18b20Type;

Card *dhtTemperature    = NULL;
Card *dhtHumidity       = NULL;
Card *saltValue         = new Card(&dashboard, GENERIC_CARD, DASH_SALT_VALUE_STRING, "%");
Card *batteryValue      = new Card(&dashboard, GENERIC_CARD, DASH_BATTERY_STRING, "mV");
Card *soilValue         = new Card(&dashboard, GENERIC_CARD, DASH_SOIL_VALUE_STRING, "%");

Card *illumination      = NULL;
Card *bmeTemperature    = NULL;
Card *bmeHumidity       = NULL;
Card *bmeAltitude       = NULL;
Card *bmePressure       = NULL;
Card *dsTemperature     = NULL;
Card *sht3xTemperature  = NULL;
Card *sht3xHumidity     = NULL;
Card *motorButton       = NULL;

void    setupLoRa();
void    loopLoRa(higrow_sensors_event_t *val);
void    deviceProbe(TwoWire &t);
float   getDsTemperature(void);

void smartConfigStart(Button2 &b)
{
    Serial.println("esp touch is started in network distribution mode, please download esp touch application for network configuration!");
    Serial.println("Appliction link: https://github.com/EspressifApp/EsptouchForAndroid/releases");
    Serial.println();
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
    if (has_lora_shield) {
        LoRa.sleep();
        SPI.end();
    }

    Serial.println("Enter Deepsleep ...");
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
    delay(1000);
    esp_deep_sleep_start();
}

void setupWiFi()
{
#ifdef SOFTAP_MODE
    Serial.println("Configuring access point...");
    uint8_t mac[6];
    char buff[128];
    WiFi.macAddress(mac);
    sprintf(buff, "T-Higrow-%02X:%02X", mac[4], mac[5]);
    WiFi.softAP(buff);
    Serial.printf("The hotspot has been established, please connect to the %s and output 192.168.4.1 in the browser to access the data page \n", buff);
#else
    WiFi.mode(WIFI_STA);

    Serial.print("Connect SSID:");
    Serial.print(WIFI_SSID);
    Serial.print(" Password:");
    Serial.println(WIFI_PASSWD);

    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi connect fail!");
        delay(3000);
        esp_restart();
    }
    Serial.print("WiFi connect success ! , ");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
#endif
    server.begin();
}

bool get_higrow_sensors_event(sensor_id_t id, higrow_sensors_event_t &val)
{
    switch (id) {
    case BME280_SENSOR_ID: {
        val.temperature = bme.readTemperature();
        val.humidity = (bme.readPressure() / 100.0F);
        val.altitude = bme.readAltitude(1013.25);
    }
    break;

    case SHT3x_SENSOR_ID: {
        float t = sht31.readTemperature();
        float h = sht31.readHumidity();
        if (! isnan(t)) {  // check if 'is not a number'
            Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
        } else {
            Serial.println("Failed to read temperature");
        }
        if (! isnan(h)) {  // check if 'is not a number'
            Serial.print("Hum. % = "); Serial.println(h);
        } else {
            Serial.println("Failed to read humidity");
        }
        val.temperature = t;
        val.humidity = h;
    }
    break;

    case DHTxx_SENSOR_ID: {
        val.temperature = dht.readTemperature();
        val.humidity = dht.readHumidity();
        if (isnan(val.temperature)) {
            val.temperature = 0.0;
        }
        if (isnan(val.humidity)) {
            val.humidity = 0.0;
        }
    }
    break;

    case BHT1750_SENSOR_ID: {
        val.light = lightMeter.readLightLevel();
        if (isnan(val.light)) {
            val.light = 0.0;
        }
    }
    break;

    case SOIL_SENSOR_ID: {
        uint16_t soil = analogRead(SOIL_PIN);
        val.soli = map(soil, 0, 4095, 100, 0);
    }
    break;

    case SALT_SENSOR_ID: {
        uint8_t samples = 120;
        uint32_t humi = 0;
        uint16_t array[120];
        for (int i = 0; i < samples; i++) {
            array[i] = analogRead(SALT_PIN);
            delay(2);
        }
        std::sort(array, array + samples);
        for (int i = 1; i < samples - 1; i++) {
            humi += array[i];
        }
        humi /= samples - 2;
        val.salt = humi;
    }
    break;

    case DS18B20_SENSOR_ID: {
        val.temperature = getDsTemperature();
        if (isnan(val.temperature) || val.temperature > 125.0) {
            val.temperature = 0;
        }
    }
    break;

    case VOLTAGE_SENSOR_ID: {
        int vref = 1100;
        uint16_t volt = analogRead(BAT_ADC);
        val.voltage = ((float)volt / 4095.0) * 6.6 * (vref);
    }
    break;
    default:
        break;
    }
    return true;
}



bool ds18b20Begin()
{
    uint8_t i;

    ds.begin(DS18B20_PIN);

    if (!ds.search(ds18b20Addr)) {
        ds.reset_search();
        return false;
    }

    Serial.print("ROM =");
    for ( i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(ds18b20Addr[i], HEX);
    }

    if (OneWire::crc8(ds18b20Addr, 7) != ds18b20Addr[7]) {
        Serial.println("CRC is not valid!");
        return false;
    }
    Serial.println();

    // the first ROM byte indicates which chip
    switch (ds18b20Addr[0]) {
    case 0x10:
        Serial.println("  Chip = DS18S20");  // or old DS1820
        ds18b20Type = 1;
        break;
    case 0x28:
        Serial.println("  Chip = DS18B20");
        ds18b20Type = 0;
        break;
    case 0x22:
        Serial.println("  Chip = DS1822");
        ds18b20Type = 0;
        break;
    default:
        Serial.println("Device is not a DS18x20 family device.");
        return false;
    }

    return true;
}


float getDsTemperature(void)
{
    uint8_t data[9];
    uint8_t present;
    float celsius, fahrenheit;
    ds.reset();
    ds.select(ds18b20Addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end

    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.

    present = ds.reset();
    ds.select(ds18b20Addr);
    ds.write(0xBE);         // Read Scratchpad

    // Serial.print("  Data = ");
    // Serial.print(present, HEX);
    // Serial.print(" ");
    for (int i = 0; i < 9; i++) {           // we need 9 bytes
        data[i] = ds.read();
        // Serial.print(data[i], HEX);
        // Serial.print(" ");
    }
    // Serial.print(" CRC=");
    // Serial.print(OneWire::crc8(data, 8), HEX);
    // Serial.println();

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (ds18b20Type) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = (float)raw / 16.0;
    // fahrenheit = celsius * 1.8 + 32.0;
    // Serial.print("  Temperature = ");
    // Serial.print(celsius);
    // Serial.print(" Celsius, ");
    // Serial.print(fahrenheit);
    // Serial.println(" Fahrenheit");
    return celsius;
}

bool dhtSensorProbe()
{
    dht.begin();
    delay(2000);// Wait a few seconds between measurements.
    int i = 5;
    while (i--) {
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        // Check if any reads failed and exit early (to try again).
        if (isnan(h)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            return true;
        }
        delay(500);
    }
    return false;
}
#ifdef AUTO_WATER
void WateringCallback(bool value)
{
    Serial.println("motorButton Triggered: " + String((value) ? "true" : "false"));
    digitalWrite(MOTOR_PIN, value);
    pixels->setPixelColor(0, value ? 0x00FF00 : 0);
    pixels->show();
    motorButton->update(value);
    dashboard.sendUpdates();
}
#endif  /*__HAS_MOTOR__*/

void setup()
{
    Serial.begin(115200);

    button.setLongClickHandler(smartConfigStart);
    useButton.setLongClickHandler(sleepHandler);

    /* *
    * Warning:
    *   Higrow sensor power control pin, use external port and onboard sensor, IO4 must be set high
    */

    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, HIGH);
    delay(100);

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire1.begin(I2C1_SDA, I2C1_SCL);

    Serial.println("-------------Devices probe-------------");
    deviceProbe(Wire);
    deviceProbe(Wire1);

    //Check DHT11 temperature and humidity sensor
    if (!dhtSensorProbe()) {
        has_dht11 = false;
        Serial.println("Warning: Failed to find DHT11 temperature and humidity sensor!");
    } else {
        has_dht11 = true;
        Serial.println("DHT11 temperature and humidity sensor init succeeded, using DHT11");
        dhtHumidity     = new Card(&dashboard, HUMIDITY_CARD, DASH_DHT_HUMIDITY_STRING, "%");
        dhtTemperature  = new Card(&dashboard, TEMPERATURE_CARD, DASH_DHT_TEMPERATURE_STRING, "°C");
    }

    //Check SHT3x temperature and humidity sensor
    if (has_sht3xSensor) {
        if (!sht31.begin(OB_SHT3X_ADDRESS)) {   // Set to 0x45 for alternate i2c addr
            Serial.println("Warning: Failed to find SHT3x temperature and humidity sensor!");
        } else {
            has_sht3xSensor = false;
            Serial.println("SHT3X temperature and humidity sensor init succeeded, using SHT3X");
            sht3xTemperature = new Card(&dashboard, TEMPERATURE_CARD, DASH_SHT3X_TEMPERATURE_STRING, "°C");
            sht3xHumidity    = new Card(&dashboard, HUMIDITY_CARD, DASH_SHT3X_HUMIDITY_STRING, "%");
        }
    } else {
        // Destroy Wire1 if SHT3x sensor does not exist
        Wire1.end();
        // If initialization fails, look for a One-Wire sensor present
        if (ds18b20Begin()) {
            has_ds18b20 = true;
            dsTemperature = new Card(&dashboard, TEMPERATURE_CARD, DASH_DS18B20_STRING, "°C");
            Serial.println("DS18B20 temperature sensor init succeeded, using DS18B20");
        } else {
            has_ds18b20 = false;
            Serial.println("Warning: Failed to find DS18B20 temperature sensor!");
        }
    }

    //Check DHT11 or DHT20 temperature and humidity sensor
    if (has_lightSensor) {
        if (!lightMeter.begin()) {
            has_lightSensor = false;
            Serial.println("Warning: Failed to find BH1750 light sensor!");
        } else {
            Serial.println("BH1750 light sensor init succeeded, using BH1750");
            illumination = new Card(&dashboard, GENERIC_CARD, DASH_BH1750_LUX_STRING, "lx");
        }
    }

    //Check BME280 temperature and humidity sensor
    if (has_bmeSensor) {
        if (!bme.begin()) {
            Serial.println("Warning: Failed to find BMP280 temperature and humidity sensor");
        } else {
            Serial.println("BMP280 temperature and humidity sensor init succeeded, using BMP280");
            has_bmeSensor   = true;
            bmeTemperature  = new Card(&dashboard, TEMPERATURE_CARD, DASH_BME280_TEMPERATURE_STRING, "°C");
            bmeHumidity     = new Card(&dashboard, HUMIDITY_CARD,   DASH_BME280_HUMIDITY_STRING, "%");
            bmeAltitude     = new Card(&dashboard, GENERIC_CARD,    DASH_BME280_ALTITUDE_STRING, "m");
            bmePressure     = new Card(&dashboard, GENERIC_CARD,    DASH_BME280_PRESSURE_STRING, "hPa");
        }
    }

    // Try initializing the LoRa Shield, if it exists!
    setupLoRa();

    /* *
     *  IO18 uses the same pins as LoRa Shield. If the initialization of LoRa Sheild fails,
     *  IO18 is initialized as RGB pixel pin by default, and IO19 is initialized as motor drive pin
     */
    if (has_lora_shield) {
        Serial.println("LoRa shield init succeeded, using SX1276 Rado");
    } else {
        Serial.println("LoRa Shield is not found, initialize IO18 as RGB pixel pin, and IO19 as motor drive pin");

        // IO18 is initialized as RGB pixel pin
        pixels = new  Adafruit_NeoPixel(1, RGB_PIN, NEO_GRB + NEO_KHZ800);

        if (pixels) {
            pixels->begin();
            pixels->setPixelColor(0, 0xFF0000);
            pixels->setBrightness(120);
            pixels->show();
        }

        // IO19 is initialized as motor drive pin
        pinMode(MOTOR_PIN, OUTPUT);
        digitalWrite(MOTOR_PIN, LOW);

        motorButton = new Card(&dashboard, BUTTON_CARD, DASH_MOTOR_CTRL_STRING);

        motorButton->attachCallback([&](bool value) {

            Serial.println("motorButton Triggered: " + String((value) ? "true" : "false"));

            digitalWrite(MOTOR_PIN, value);

            if (pixels) {
                pixels->setPixelColor(0, value ? 0x00FF00 : 0);
                pixels->show();
            }

            motorButton->update(value);

            dashboard.sendUpdates();
        });
    }

    setupWiFi();
}



void loop()
{
    button.loop();
    useButton.loop();

    if (millis() - timestamp > 1000) {
        timestamp = millis();

        higrow_sensors_event_t val = {0};

        get_higrow_sensors_event(SOIL_SENSOR_ID, val);
        soilValue->update(val.soli);

        get_higrow_sensors_event(SALT_SENSOR_ID, val);
        saltValue->update(val.salt);

        get_higrow_sensors_event(VOLTAGE_SENSOR_ID, val);
        batteryValue->update(val.voltage);

        if (has_dht11) {
            get_higrow_sensors_event(DHTxx_SENSOR_ID, val);
            dhtTemperature->update(val.temperature);
            dhtHumidity->update(val.humidity);
        }

        if (has_lightSensor) {
            get_higrow_sensors_event(BHT1750_SENSOR_ID, val);
            illumination->update(val.light);
        }

        if (has_bmeSensor) {
            get_higrow_sensors_event(BME280_SENSOR_ID, val);
            bmeTemperature->update(val.temperature);
            bmeHumidity->update(val.humidity);
            bmeAltitude->update(val.altitude);
            bmePressure->update(val.pressure);
        }

        if (has_sht3xSensor) {
            get_higrow_sensors_event(SHT3x_SENSOR_ID, val);
            sht3xTemperature->update(val.temperature);
            sht3xHumidity->update(val.humidity);
        }

        if (has_ds18b20) {
            get_higrow_sensors_event(DS18B20_SENSOR_ID, val);
            dsTemperature->update(val.temperature);
        }

#ifdef AUTO_WATER
        uint16_t soil = analogRead(SOIL_PIN);
        uint16_t soli_val = map(soil, 0, 4095, 100, 0);
        if (soli_val < 26) {
            // Serial.println("Start adding water");
            WateringCallback(true);
        }
        if (soli_val >= 40) {
            // Serial.println("Stop adding water");
            WateringCallback(false);
        }
#endif  /*AUTO_WATER*/

        dashboard.sendUpdates();

        loopLoRa(&val);
    }
}



void deviceProbe(TwoWire &t)
{

    uint8_t err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        t.beginTransmission(addr);
        err = t.endTransmission();
        if (err == 0) {

            switch (addr) {
            case OB_BH1750_ADDRESS:
                has_lightSensor = true;
                Serial.println("BH1750 light sensor found!");
                break;
            case OB_BME280_ADDRESS:
                has_bmeSensor = true;
                Serial.println("BME280 temperature and humidity sensor found!");
                break;
            case OB_SHT3X_ADDRESS:
                has_sht3xSensor = true;
                Serial.println("SHT3X temperature and humidity sensor found!");
                break;
            default:
                Serial.print("I2C device found at address 0x");
                if (addr < 16)
                    Serial.print("0");
                Serial.print(addr, HEX);
                Serial.println(" !");
                break;
            }
            nDevices++;
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16)
                Serial.print("0");
            Serial.println(addr, HEX);
        }
    }
}


void setupLoRa()
{
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    LoRa.setPins(RADIO_CS_PIN, RADIO_RESET_PIN, RADIO_DI0_PIN);
    if (!LoRa.begin(LoRa_frequency)) {
        SPI.end();
        return;
    }
    has_lora_shield = true;
}

void loopLoRa(higrow_sensors_event_t *val)
{
    if (!has_lora_shield)return;

    /**
     *   The transmission JSON format is sent through LoRa,
     *   and the data will be parsed in the receiving node
     */

    cJSON *root =  cJSON_CreateObject();
    cJSON_AddStringToObject(root, "L", String(val->light).c_str());
    cJSON_AddStringToObject(root, "S", String(val->soli).c_str());
    cJSON_AddStringToObject(root, "A", String(val->salt).c_str());
    cJSON_AddStringToObject(root, "V", String(val->voltage).c_str());
    cJSON_AddStringToObject(root, "T", String(val->temperature).c_str());

    LoRa.beginPacket();
    char *packet = cJSON_Print(root);
    LoRa.print(packet);
    LoRa.endPacket();

    cJSON_Delete(root);
}




