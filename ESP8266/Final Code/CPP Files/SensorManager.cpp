//-----------------------------
// Title: SensorManager.cpp
//-----------------------------
// Purpose: Implements sensor initialization and reading for four USB outputs.
// It reads USB current from INA260 sensors, USB voltage from ADS1115 channels,
// and supply/battery voltage from analog input A0.
// Dependencies: Arduino.h, SensorManager.h
// Compiler: Platform IO
// Author: Juan Jimenez
// OUTPUTS: Current in amps, USB voltage, calibrated supply voltage
// INPUTS: INA260 sensors at I2C addresses 0x40, 0x41, 0x44, 0x45;
//         ADS1115 at 0x48; analog input A0
// Versions:
//      V1.0: 5/18/2026 - Added current, voltage, and supply voltage sensor reading
//-----------------------------

#include <Arduino.h>
#include "SensorManager.h"

SensorManager::SensorManager() {}

void SensorManager::begin() {
    ads1.begin(0x48);
    ads1.setGain(GAIN_ONE);

    ina1.begin(0x40);
    ina2.begin(0x41);
    ina3.begin(0x44);
    ina4.begin(0x45);
}

float SensorManager::readVoltageSensor(Adafruit_ADS1115 &adc, uint8_t ch) {
    int16_t raw = adc.readADC_SingleEnded(ch);
    float v_adc = raw * 0.000125f;

    const float DIVIDER_GAIN = 5.0f;

    float v_usb = v_adc * DIVIDER_GAIN;
    if (v_usb < 0) v_usb = 0;
    if (v_usb > 20) v_usb = 20;

    return v_usb;
}

float SensorManager::readUsb1Current() {
    float mA = ina1.readCurrent();
    float A = -(mA / 1000.0f);
    return A;
}
float SensorManager::readUsb2Current() {
    float mA = ina2.readCurrent();
    float A = -(mA / 1000.0f);
    return A;
}
float SensorManager::readUsb3Current() {
    float mA = ina3.readCurrent();
    float A = -(mA / 1000.0f);
    return A;
}
float SensorManager::readUsb4Current() {
    float mA = ina4.readCurrent();
    float A = -(mA / 1000.0f);
    return A;
}


float SensorManager::readSupplyVoltage() {
    int raw = analogRead(A0);

    float v_adc = (raw / 1023.0f) * 3.3f;

    const float SUPPLY_DIVIDER_GAIN = 5.00f;
    float v_measured = v_adc * SUPPLY_DIVIDER_GAIN;

    // Calibration from your measured data:
    // Vactual ≈ 0.9587 * Vmeasured + 0.0835
    float v_supply = (0.954f * v_measured);

    if (v_supply < 0) v_supply = 0;
    if (v_supply > 20) v_supply = 20;

    return v_supply;
}


void SensorManager::readAll(
    float &c1, float &c2, float &c3, float &c4,
    float &v1, float &v2, float &v3, float &v4,
    float &vSupply
) {
    c1 = readUsb1Current();
    c2 = readUsb2Current();
    c3 = readUsb3Current();
    c4 = readUsb4Current();

    v1 = readVoltageSensor(ads1, 0);
    v2 = readVoltageSensor(ads1, 1);
    v3 = readVoltageSensor(ads1, 2);
    v4 = readVoltageSensor(ads1, 3);

    vSupply = readSupplyVoltage();
}
