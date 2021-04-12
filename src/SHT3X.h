#ifndef __SHT3X_H
#define __HT3X_H

#include <Arduino.h>
#include <Wire.h>

class SHT3X
{
public:
    SHT3X(uint8_t address = 0x44)
    {
        _address = address;
    }
    bool get(void)
    {
        unsigned int data[6];

        Wire1.beginTransmission(_address);
        Wire1.write(0x2C);
        Wire1.write(0x06);
        if (Wire1.endTransmission() != 0)
            return false;
        delay(500);
        Wire1.requestFrom(_address, 6U);
        for (int i = 0; i < 6; i++) {
            data[i] = Wire1.read();
        };
        delay(50);
        cTemp = ((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45;
        fTemp = (cTemp * 1.8) + 32;
        humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);
        return true;
    }
    float cTemp = 0;
    float fTemp = 0;
    float humidity = 0;

private:
    uint8_t _address;

};


#endif
