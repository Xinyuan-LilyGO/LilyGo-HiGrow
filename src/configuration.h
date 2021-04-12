#pragma once

/*
    The default is hotspot mode, the note will be connected to the configured WiFi
*/
// #define SOFTAP_MODE



/*
    Sensor selection, the default is the onboard sensor
*/
// #define __HAS_BME280__               //BME280 temperature, humidity, pressure, height sensor
// #define __HAS_MOTOR__                //High and low level control relay, or other level drive motor
// #define __HAS_RGB__                  //WS2812 single point colorful light


// Only one of the following two can be selected, otherwise there will be conflicts
// #define __HAS_SHT3X__                //SHT3X temperature and humidity sensor
// #define __HAS_DS18B20__              //DS18B20 temperature and humidity sensor


// Wireless access point ssid password
#define WIFI_SSID               "XXXXXXXX"
#define WIFI_PASSWD             "12345678"


#define I2C_SDA                 (25)
#define I2C_SCL                 (26)
#define DHT12_PIN               (16)
#define BAT_ADC                 (33)
#define SALT_PIN                (34)
#define SOIL_PIN                (32)
#define BOOT_PIN                (0)
#define POWER_CTRL              (4)
#define USER_BUTTON             (35)
#define DS18B20_PIN             (21)                  //18b20 data pin

#define MOTOR_PIN               (19)
#define RGB_PIN                 (18)

#define OB_BH1750_ADDRESS       (0x23)
#define OB_BME280_ADDRESS       (0x77)
#define OB_SHT3X_ADDRESS        (0x44)

#ifdef _USE_CN_
#define DASH_BME280_TEMPERATURE_STRING  "BME 温度"
#define DASH_BME280_PRESSURE_STRING     "BME 压力"
#define DASH_BME280_ALTITUDE_STRING     "BME 高度"
#define DASH_BME280_HUMIDITY_STRING     "BME 湿度"
#define DASH_DHT_TEMPERATURE_STRING     "DHT1x 温度"
#define DASH_DHT_HUMIDITY_STRING        "DHT1x 湿度"
#define DASH_BH1750_LUX_STRING          "BH1750 亮度"
#define DASH_SOIL_VALUE_STRING          "土壤湿度百分比"
#define DASH_SALT_VALUE_STRING          "盐分百分比"
#define DASH_BATTERY_STRING             "电池电压"
#define DASH_DS18B20_STRING             "18B20 温度"
#define DASH_SHT3X_TEMPERATURE_STRING   "SHT3X 温度"
#define DASH_SHT3X_HUMIDITY_STRING      "SHT3X 湿度"
#define DASH_MOTOR_CTRL_STRING          "水泵"
#else
#define DASH_BME280_TEMPERATURE_STRING  "BME Temperature"
#define DASH_BME280_PRESSURE_STRING     "BME Pressure"
#define DASH_BME280_ALTITUDE_STRING     "BME Altitude"
#define DASH_BME280_HUMIDITY_STRING     "BME Humidity"
#define DASH_DHT_TEMPERATURE_STRING     "DHT1x Temperature"
#define DASH_DHT_HUMIDITY_STRING        "DHT1x Humidity"
#define DASH_BH1750_LUX_STRING          "BH1750"
#define DASH_SOIL_VALUE_STRING          "Soil Value"
#define DASH_SALT_VALUE_STRING          "Salt Value"
#define DASH_BATTERY_STRING             "Battery"
#define DASH_DS18B20_STRING             "18B20 Temperature"
#define DASH_SHT3X_TEMPERATURE_STRING   "SHT3X Temperature"
#define DASH_SHT3X_HUMIDITY_STRING      "SHT3X Humidity"
#define DASH_MOTOR_CTRL_STRING          "Water pump"

#endif  /*_USE_EN_*/

