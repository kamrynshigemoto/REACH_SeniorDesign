//-----------------------------
// Title: Batteryestimator.cpp
//-----------------------------
// Purpose: Implements battery runtime and state-of-charge estimation.
// It uses piecewise voltage curves for LiFePO4 and Lead-Acid batteries, converts
// supply voltage into estimated state-of-charge, and estimates remaining runtime
// using battery capacity and total current draw.
// Dependencies: BatteryEstimator.h
// Compiler:PlatformIO for ESP8266
// Author: Juan Jimenez
// OUTPUTS: Estimated battery SoC and runtime remaining in hours
// INPUTS: Battery voltage, battery chemistry, battery amp-hour capacity,
//         total system current draw
// Versions:
//      V1.0: 5/18/2026 - Added LFP/Lead-Acid SoC curves and runtime estimation
//-----------------------------
#include "BatteryEstimator.h"

// ---------------------------------------------------------------------------
// Piecewise-linear helper
// Maps v in [v0, v1] → SoC in [s0, s1].  Clamps at both ends.
// ---------------------------------------------------------------------------
static float lerp(float v, float v0, float v1, float s0, float s1) {
    if (v >= v1) return s1;
    if (v <= v0) return s0;
    return s0 + (s1 - s0) * (v - v0) / (v1 - v0);
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
BatteryEstimator::BatteryEstimator()
    : battAh(0.0f), battChem(BATT_LFP), _configured(false) {}
    // Note: battChem defaults to BATT_LFP so voltageToSoC() gives a
    // meaningful reading for the most common battery type even before
    // the Boron sends the BATT config message.

// ---------------------------------------------------------------------------
// setConfig / isConfigured
// ---------------------------------------------------------------------------
void BatteryEstimator::setConfig(float ah, BattChemistry chem) {
    battAh     = ah;
    battChem   = chem;
    _configured = true;
}

bool BatteryEstimator::isConfigured() const {
    return _configured;
}

// ---------------------------------------------------------------------------
// voltageToSoC
//
// LiFePO4 curve (12.8 V nominal) — derived from the LFP discharge data in
// the Gemini conversation used as reference.
//
//   V      SoC
//  13.6+   100 %
//  13.4    95 %
//  13.3    90 %
//  13.2    75 %   ← start of the long "plateau"
//  13.1    55 %
//  13.0    35 %
//  12.9    25 %
//  12.8    17 %   ← "low" threshold; cliff begins below here
//  12.5    10 %
//  12.0     5 %
//  10.0     0 %
//
// Lead-Acid / AGM curve (12 V nominal):
//   V      SoC
//  12.7+   100 %
//  12.4     75 %
//  12.2     50 %
//  12.0     25 %
//  10.5      0 %
// ---------------------------------------------------------------------------
float BatteryEstimator::voltageToSoC(float v) const {
    if (battChem == BATT_LFP) {
        if (v >= 13.6f) return 1.00f;
        if (v >= 13.4f) return lerp(v, 13.4f, 13.6f, 0.95f, 1.00f);
        if (v >= 13.3f) return lerp(v, 13.3f, 13.4f, 0.90f, 0.95f);
        if (v >= 13.2f) return lerp(v, 13.2f, 13.3f, 0.75f, 0.90f);
        if (v >= 13.1f) return lerp(v, 13.1f, 13.2f, 0.55f, 0.75f);
        if (v >= 13.0f) return lerp(v, 13.0f, 13.1f, 0.35f, 0.55f);
        if (v >= 12.9f) return lerp(v, 12.9f, 13.0f, 0.25f, 0.35f);
        if (v >= 12.8f) return lerp(v, 12.8f, 12.9f, 0.17f, 0.25f);
        if (v >= 12.5f) return lerp(v, 12.5f, 12.8f, 0.10f, 0.17f);
        if (v >= 12.0f) return lerp(v, 12.0f, 12.5f, 0.05f, 0.10f);
        if (v >= 10.0f) return lerp(v, 10.0f, 12.0f, 0.00f, 0.05f);
        return 0.0f;

    } else {
        // Lead-Acid / AGM
        if (v >= 12.7f) return 1.00f;
        if (v >= 12.4f) return lerp(v, 12.4f, 12.7f, 0.75f, 1.00f);
        if (v >= 12.2f) return lerp(v, 12.2f, 12.4f, 0.50f, 0.75f);
        if (v >= 12.0f) return lerp(v, 12.0f, 12.2f, 0.25f, 0.50f);
        if (v >= 10.5f) return lerp(v, 10.5f, 12.0f, 0.00f, 0.25f);
        return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// estimateRuntime
//
// Formula:
//   remainingAh = battAh * SoC
//   For Lead-Acid: subtract the 50 % reserve (never discharge below 50 %)
//   Runtime (h) = remainingAh / totalCurrentA
// ---------------------------------------------------------------------------
float BatteryEstimator::estimateRuntime(float vSupply, float totalCurrentA) const {
    if (!_configured)         return -1.0f;
    if (totalCurrentA <= 0.0f) return -1.0f;

    float soc         = voltageToSoC(vSupply);
    float remainingAh = battAh * soc;

    // Lead-acid health: never deplete below 50 % of rated capacity
    if (battChem == BATT_LEAD) {
        float reserveAh = battAh * 0.50f;
        remainingAh -= reserveAh;
        if (remainingAh < 0.0f) remainingAh = 0.0f;
    }

    return remainingAh / totalCurrentA;
}
