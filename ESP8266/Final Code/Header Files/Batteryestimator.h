//-----------------------------
// Title: Batteryestimator.h
//-----------------------------
// Purpose: Declares the BatteryEstimator class used to estimate battery state-of-charge
// and runtime remaining based on battery chemistry, battery capacity, supply voltage,
// and total current draw.
// Dependencies: Arduino.h
// Compiler:PlatformIO for ESP8266
// Author: Juan Jimenez
// OUTPUTS: Runtime estimate in hours, battery state-of-charge estimate
// INPUTS: Battery amp-hour capacity, battery chemistry, supply voltage, total current
// Versions:
//      V1.0: 5/18/2026 - Declared battery configuration, SoC, and runtime estimation interface
//-----------------------------
#pragma once
#include <Arduino.h>

// Supported battery chemistries
enum BattChemistry {
    BATT_LFP,   // LiFePO4  (12.8 V nominal)
    BATT_LEAD   // Lead-Acid / AGM (12 V nominal)
};

class BatteryEstimator {
public:
    BatteryEstimator();

    // Called once when the Boron sends battery config at boot.
    // ah   : rated capacity in Amp-hours (e.g. 15.0)
    // chem : BATT_LFP or BATT_LEAD
    void setConfig(float ah, BattChemistry chem);

    // Returns true after setConfig() has been called.
    bool isConfigured() const;

    // Estimate hours of runtime remaining.
    // vSupply      : live supply rail voltage (V)
    // totalCurrentA: total load current in Amps (all outlets + base draw)
    // Returns -1 if not configured or current == 0.
    float estimateRuntime(float vSupply, float totalCurrentA) const;

    // Convert resting voltage to State-of-Charge [0.0 .. 1.0].
    // Exposed so main / LCD can display a % if needed.
    float voltageToSoC(float v) const;

    // Public so BoronComm can read them back if needed
    float       battAh;
    BattChemistry battChem;

private:
    bool _configured;
};
