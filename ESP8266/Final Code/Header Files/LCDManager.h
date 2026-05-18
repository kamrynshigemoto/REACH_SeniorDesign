//-----------------------------
// Title: LCDManager.h
//-----------------------------
// Purpose: Declares the LCDManager class used to control a 20x4 I2C LCD display.
// The display shows USB voltage, current, ON/OFF state, supply voltage, and schedule ACK.
// Dependencies: Arduino.h, LiquidCrystal_I2C.h
// Compiler: Arduino IDE / PlatformIO for ESP8266
// Author: Juan Jimenez
// OUTPUTS: LCD display update interface
// INPUTS: USB voltages, USB currents, supply voltage, USB states, schedule data,
//         epoch time, schedule ACK flag
// Versions:
//      V1.0: 5/18/2026 - Declared LCD display manager for USB telemetry dashboard
//-----------------------------
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
