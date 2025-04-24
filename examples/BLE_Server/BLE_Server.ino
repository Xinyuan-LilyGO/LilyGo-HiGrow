/**
 * @file      BLE_Server.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-04-24
 *
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BH1750.h>             //https://github.com/claws/BH1750

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

typedef struct {
    float lux;
    float voltage;
    uint16_t soil;
    uint16_t salt;
} HiGrowData;

#define BAT_ADC                         (33)
#define SALT_PIN                        (34)
#define SOIL_PIN                        (32)
#define I2C_SDA                         (25)
#define I2C_SCL                         (26)
#define POWER_CTRL                      (4)
#define OB_BH1750_ADDRESS               (0x23)
BH1750 lightMeter(OB_BH1750_ADDRESS);  //0x23


class MyServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer)
    {
        deviceConnected = false;
    }
};


void setup()
{
    Serial.begin(115200);


    /* *
    * Warning:
    *   Higrow sensor power control pin, use external port and onboard sensor, IO4 must be set high
    */
    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, HIGH);
    delay(100);

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!lightMeter.begin()) {
        Serial.println("Warning: Failed to find BH1750 light sensor!");
    } else {
        Serial.println("BH1750 light sensor init succeeded, using BH1750");
    }

    // Create the BLE Device
    BLEDevice::init("T-HiGrow");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                          CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_READ   |
                          BLECharacteristic::PROPERTY_WRITE  |
                          BLECharacteristic::PROPERTY_NOTIFY |
                          BLECharacteristic::PROPERTY_INDICATE
                      );

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("Waiting a client connection to notify...");

}


uint32_t upload_interval = 0;



void loop()
{
    // notify changed value
    if (deviceConnected) {
        if (millis() > upload_interval) {
            /*
            * Send data to the host through notification. 
            * The data is sent directly without being processed. Change the data to your own data
            * */
            HiGrowData data;
            data.lux = lightMeter.readLightLevel();
            if (isnan(data.lux)) {
                data.lux = 0.0;
            }
            data.voltage = ((float)analogRead(BAT_ADC) / 4095.0) * 6.6 * (1100);
            data.soil = analogRead(SOIL_PIN);
            data.salt = analogRead(SALT_PIN);
            pCharacteristic->setValue((uint8_t*)&data, sizeof(HiGrowData));
            pCharacteristic->notify();
            Serial.println("Update value..");
            upload_interval = millis() + 3000;
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
