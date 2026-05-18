#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class LCDManager {
public:
    LCDManager();
    void begin();

    void update(float v1, float v2, float v3, float v4,
            float c1, float c2, float c3, float c4,
            float vSupply,
            String usbState[4],
            bool scheduleEnabled[4],
            unsigned long schedStart[4],
            unsigned long schedEnd[4],
            int schedTarget[4],
            unsigned long epoch,
            bool schedAck);  


private:
    LiquidCrystal_I2C lcd;
};
