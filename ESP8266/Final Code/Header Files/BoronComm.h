/* #pragma once
#include <Arduino.h>
#include "USBController.h"

class BoronComm {
public:
    BoronComm(USBController &usb);

    void begin();
    void process();
    void send(const String &msg);

    void sendAll(float c1, float c2, float c3, float c4,
                 float v1, float v2, float v3, float v4,
                 float vSupply);

private:
    USBController &usbRef;
    void parseLine(String line);
};
*/
/*
#pragma once
#include <Arduino.h>
#include "USBController.h"

class BoronComm {
public:
    BoronComm(USBController &usb);

    void begin();
    void process();
    void send(const String &msg);

    void sendAll(float c1, float c2, float c3, float c4,
                 float v1, float v2, float v3, float v4,
                 float vSupply);

private:
    USBController &usbRef;
    void parseLine(String line);
};
*/


////////////////////////////
// LAST WORKING GOOD CODE//
//////////////////////////
/*
#pragma once
#include <Arduino.h>
#include "USBController.h"
#include "BatteryEstimator.h"

// How often to re-send REQ:BATT until the Boron responds (ms)
static const unsigned long BATT_REQ_INTERVAL_MS = 5000UL;

class BoronComm {
public:
    BoronComm(USBController &usb, BatteryEstimator &batt);

    void begin();

    // Call every loop() tick.  Drains serial AND handles REQ:BATT retries.
    void process();

    void send(const String &msg);

    // runtimeHours: result of BatteryEstimator::estimateRuntime().
    //               Pass -1 when not yet available.
    void sendAll(float c1, float c2, float c3, float c4,
                 float v1, float v2, float v3, float v4,
                 float vSupply,
                 float runtimeHours);

    // True once the Boron has replied with BATT,AH=...,TYPE=...
    bool batteryInfoReceived;

private:
    USBController    &usbRef;
    BatteryEstimator &battRef;

    unsigned long _lastBattReqMs;   // timestamp of last REQ:BATT send

    void parseLine(String line);
    void requestBattInfo();         // sends REQ:BATT and stamps the timer
};
*/ 

// SYSTEM RUNTIME UPDATE

#pragma once
#include <Arduino.h>
#include "USBController.h"
#include "BatteryEstimator.h"

class BoronComm {
public:
    BoronComm(USBController &usb, BatteryEstimator &batt);

    void begin();
    void process();
    void send(const String &msg);

    void sendAll(float c1, float c2, float c3, float c4,
                 float v1, float v2, float v3, float v4,
                 float vSupply,
                 float runtimeHours);

    bool batteryInfoReceived;

private:
    USBController    &usbRef;
    BatteryEstimator &battRef;

    void parseLine(String line);
};
