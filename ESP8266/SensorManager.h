#pragma once

#include <Adafruit_ADS1X15.h>
#include <Adafruit_INA260.h>

class SensorManager {
public:
    SensorManager();
    void begin();

    void readAll(float &c1, float &c2, float &c3, float &c4,
                 float &v1, float &v2, float &v3, float &v4,
                 float &vSupply);

    float readVoltageSensor(Adafruit_ADS1115 &adc, uint8_t ch);
    float readUsb1Current();
    float readUsb2Current();
    float readUsb3Current();
    float readUsb4Current();
    float readSupplyVoltage();

private:
    Adafruit_ADS1115 ads1;
    Adafruit_INA260 ina1;
    Adafruit_INA260 ina2;
    Adafruit_INA260 ina3;
    Adafruit_INA260 ina4;

};
