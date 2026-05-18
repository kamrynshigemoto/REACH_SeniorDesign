//-----------------------------
// Title: BoronComm.cpp
//-----------------------------
// Purpose: Implements UART communication between the ESP8266 and Particle Boron.
// It parses incoming TIME, BATCFG, PRIORITY, SCHED, and USB ON/OFF messages,
// updates USBController and BatteryEstimator, and sends USB telemetry plus runtime
// estimates back to the Boron.
// Dependencies: BoronComm.h
// Compiler: Platform IO
// Author: Juan Jimenez
// OUTPUTS: USB telemetry serial messages, RUNTIME messages, ACK:BATCFG response
// INPUTS: Serial commands from Boron, USB states, sensor readings, supply voltage,
//         runtime estimate
// Versions:
//      V1.0: 5/18/2026 - Added Boron command parsing, battery config parsing,
//                        telemetry output, priority updates, and runtime reporting
//-----------------------------

/*#include "BoronComm.h"


BoronComm::BoronComm(USBController &usb) : usbRef(usb) {}


void BoronComm::begin() {
    Serial.begin(9600);
}


void BoronComm::send(const String &msg) {
    Serial.println(msg);
}


void BoronComm::process() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        parseLine(line);
    }
}


void BoronComm::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;


    if (line.startsWith("TIME=")) {
        usbRef.nowEpoch = line.substring(5).toInt();
        return;
    }


    if (line.startsWith("SCHED")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);
        int p4 = line.indexOf(',', p3 + 1);
        int p5 = line.indexOf(',', p4 + 1);


        int idx = line.substring(p1 + 1, p2).toInt() - 1;
        unsigned long start = line.substring(p2 + 1, p3).toInt();
        unsigned long end = line.substring(p3 + 1, p4).toInt();
        int target = line.substring(p4 + 1, p5).toInt();
        bool enabled = line.substring(p5 + 1).toInt() == 1;
        usbRef.overrideActive[idx] = false;


        usbRef.setSchedule(idx, start, end, target, enabled);
        return;
    }


    if (line.startsWith("USB")) {
        int idx = line.charAt(3) - '1';
        bool on = line.endsWith("ON");


        usbRef.setOverride(idx, on ? 1 : 0, usbRef.nowEpoch + 86400);
        usbRef.applyManual(idx, on);


        return;
    }
}


void BoronComm::sendAll(float c1, float c2, float c3, float c4,
                        float v1, float v2, float v3, float v4,
                        float vSupply)
{
    float currents[4] = { c1, c2, c3, 0.0f };
    float voltages[4] = { v1, v2, v3, v4 };


    for (int i = 0; i < 4; i++) {
        String msg =
            "USB" + String(i + 1) + "," +
            "STATE=" + usbRef.usbState[i] + "," +
            "SCHED_EN=" + String(usbRef.scheduleEnabled[i]) + "," +
            "SCHED_START=" + String(usbRef.schedStart[i]) + "," +
            "SCHED_END=" + String(usbRef.schedEnd[i]) + "," +
            "SCHED_TARGET=" + String(usbRef.schedTarget[i]) + "," +
            "OVERRIDE=" + String(usbRef.overrideActive[i]) + "," +
            "OVERRIDE_TARGET=" + String(usbRef.overrideTarget[i]) + "," +
            "OVERRIDE_UNTIL=" + String(usbRef.overrideUntil[i]) + "," +
            "CURRENT=" + String(currents[i], 3) + "," +
            "VOLTAGE=" + String(voltages[i], 3) + "," +
            "SUPPLY=" + String(vSupply, 3) + "," +
            "EPOCH=" + String(usbRef.nowEpoch);


        Serial.println(msg);
    }
}
*/
/* #include "BoronComm.h"

BoronComm::BoronComm(USBController &usb) : usbRef(usb) {}

void BoronComm::begin() {
    Serial.begin(9600);
}

void BoronComm::send(const String &msg) {
    Serial.println(msg);
}

void BoronComm::process() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        parseLine(line);
    }
}

void BoronComm::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    if (line.startsWith("TIME=")) {
        usbRef.nowEpoch = line.substring(5).toInt();
        return;
    }

    // NEW: Priority parsing
    // Accept: "PRIO=1010" OR "PRIO,1,0,1,0"
    if (line.startsWith("PRIO")) {
        int eq = line.indexOf('=');
        if (eq > 0) {
            // PRIO=1010
            String bits = line.substring(eq + 1);
            bits.trim();
            for (int i = 0; i < USBController::NUM_USB && i < (int)bits.length(); i++) {
                usbRef.setPriority(i, bits.charAt(i) == '1');
            }
        } else {
            // PRIO,1,0,1,0
            int pos = line.indexOf(',');
            if (pos > 0) {
                pos += 1;
                for (int i = 0; i < USBController::NUM_USB; i++) {
                    int next = line.indexOf(',', pos);
                    String token = (next < 0) ? line.substring(pos) : line.substring(pos, next);
                    token.trim();
                    usbRef.setPriority(i, token.toInt() == 1);
                    if (next < 0) break;
                    pos = next + 1;
                }
            }
        }
        return;
    }

    if (line.startsWith("SCHED")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);
        int p4 = line.indexOf(',', p3 + 1);
        int p5 = line.indexOf(',', p4 + 1);

        int idx = line.substring(p1 + 1, p2).toInt() - 1;
        unsigned long start = line.substring(p2 + 1, p3).toInt();
        unsigned long end = line.substring(p3 + 1, p4).toInt();
        int target = line.substring(p4 + 1, p5).toInt();
        bool enabled = line.substring(p5 + 1).toInt() == 1;

        usbRef.overrideActive[idx] = false;
        usbRef.setSchedule(idx, start, end, target, enabled);
        return;
    }

    if (line.startsWith("USB")) {
        int idx = line.charAt(3) - '1';
        bool on = line.endsWith("ON");

        usbRef.setOverride(idx, on ? 1 : 0, usbRef.nowEpoch + 86400);
        usbRef.applyManual(idx, on ? 1 : 0);
        return;
    }
}

void BoronComm::sendAll(float c1, float c2, float c3, float c4,
                        float v1, float v2, float v3, float v4,
                        float vSupply)
{
    float currents[4] = { c1, c2, c3, c4 };         // FIXED: include c4
    float voltages[4] = { v1, v2, v3, v4 };

    // NEW: compact snapshot line (easy for Boron to parse/store)
    {
        String pr = "PRIO=";
        String eo = "EOFF=";
        for (int i = 0; i < 4; i++) {
            pr += usbRef.isHighPriority(i) ? "1" : "0";
            eo += usbRef.isEmergencyOff(i) ? "1" : "0";
        }
        String snap = pr + "," + eo + ",EPOCH=" + String(usbRef.nowEpoch);
        Serial.println(snap);
    }

    for (int i = 0; i < 4; i++) {
        String msg =
            "USB" + String(i + 1) + "," +
            "STATE=" + usbRef.usbState[i] + "," +
            "PRIO=" + String(usbRef.isHighPriority(i) ? 1 : 0) + "," +
            "EOFF=" + String(usbRef.isEmergencyOff(i) ? 1 : 0) + "," +
            "SCHED_EN=" + String(usbRef.scheduleEnabled[i]) + "," +
            "SCHED_START=" + String(usbRef.schedStart[i]) + "," +
            "SCHED_END=" + String(usbRef.schedEnd[i]) + "," +
            "SCHED_TARGET=" + String(usbRef.schedTarget[i]) + "," +
            "OVERRIDE=" + String(usbRef.overrideActive[i]) + "," +
            "OVERRIDE_TARGET=" + String(usbRef.overrideTarget[i]) + "," +
            "OVERRIDE_UNTIL=" + String(usbRef.overrideUntil[i]) + "," +
            "CURRENT=" + String(currents[i], 3) + "," +
            "VOLTAGE=" + String(voltages[i], 3) + "," +
            "SUPPLY=" + String(vSupply, 3) + "," +
            "EPOCH=" + String(usbRef.nowEpoch);

        Serial.println(msg);
    }
}

*/
/*
#include "BoronComm.h"

BoronComm::BoronComm(USBController &usb) : usbRef(usb) {}

void BoronComm::begin() {
    Serial.begin(9600);
}

void BoronComm::send(const String &msg) {
    Serial.println(msg);
}

void BoronComm::process() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        parseLine(line);
    }
}

void BoronComm::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    if (line.startsWith("TIME=")) {
        usbRef.nowEpoch = line.substring(5).toInt();
        return;
    }

    // Accept:
    // PRIO=1010
    // or
    // PRIO,1,0,1,0
    if (line.startsWith("PRIO")) {
        int eq = line.indexOf('=');

        if (eq > 0) {
            String bits = line.substring(eq + 1);
            bits.trim();

            for (int i = 0; i < USBController::NUM_USB && i < bits.length(); i++) {
                usbRef.setPriority(i, bits.charAt(i) == '1');
            }
        } else {
            int p = line.indexOf(',');
            if (p > 0) {
                p += 1;
                for (int i = 0; i < USBController::NUM_USB; i++) {
                    int next = line.indexOf(',', p);
                    String token = (next < 0) ? line.substring(p) : line.substring(p, next);
                    token.trim();
                    usbRef.setPriority(i, token.toInt() == 1);

                    if (next < 0) break;
                    p = next + 1;
                }
            }
        }
        return;
    }

    if (line.startsWith("SCHED")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);
        int p4 = line.indexOf(',', p3 + 1);
        int p5 = line.indexOf(',', p4 + 1);

        int idx = line.substring(p1 + 1, p2).toInt() - 1;
        unsigned long start = line.substring(p2 + 1, p3).toInt();
        unsigned long end = line.substring(p3 + 1, p4).toInt();
        int target = line.substring(p4 + 1, p5).toInt();
        bool enabled = line.substring(p5 + 1).toInt() == 1;

        usbRef.overrideActive[idx] = false;
        usbRef.setSchedule(idx, start, end, target, enabled);
        return;
    }

    if (line.startsWith("USB")) {
        int idx = line.charAt(3) - '1';
        bool on = line.endsWith("ON");

        usbRef.setOverride(idx, on ? 1 : 0, usbRef.nowEpoch + 86400);
        usbRef.applyManual(idx, on ? 1 : 0);
        return;
    }
}

void BoronComm::sendAll(float c1, float c2, float c3, float c4,
                        float v1, float v2, float v3, float v4,
                        float vSupply)
{
    float currents[4] = { c1, c2, c3, c4 };
    float voltages[4] = { v1, v2, v3, v4 };

    // Compact status for Boron
    String prioLine = "PRIO=";
    String eoffLine = "EOFF=";
    for (int i = 0; i < 4; i++) {
        prioLine += usbRef.isHighPriority(i) ? "1" : "0";
        eoffLine += usbRef.isEmergencyOff(i) ? "1" : "0";
    }
    Serial.println(prioLine + "," + eoffLine + ",EPOCH=" + String(usbRef.nowEpoch));

    for (int i = 0; i < 4; i++) {
        String msg =
            "USB" + String(i + 1) + "," +
            "STATE=" + usbRef.usbState[i] + "," +
            "PRIO=" + String(usbRef.isHighPriority(i) ? 1 : 0) + "," +
            "EOFF=" + String(usbRef.isEmergencyOff(i) ? 1 : 0) + "," +
            "SCHED_EN=" + String(usbRef.scheduleEnabled[i]) + "," +
            "SCHED_START=" + String(usbRef.schedStart[i]) + "," +
            "SCHED_END=" + String(usbRef.schedEnd[i]) + "," +
            "SCHED_TARGET=" + String(usbRef.schedTarget[i]) + "," +
            "OVERRIDE=" + String(usbRef.overrideActive[i]) + "," +
            "OVERRIDE_TARGET=" + String(usbRef.overrideTarget[i]) + "," +
            "OVERRIDE_UNTIL=" + String(usbRef.overrideUntil[i]) + "," +
            "CURRENT=" + String(currents[i], 3) + "," +
            "VOLTAGE=" + String(voltages[i], 3) + "," +
            "SUPPLY=" + String(vSupply, 3) + "," +
            "EPOCH=" + String(usbRef.nowEpoch);

        Serial.println(msg);
    }
}
*/
/*#include "BoronComm.h"

BoronComm::BoronComm(USBController &usb) : usbRef(usb) {}

void BoronComm::begin() {
    Serial.begin(9600);
}

void BoronComm::send(const String &msg) {
    Serial.println(msg);
}

void BoronComm::process() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        parseLine(line);
    }
}

void BoronComm::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    // ----------------------------
    // TIME sync from Boron
    // Format: TIME=1700000000
    // ----------------------------
    if (line.startsWith("TIME=")) {
        usbRef.nowEpoch = line.substring(5).toInt();
        return;
    }

    // ----------------------------
    // PRIORITY from Boron
    // Format: PRIORITY,<outlet>,<priority>
    // Example: PRIORITY,2,1
    // outlet: 1..4
    // priority: 1 = high, 0 = low
    // ----------------------------
    if (line.startsWith("PRIORITY,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);

        if (p1 < 0 || p2 < 0) return;

        int outlet = line.substring(p1 + 1, p2).toInt();
        int priority = line.substring(p2 + 1).toInt();

        int idx = outlet - 1;
        if (idx < 0 || idx >= USBController::NUM_USB) return;

        usbRef.setPriority(idx, priority == 1);
        return;
    }

    // ----------------------------
    // Compact priority bitmap
    // Format: PRIO=1010
    // ----------------------------
    if (line.startsWith("PRIO=")) {
        String bits = line.substring(5);
        bits.trim();

        for (int i = 0; i < USBController::NUM_USB && i < bits.length(); i++) {
            usbRef.setPriority(i, bits.charAt(i) == '1');
        }
        return;
    }

    // ----------------------------
    // CSV priority list
    // Format: PRIO,1,0,1,0
    // ----------------------------
    if (line.startsWith("PRIO,")) {
        int pos = line.indexOf(',') + 1;

        for (int i = 0; i < USBController::NUM_USB; i++) {
            int next = line.indexOf(',', pos);
            String token = (next < 0) ? line.substring(pos) : line.substring(pos, next);
            token.trim();

            usbRef.setPriority(i, token.toInt() == 1);

            if (next < 0) break;
            pos = next + 1;
        }
        return;
    }

    // ----------------------------
    // Schedule from Boron
    // Format: SCHED,outlet,start,end,target,enabled
    // ----------------------------
    if (line.startsWith("SCHED,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);
        int p4 = line.indexOf(',', p3 + 1);
        int p5 = line.indexOf(',', p4 + 1);

        if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) return;

        int idx = line.substring(p1 + 1, p2).toInt() - 1;
        if (idx < 0 || idx >= USBController::NUM_USB) return;

        unsigned long start = line.substring(p2 + 1, p3).toInt();
        unsigned long end = line.substring(p3 + 1, p4).toInt();
        int target = line.substring(p4 + 1, p5).toInt();
        bool enabled = line.substring(p5 + 1).toInt() == 1;

        usbRef.overrideActive[idx] = false;
        usbRef.setSchedule(idx, start, end, target, enabled);
        return;
    }

    // ----------------------------
    // Manual command from Boron
    // Format: USB1=ON or USB2=OFF
    // ----------------------------
    if (line.startsWith("USB")) {
        if (line.length() < 6) return;

        int idx = line.charAt(3) - '1';
        if (idx < 0 || idx >= USBController::NUM_USB) return;

        bool on = line.endsWith("ON");

        usbRef.setOverride(idx, on ? 1 : 0, usbRef.nowEpoch + 86400);
        usbRef.applyManual(idx, on ? 1 : 0);
        return;
    }
}

void BoronComm::sendAll(float c1, float c2, float c3, float c4,
                        float v1, float v2, float v3, float v4,
                        float vSupply)
{
    float currents[4] = { c1, c2, c3, c4 };
    float voltages[4] = { v1, v2, v3, v4 };

    // Optional compact snapshot
    String prioLine = "PRIO=";
    String eoffLine = "EOFF=";
    for (int i = 0; i < 4; i++) {
        prioLine += usbRef.isHighPriority(i) ? "1" : "0";
        eoffLine += usbRef.isEmergencyOff(i) ? "1" : "0";
    }
    Serial.println(prioLine + "," + eoffLine + ",EPOCH=" + String(usbRef.nowEpoch));

    for (int i = 0; i < 4; i++) {
        String msg =
            "USB" + String(i + 1) + "," +
            "STATE=" + usbRef.usbState[i] + "," +
            "PRIO=" + String(usbRef.isHighPriority(i) ? 1 : 0) + "," +
            "EOFF=" + String(usbRef.isEmergencyOff(i) ? 1 : 0) + "," +
            "SCHED_EN=" + String(usbRef.scheduleEnabled[i]) + "," +
            "SCHED_START=" + String(usbRef.schedStart[i]) + "," +
            "SCHED_END=" + String(usbRef.schedEnd[i]) + "," +
            "SCHED_TARGET=" + String(usbRef.schedTarget[i]) + "," +
            "OVERRIDE=" + String(usbRef.overrideActive[i]) + "," +
            "OVERRIDE_TARGET=" + String(usbRef.overrideTarget[i]) + "," +
            "OVERRIDE_UNTIL=" + String(usbRef.overrideUntil[i]) + "," +
            "CURRENT=" + String(currents[i], 3) + "," +
            "VOLTAGE=" + String(voltages[i], 3) + "," +
            "SUPPLY=" + String(vSupply, 3) + "," +
            "EPOCH=" + String(usbRef.nowEpoch);

        Serial.println(msg);
    }
}
    */
/////////////////////////////////////////////////////////
// LAST WORKING GOOD CODE ///////////////////////////////
/////////////////////////////////////////////////////////
/*

#include "BoronComm.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
BoronComm::BoronComm(USBController &usb, BatteryEstimator &batt)
    : usbRef(usb), battRef(batt), batteryInfoReceived(false),
      _lastBattReqMs(0) {}

// ---------------------------------------------------------------------------
// requestBattInfo — sends REQ:BATT and records the timestamp
// ---------------------------------------------------------------------------
void BoronComm::requestBattInfo() {
    Serial.println("REQ:BATT");
    _lastBattReqMs = millis();
}

// ---------------------------------------------------------------------------
// begin — opens serial and fires the first REQ:BATT
// ---------------------------------------------------------------------------
void BoronComm::begin() {
    Serial.begin(9600);
    requestBattInfo();   // first request; process() will retry every 5 s
}

// ---------------------------------------------------------------------------
// send  (convenience wrapper)
// ---------------------------------------------------------------------------
void BoronComm::send(const String &msg) {
    Serial.println(msg);
}

// ---------------------------------------------------------------------------
// process  —  drain the incoming serial buffer + retry REQ:BATT if needed
// ---------------------------------------------------------------------------
void BoronComm::process() {
    // Retry REQ:BATT every BATT_REQ_INTERVAL_MS until the Boron responds.
    if (!batteryInfoReceived &&
        (millis() - _lastBattReqMs >= BATT_REQ_INTERVAL_MS)) {
        requestBattInfo();
    }

    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        parseLine(line);
    }
}

// ---------------------------------------------------------------------------
// parseLine
// Handles every message the Boron can send down to the ESP.
// ---------------------------------------------------------------------------
void BoronComm::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    // ------------------------------------------------------------------
    // TIME sync
    // Format: TIME=<epoch>
    // ------------------------------------------------------------------
    if (line.startsWith("TIME=")) {
        usbRef.nowEpoch = line.substring(5).toInt();
        return;
    }

    // ------------------------------------------------------------------
    // Battery configuration reply  (sent by Boron at startup in response
    // to our "REQ:BATT" request, or any time the config changes)
    //
    // Format: BATT,AH=<float>,TYPE=<LFP|LEAD>
    // Example: BATT,AH=15.0,TYPE=LFP
    // ------------------------------------------------------------------
    if (line.startsWith("BATT,")) {
        // Find AH= value
        int ahIdx = line.indexOf("AH=");
        int typeIdx = line.indexOf("TYPE=");

        if (ahIdx < 0 || typeIdx < 0) return;   // malformed

        // Parse Ah — value runs from after "AH=" to the next comma
        int ahEnd = line.indexOf(',', ahIdx);
        float ah = (ahEnd < 0)
            ? line.substring(ahIdx + 3).toFloat()
            : line.substring(ahIdx + 3, ahEnd).toFloat();

        // Parse chemistry string
        String typeStr = line.substring(typeIdx + 5);
        typeStr.trim();
        BattChemistry chem = (typeStr == "LEAD") ? BATT_LEAD : BATT_LFP;

        battRef.setConfig(ah, chem);
        batteryInfoReceived = true;

        // Acknowledge so the Boron knows we received it
        Serial.println("BATT:ACK");
        return;
    }

    // ------------------------------------------------------------------
    // Priority — single outlet
    // Format: PRIORITY,<outlet 1..4>,<0|1>
    // ------------------------------------------------------------------
    if (line.startsWith("PRIORITY,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        if (p1 < 0 || p2 < 0) return;

        int outlet   = line.substring(p1 + 1, p2).toInt();
        int priority = line.substring(p2 + 1).toInt();
        int idx      = outlet - 1;

        if (idx >= 0 && idx < USBController::NUM_USB) {
            usbRef.setPriority(idx, priority == 1);
        }
        return;
    }

    // ------------------------------------------------------------------
    // Priority bitmap
    // Format: PRIO=1010
    // ------------------------------------------------------------------
    if (line.startsWith("PRIO=")) {
        String bits = line.substring(5);
        bits.trim();
        for (int i = 0; i < USBController::NUM_USB && i < (int)bits.length(); i++) {
            usbRef.setPriority(i, bits.charAt(i) == '1');
        }
        return;
    }

    // ------------------------------------------------------------------
    // Priority CSV list
    // Format: PRIO,1,0,1,0
    // ------------------------------------------------------------------
    if (line.startsWith("PRIO,")) {
        int pos = line.indexOf(',') + 1;
        for (int i = 0; i < USBController::NUM_USB; i++) {
            int next  = line.indexOf(',', pos);
            String tok = (next < 0) ? line.substring(pos) : line.substring(pos, next);
            tok.trim();
            usbRef.setPriority(i, tok.toInt() == 1);
            if (next < 0) break;
            pos = next + 1;
        }
        return;
    }

    // ------------------------------------------------------------------
    // Schedule
    // Format: SCHED,<outlet 1..4>,<start>,<end>,<target>,<enabled>
    // ------------------------------------------------------------------
    if (line.startsWith("SCHED,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);
        int p4 = line.indexOf(',', p3 + 1);
        int p5 = line.indexOf(',', p4 + 1);
        if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) return;

        int           idx     = line.substring(p1 + 1, p2).toInt() - 1;
        unsigned long start   = line.substring(p2 + 1, p3).toInt();
        unsigned long end     = line.substring(p3 + 1, p4).toInt();
        int           target  = line.substring(p4 + 1, p5).toInt();
        bool          enabled = line.substring(p5 + 1).toInt() == 1;

        if (idx >= 0 && idx < USBController::NUM_USB) {
            usbRef.overrideActive[idx] = false;
            usbRef.setSchedule(idx, start, end, target, enabled);
        }
        return;
    }

    // ------------------------------------------------------------------
    // Manual outlet command
    // Format: USB1=ON  /  USB2=OFF
    // ------------------------------------------------------------------
    if (line.startsWith("USB")) {
        if (line.length() < 6) return;
        int idx = line.charAt(3) - '1';
        if (idx < 0 || idx >= USBController::NUM_USB) return;

        bool on = line.endsWith("ON");
        usbRef.setOverride(idx, on ? 1 : 0, usbRef.nowEpoch + 86400);
        usbRef.applyManual(idx, on ? 1 : 0);
        return;
    }
}

// ---------------------------------------------------------------------------
// sendAll
// Sends per-outlet telemetry plus a dedicated RUNTIME line.
//
// runtimeHours: pass the result of BatteryEstimator::estimateRuntime().
//               -1 means "not yet available / battery not configured".
// ---------------------------------------------------------------------------
void BoronComm::sendAll(float c1, float c2, float c3, float c4,
                        float v1, float v2, float v3, float v4,
                        float vSupply,
                        float runtimeHours)
{
    float currents[4] = { c1, c2, c3, c4 };
    float voltages[4] = { v1, v2, v3, v4 };

    // ------------------------------------------------------------------
    // Compact snapshot line.
    //
    // SoC is derived purely from vSupply using the voltage→SoC table.
    // This works as soon as the first sensor read happens — it does NOT
    // require the Boron to have replied with battery Ah info yet.
    //
    // RUNTIME requires both voltage AND Ah capacity, so it stays NA
    // until the Boron sends BATT,AH=...,TYPE=...
    //
    // Format:
    //   SNAP,PRIO=<xxxx>,EOFF=<xxxx>,SOC=<0-100>,RUNTIME=<h|NA>,EPOCH=<n>
    // ------------------------------------------------------------------
    String prioStr = "";
    String eoffStr = "";
    for (int i = 0; i < 4; i++) {
        prioStr += usbRef.isHighPriority(i) ? "1" : "0";
        eoffStr += usbRef.isEmergencyOff(i) ? "1" : "0";
    }

    // SoC from voltage only — always available once sensors are running.
    // voltageToSoC() uses the stored chemistry; default is LFP which is
    // correct until the Boron overrides it via BATT,TYPE=LEAD.
    int socPct = (int)(battRef.voltageToSoC(vSupply) * 100.0f);

    String snapLine = "SNAP,"
        "PRIO=" + prioStr + ","
        "EOFF=" + eoffStr + ","
        "SOC="  + String(socPct) + ",";

    if (runtimeHours >= 0.0f) {
        snapLine += "RUNTIME=" + String(runtimeHours, 2) + ",";
    } else {
        snapLine += "RUNTIME=NA,";
    }

    snapLine += "EPOCH=" + String(usbRef.nowEpoch);
    Serial.println(snapLine);

    // ------------------------------------------------------------------
    // Per-outlet detail lines (unchanged format, no breaking change)
    // ------------------------------------------------------------------
    for (int i = 0; i < 4; i++) {
        String msg =
            "USB" + String(i + 1) + ","
            "STATE="           + usbRef.usbState[i]                        + ","
            "PRIO="            + String(usbRef.isHighPriority(i) ? 1 : 0)  + ","
            "EOFF="            + String(usbRef.isEmergencyOff(i) ? 1 : 0)  + ","
            "SCHED_EN="        + String(usbRef.scheduleEnabled[i])          + ","
            "SCHED_START="     + String(usbRef.schedStart[i])               + ","
            "SCHED_END="       + String(usbRef.schedEnd[i])                 + ","
            "SCHED_TARGET="    + String(usbRef.schedTarget[i])              + ","
            "OVERRIDE="        + String(usbRef.overrideActive[i])           + ","
            "OVERRIDE_TARGET=" + String(usbRef.overrideTarget[i])           + ","
            "OVERRIDE_UNTIL="  + String(usbRef.overrideUntil[i])            + ","
            "CURRENT="         + String(currents[i], 3)                     + ","
            "VOLTAGE="         + String(voltages[i], 3)                     + ","
            "SUPPLY="          + String(vSupply, 3)                         + ","
            "EPOCH="           + String(usbRef.nowEpoch);

        Serial.println(msg);
    }
}
*/ 
//////////////////////////////////////
// SYSTEM RUNTIME ATTEMPT/////////////
/////////////////////////////////////

#include "BoronComm.h"

BoronComm::BoronComm(USBController &usb, BatteryEstimator &batt)
    : usbRef(usb), battRef(batt), batteryInfoReceived(false) {}

void BoronComm::begin() {
    Serial.begin(9600);
}

void BoronComm::send(const String &msg) {
    Serial.println(msg);
}

void BoronComm::process() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        parseLine(line);
    }
}

void BoronComm::parseLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    // TIME=<epoch>
    if (line.startsWith("TIME=")) {
        usbRef.nowEpoch = line.substring(5).toInt();
        return;
    }

    // BATCFG,15.0,LFP,12.80
    if (line.startsWith("BATCFG,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);

        if (p1 < 0 || p2 < 0 || p3 < 0) return;

        float ah = line.substring(p1 + 1, p2).toFloat();

        String chemStr = line.substring(p2 + 1, p3);
        chemStr.trim();

        float nominalVoltage = line.substring(p3 + 1).toFloat();
        (void)nominalVoltage; // parsed but not used yet

        BattChemistry chem = (chemStr == "LEAD") ? BATT_LEAD : BATT_LFP;

        battRef.setConfig(ah, chem);
        batteryInfoReceived = true;

        Serial.println("ACK:BATCFG");
        return;
    }

    // PRIORITY,2,1
    if (line.startsWith("PRIORITY,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);

        if (p1 < 0 || p2 < 0) return;

        int outlet = line.substring(p1 + 1, p2).toInt();
        int priority = line.substring(p2 + 1).toInt();
        int idx = outlet - 1;

        if (idx >= 0 && idx < USBController::NUM_USB) {
            usbRef.setPriority(idx, priority == 1);
        }

        return;
    }

    // SCHED,outlet,start,end,target,enabled
    if (line.startsWith("SCHED,")) {
        int p1 = line.indexOf(',');
        int p2 = line.indexOf(',', p1 + 1);
        int p3 = line.indexOf(',', p2 + 1);
        int p4 = line.indexOf(',', p3 + 1);
        int p5 = line.indexOf(',', p4 + 1);

        if (p1 < 0 || p2 < 0 || p3 < 0 || p4 < 0 || p5 < 0) return;

        int idx = line.substring(p1 + 1, p2).toInt() - 1;
        unsigned long start = line.substring(p2 + 1, p3).toInt();
        unsigned long end = line.substring(p3 + 1, p4).toInt();
        int target = line.substring(p4 + 1, p5).toInt();
        bool enabled = line.substring(p5 + 1).toInt() == 1;

        if (idx >= 0 && idx < USBController::NUM_USB) {
            usbRef.overrideActive[idx] = false;
            usbRef.setSchedule(idx, start, end, target, enabled);
        }

        return;
    }

    // USB1=ON or USB1=OFF
    if (line.startsWith("USB")) {
        if (line.length() < 6) return;

        int idx = line.charAt(3) - '1';
        if (idx < 0 || idx >= USBController::NUM_USB) return;

        bool on = line.endsWith("ON");

        usbRef.setOverride(idx, on ? 1 : 0, usbRef.nowEpoch + 86400);
        usbRef.applyManual(idx, on ? 1 : 0);

        return;
    }
}

void BoronComm::sendAll(float c1, float c2, float c3, float c4,
                        float v1, float v2, float v3, float v4,
                        float vSupply,
                        float runtimeHours)
{
    float currents[4] = { c1, c2, c3, c4 };
    float voltages[4] = { v1, v2, v3, v4 };

    for (int i = 0; i < 4; i++) {
        String msg =
            "USB" + String(i + 1) + "," +
            "STATE=" + usbRef.usbState[i] + "," +
            "PRIO=" + String(usbRef.isHighPriority(i) ? 1 : 0) + "," +
            "EOFF=" + String(usbRef.isEmergencyOff(i) ? 1 : 0) + "," +
            "SCHED_EN=" + String(usbRef.scheduleEnabled[i] ? 1 : 0) + "," +
            "SCHED_START=" + String(usbRef.schedStart[i]) + "," +
            "SCHED_END=" + String(usbRef.schedEnd[i]) + "," +
            "SCHED_TARGET=" + String(usbRef.schedTarget[i]) + "," +
            "OVERRIDE=" + String(usbRef.overrideActive[i] ? 1 : 0) + "," +
            "OVERRIDE_TARGET=" + String(usbRef.overrideTarget[i]) + "," +
            "OVERRIDE_UNTIL=" + String(usbRef.overrideUntil[i]) + "," +
            "CURRENT=" + String(currents[i], 3) + "," +
            "VOLTAGE=" + String(voltages[i], 3) + "," +
            "SUPPLY=" + String(vSupply, 3) + "," +
            "EPOCH=" + String(usbRef.nowEpoch);

        Serial.println(msg);
    }

    if (runtimeHours >= 0.0f) {
        Serial.println("RUNTIME," + String(runtimeHours, 1));
    } else {
        Serial.println("RUNTIME,NA");
    }
}
