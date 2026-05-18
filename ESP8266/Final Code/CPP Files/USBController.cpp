//-----------------------------
// Title: USBController.cpp
//-----------------------------
// Purpose: Implements USB output control for four MOSFET-switched USB ports.
// It decides whether each USB output should be ON or OFF based on emergency-off,
// manual override, schedule window, and default/manual state.
// Dependencies: Arduino.h, USBController.h
// Compiler: Platform IO
// Author: Juan Jimenez
// OUTPUTS: Digital MOSFET control signals, USB state strings, status line
// INPUTS: Schedule data, manual commands, priority settings, emergency-off commands,
//         current epoch time
// Versions:
//      V1.0: 5/18/2026 - Added USB state control, schedules, overrides, priority,
//                        emergency-off handling, and status reporting
//-----------------------------

/*#include <Arduino.h>
#include "USBController.h"


USBController::USBController(const uint8_t pins[NUM_USB], const char* names[NUM_USB])
    : mosfetPins(pins), usbNames(names) {}


void USBController::begin() {
    for (int i = 0; i < NUM_USB; i++) {
        pinMode(mosfetPins[i], OUTPUT);
        digitalWrite(mosfetPins[i], LOW);


        usbState[i] = "OFF";
        scheduleEnabled[i] = false;
        overrideActive[i] = false;


        schedStart[i] = schedEnd[i] = schedTarget[i] = 0;
        overrideTarget[i] = overrideUntil[i] = 0;


        defaultState[i] = 0;   
    }


    schedAck = false;
    lastEpochMillis = millis();
}


void USBController::tickEpoch() {
    unsigned long ms = millis();
    unsigned long delta = ms - lastEpochMillis;
    lastEpochMillis = ms;


    if (nowEpoch > 0) {
        nowEpoch += (delta / 1000);
    }


    for (int i = 0; i < NUM_USB; i++) {
        int desired = computeDesired(i);
        apply(i, desired);
    }
}


void USBController::apply(int idx, int state01) {
    if (idx < 0 || idx >= NUM_USB) return;


    usbState[idx] = state01 ? "ON" : "OFF";
    digitalWrite(mosfetPins[idx], state01 ? HIGH : LOW);
}


void USBController::applyManual(int idx, int state01) {
    if (idx < 0 || idx >= NUM_USB) return;
    defaultState[idx] = state01;   
}


int USBController::computeDesired(int idx) {
    if (idx < 0 || idx >= NUM_USB) return 0;


    // Override takes priority
    if (overrideActive[idx]) {
        if (nowEpoch < overrideUntil[idx]) return overrideTarget[idx];
        overrideActive[idx] = false;
    }


    // Schedule applies only during its window
    if (scheduleEnabled[idx] &&
        nowEpoch >= schedStart[idx] &&
        nowEpoch < schedEnd[idx]) {
        return schedTarget[idx];
    }


    // Outside schedule → return manual baseline
    return defaultState[idx];
}


void USBController::setSchedule(int idx, unsigned long start, unsigned long end, int target, bool enabled) {
    schedStart[idx] = start;
    schedEnd[idx] = end;
    schedTarget[idx] = target;
    scheduleEnabled[idx] = enabled;


    schedAck = true;
}


void USBController::setOverride(int idx, int target, unsigned long until) {
    overrideActive[idx] = true;
    overrideTarget[idx] = target;
    overrideUntil[idx] = until;
}


String USBController::statusLine() {
    String s = "STATUS: ";
    for (int i = 0; i < NUM_USB; i++) {
        s += usbNames[i];
        s += "=";
        s += usbState[i];
        if (i < NUM_USB - 1) s += ", ";
    }
    s += " EPOCH=" + String(nowEpoch);
    return s;
}
*/
/* #include <Arduino.h>
#include "USBController.h"

USBController::USBController(const uint8_t pins[NUM_USB], const char* names[NUM_USB])
    : mosfetPins(pins), usbNames(names) {}

void USBController::begin() {
    for (int i = 0; i < NUM_USB; i++) {
        pinMode(mosfetPins[i], OUTPUT);
        digitalWrite(mosfetPins[i], LOW);

        usbState[i] = "OFF";
        scheduleEnabled[i] = false;
        overrideActive[i] = false;

        schedStart[i] = schedEnd[i] = 0;
        schedTarget[i] = 0;

        overrideTarget[i] = 0;
        overrideUntil[i] = 0;

        defaultState[i] = 0;

        // NEW defaults
        priorityHigh[i] = true;    // assume high until Boron sends PRIO
        emergencyOff[i] = false;   // no shedding by default
    }

    schedAck = false;
    lastEpochMillis = millis();
}

void USBController::tickEpoch() {
    unsigned long ms = millis();
    unsigned long delta = ms - lastEpochMillis;
    lastEpochMillis = ms;

    if (nowEpoch > 0) {
        nowEpoch += (delta / 1000);
    }

    for (int i = 0; i < NUM_USB; i++) {
        int desired = computeDesired(i);
        apply(i, desired);
    }
}

void USBController::apply(int idx, int state01) {
    if (idx < 0 || idx >= NUM_USB) return;

    usbState[idx] = state01 ? "ON" : "OFF";
    digitalWrite(mosfetPins[idx], state01 ? HIGH : LOW);
}

void USBController::applyManual(int idx, int state01) {
    if (idx < 0 || idx >= NUM_USB) return;
    defaultState[idx] = state01;
}

int USBController::computeDesired(int idx) {
    if (idx < 0 || idx >= NUM_USB) return 0;

    // NEW: Emergency shedding overrides everything
    if (emergencyOff[idx]) return 0;

    // Override takes priority
    if (overrideActive[idx]) {
        if (nowEpoch < overrideUntil[idx]) return overrideTarget[idx];
        overrideActive[idx] = false;
    }

    // Schedule applies only during its window
    if (scheduleEnabled[idx] &&
        nowEpoch >= schedStart[idx] &&
        nowEpoch < schedEnd[idx]) {
        return schedTarget[idx];
    }

    // Outside schedule → return manual baseline
    return defaultState[idx];
}

void USBController::setSchedule(int idx, unsigned long start, unsigned long end, int target, bool enabled) {
    if (idx < 0 || idx >= NUM_USB) return;

    schedStart[idx] = start;
    schedEnd[idx] = end;
    schedTarget[idx] = target;
    scheduleEnabled[idx] = enabled;

    schedAck = true;
}

void USBController::setOverride(int idx, int target, unsigned long until) {
    if (idx < 0 || idx >= NUM_USB) return;

    overrideActive[idx] = true;
    overrideTarget[idx] = target;
    overrideUntil[idx] = until;
}

// ---------------------------
// NEW: Priority + Emergency API
// ---------------------------
void USBController::setPriority(int idx, bool isHigh) {
    if (idx < 0 || idx >= NUM_USB) return;
    priorityHigh[idx] = isHigh;
}

bool USBController::isHighPriority(int idx) const {
    if (idx < 0 || idx >= NUM_USB) return true;
    return priorityHigh[idx];
}

void USBController::setEmergencyOff(int idx, bool off) {
    if (idx < 0 || idx >= NUM_USB) return;
    emergencyOff[idx] = off;
}

bool USBController::isEmergencyOff(int idx) const {
    if (idx < 0 || idx >= NUM_USB) return false;
    return emergencyOff[idx];
}

void USBController::clearEmergencyAll() {
    for (int i = 0; i < NUM_USB; i++) emergencyOff[i] = false;
}

String USBController::statusLine() {
    // Web UI uses this
    String s = "STATUS: ";
    for (int i = 0; i < NUM_USB; i++) {
        s += usbNames[i];
        s += "=";
        s += usbState[i];
        s += " PRIO=";
        s += (priorityHigh[i] ? "1" : "0");
        s += " EOFF=";
        s += (emergencyOff[i] ? "1" : "0");
        if (i < NUM_USB - 1) s += ", ";
    }
    s += " EPOCH=" + String(nowEpoch);
    return s;
}

*/
#include <Arduino.h>
#include "USBController.h"

USBController::USBController(const uint8_t pins[NUM_USB], const char* names[NUM_USB])
    : mosfetPins(pins), usbNames(names) {}

void USBController::begin() {
    for (int i = 0; i < NUM_USB; i++) {
        pinMode(mosfetPins[i], OUTPUT);
        digitalWrite(mosfetPins[i], LOW);

        usbState[i] = "OFF";
        scheduleEnabled[i] = false;
        overrideActive[i] = false;

        schedStart[i] = 0;
        schedEnd[i] = 0;
        schedTarget[i] = 0;

        overrideTarget[i] = 0;
        overrideUntil[i] = 0;

        defaultState[i] = 0;

        // Default assumptions
        priorityHigh[i] = true;   // high priority until Boron says otherwise
        emergencyOff[i] = false;
    }

    schedAck = false;
    lastEpochMillis = millis();
}

void USBController::tickEpoch() {
    unsigned long ms = millis();
    unsigned long delta = ms - lastEpochMillis;
    lastEpochMillis = ms;

    if (nowEpoch > 0) {
        nowEpoch += (delta / 1000);
    }

    for (int i = 0; i < NUM_USB; i++) {
        int desired = computeDesired(i);
        apply(i, desired);
    }
}

void USBController::apply(int idx, int state01) {
    if (idx < 0 || idx >= NUM_USB) return;

    usbState[idx] = state01 ? "ON" : "OFF";
    digitalWrite(mosfetPins[idx], state01 ? HIGH : LOW);
}

void USBController::applyManual(int idx, int state01) {
    if (idx < 0 || idx >= NUM_USB) return;
    defaultState[idx] = state01;
}

int USBController::computeDesired(int idx) {
    if (idx < 0 || idx >= NUM_USB) return 0;

    // Emergency shedding overrides everything
    if (emergencyOff[idx]) {
        return 0;
    }

    // Override takes priority
    if (overrideActive[idx]) {
        if (nowEpoch < overrideUntil[idx]) return overrideTarget[idx];
        overrideActive[idx] = false;
    }

    // Schedule applies only during its window
    if (scheduleEnabled[idx] &&
        nowEpoch >= schedStart[idx] &&
        nowEpoch < schedEnd[idx]) {
        return schedTarget[idx];
    }

    // Outside schedule -> return manual baseline
    return defaultState[idx];
}

void USBController::setSchedule(int idx, unsigned long start, unsigned long end, int target, bool enabled) {
    if (idx < 0 || idx >= NUM_USB) return;

    schedStart[idx] = start;
    schedEnd[idx] = end;
    schedTarget[idx] = target;
    scheduleEnabled[idx] = enabled;

    schedAck = true;
}

void USBController::setOverride(int idx, int target, unsigned long until) {
    if (idx < 0 || idx >= NUM_USB) return;

    overrideActive[idx] = true;
    overrideTarget[idx] = target;
    overrideUntil[idx] = until;
}

void USBController::setPriority(int idx, bool isHigh) {
    if (idx < 0 || idx >= NUM_USB) return;
    priorityHigh[idx] = isHigh;
}

bool USBController::isHighPriority(int idx) const {
    if (idx < 0 || idx >= NUM_USB) return true;
    return priorityHigh[idx];
}

void USBController::setEmergencyOff(int idx, bool off) {
    if (idx < 0 || idx >= NUM_USB) return;
    emergencyOff[idx] = off;
}

bool USBController::isEmergencyOff(int idx) const {
    if (idx < 0 || idx >= NUM_USB) return false;
    return emergencyOff[idx];
}

void USBController::clearEmergencyAll() {
    for (int i = 0; i < NUM_USB; i++) {
        emergencyOff[i] = false;
    }
}

String USBController::statusLine() {
    String s = "STATUS: ";
    for (int i = 0; i < NUM_USB; i++) {
        s += usbNames[i];
        s += "=";
        s += usbState[i];
        s += " PRIO=";
        s += (priorityHigh[i] ? "1" : "0");
        s += " EOFF=";
        s += (emergencyOff[i] ? "1" : "0");
        if (i < NUM_USB - 1) s += ", ";
    }
    s += " EPOCH=" + String(nowEpoch);
    return s;
}
