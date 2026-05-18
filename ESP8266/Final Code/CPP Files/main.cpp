/*#include <Arduino.h>
#include "SensorManager.h"
#include "USBController.h"
#include "LCDManager.h"
#include "BoronComm.h"
#include "WebServerManager.h"

// ------------------------------------------------------
// Define USB pins + names
// ------------------------------------------------------
const uint8_t USB_PINS[4] = { D0, D6, D7, D8 };
const char* USB_NAMES[4]  = { "USB1", "USB2", "USB3", "USB4" };

// ------------------------------------------------------
// Global objects
// ------------------------------------------------------
SensorManager sensor;
USBController usbController(USB_PINS, USB_NAMES);
LCDManager lcd;
BoronComm boron(usbController);
WebServerManager web(usbController);

unsigned long lastLCD = 0;
unsigned long lastSend = 0;

// ------------------------------------------------------
// Setup
// ------------------------------------------------------
void setup() {
    Serial.begin(9600);

    sensor.begin();
    usbController.begin();
    lcd.begin();
    boron.begin();
    web.begin();
}

// ------------------------------------------------------
// Loop
// ------------------------------------------------------
void loop() {
    usbController.tickEpoch();
    web.handle();
    boron.process();

    float c1, c2, c3, c4;
    float v1, v2, v3, v4;
    float vSupply;

    //  real SensorManager signature:
    // readAll(c1,c2,c3, v1,v2,v3,v4, vSupply)
    sensor.readAll(
        c1, c2, c3, c4,
        v1, v2, v3, v4,
        vSupply
    );

    // --------------------------------------------------
    // LCD update every 500ms
    // --------------------------------------------------
    if (millis() - lastLCD > 500) {
        lastLCD = millis();

        lcd.update(
            v1, v2, v3, v4,
            c1, c2, c3, c4,      
            vSupply,
            usbController.usbState,
            usbController.scheduleEnabled,
            usbController.schedStart,
            usbController.schedEnd,
            usbController.schedTarget,
            usbController.nowEpoch,
            usbController.schedAck   // <-- ACK flag
        );
    }

    // --------------------------------------------------
    // Telemetry to Boron every 2 seconds
    // --------------------------------------------------
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        boron.sendAll(
            c1, c2, c3, c4,
            v1, v2, v3, v4,
            vSupply
        );
    }
}
*/
/* #include <Arduino.h>
#include "SensorManager.h"
#include "USBController.h"
#include "LCDManager.h"
#include "BoronComm.h"
#include "WebServerManager.h"

// ------------------------------------------------------
// Define USB pins + names
// ------------------------------------------------------
const uint8_t USB_PINS[4] = { D0, D6, D7, D8 };
const char* USB_NAMES[4]  = { "USB1", "USB2", "USB3", "USB4" };

// ------------------------------------------------------
// Global objects
// ------------------------------------------------------
SensorManager sensor;
USBController usbController(USB_PINS, USB_NAMES);
LCDManager lcd;
BoronComm boron(usbController);
WebServerManager web(usbController);

unsigned long lastLCD = 0;
unsigned long lastSend = 0;

// ------------------------------------------------------
// Low-voltage shedding config
// ------------------------------------------------------
static const float VS_CUTOFF  = 11.8f;
static const float VS_RECOVER = 12.0f;   // hysteresis to prevent thrash
static const int   AVG_SAMPLES = 200;
static const float EQUAL_A = 0.005f;     // 5 mA = 0.005 A

static bool lowVActive = false;
static unsigned long lastSheddingMs = 0;

static float sampleAvgCurrentA(SensorManager &s, int outletIdx) {
    double sum = 0.0;
    for (int k = 0; k < AVG_SAMPLES; k++) {
        float a = 0.0f;
        switch (outletIdx) {
            case 0: a = s.readUsb1Current(); break;
            case 1: a = s.readUsb2Current(); break;
            case 2: a = s.readUsb3Current(); break;
            case 3: a = s.readUsb4Current(); break;
            default: a = 0.0f; break;
        }
        sum += a;
        yield(); // keep ESP responsive
    }
    return (float)(sum / (double)AVG_SAMPLES);
}

// ------------------------------------------------------
// Setup
// ------------------------------------------------------
void setup() {
    Serial.begin(9600);

    sensor.begin();
    usbController.begin();
    lcd.begin();
    boron.begin();
    web.begin();
    
    // ---------- TEST PRIORITY CONFIG ----------
    usbController.setPriority(0, true);   // USB1 priority
    usbController.setPriority(1, false);  // USB2 priority
    usbController.setPriority(2, false);  // USB3 priority
    usbController.setPriority(3, true);   // USB4 priority
}


// ------------------------------------------------------
// Loop
// ------------------------------------------------------
void loop() {
    usbController.tickEpoch();
    web.handle();
    boron.process();

    float c1, c2, c3, c4;
    float v1, v2, v3, v4;
    float vSupply;

    sensor.readAll(
        c1, c2, c3, c4,
        v1, v2, v3, v4,
        vSupply
    );

    // --------------------------------------------------
    // Low-voltage priority shedding (Emergency layer)
    // --------------------------------------------------
    if (!lowVActive) {
        if (vSupply <= VS_CUTOFF) {
            lowVActive = true;
            lastSheddingMs = 0; // force immediate action
        }
    } else {
        // recover only when above hysteresis
        if (vSupply >= VS_RECOVER) {
            lowVActive = false;
            usbController.clearEmergencyAll();
        }
    }

    if (lowVActive) {
        const unsigned long SHED_PERIOD_MS = 2000;
        if (millis() - lastSheddingMs >= SHED_PERIOD_MS) {
            lastSheddingMs = millis();

            // low-priority outlets that are currently ON
            int candidates[USBController::NUM_USB];
            int n = 0;

            for (int i = 0; i < USBController::NUM_USB; i++) {
                bool isLowPriority = !usbController.isHighPriority(i);
                bool isOn = (usbController.usbState[i] == "ON");
                if (isLowPriority && isOn) candidates[n++] = i;
            }

            if (n > 0) {
                float avgA[USBController::NUM_USB] = {0,0,0,0};

                for (int j = 0; j < n; j++) {
                    int idx = candidates[j];
                    avgA[idx] = sampleAvgCurrentA(sensor, idx);
                }

                float maxA = -1.0f;
                float minA = 1e9f;
                int maxIdx = candidates[0];

                for (int j = 0; j < n; j++) {
                    int idx = candidates[j];
                    float a = avgA[idx];
                    if (a > maxA) { maxA = a; maxIdx = idx; }
                    if (a < minA) { minA = a; }
                }

                if ((maxA - minA) <= EQUAL_A) {
                    // equal within 5mA -> turn off ALL low priority outlets
                    for (int j = 0; j < n; j++) {
                        usbController.setEmergencyOff(candidates[j], true);
                    }
                } else {
                    // otherwise turn off the highest-current low priority outlet
                    usbController.setEmergencyOff(maxIdx, true);
                }
            }
        }
    }

    // --------------------------------------------------
    // LCD update every 500ms
    // --------------------------------------------------
    if (millis() - lastLCD > 500) {
        lastLCD = millis();

        lcd.update(
            v1, v2, v3, v4,
            c1, c2, c3, c4,
            vSupply,
            usbController.usbState,
            usbController.scheduleEnabled,
            usbController.schedStart,
            usbController.schedEnd,
            usbController.schedTarget,
            usbController.nowEpoch,
            usbController.schedAck
        );
    }

    // --------------------------------------------------
    // Telemetry to Boron every 2 seconds 
    // --------------------------------------------------
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        boron.sendAll(
            c1, c2, c3, c4,
            v1, v2, v3, v4,
            vSupply
        );
    }
}
*/
/*
#include <Arduino.h>
#include "SensorManager.h"
#include "USBController.h"
#include "LCDManager.h"
#include "BoronComm.h"
#include "WebServerManager.h"

// ------------------------------------------------------
// Define USB pins + names
// ------------------------------------------------------
const uint8_t USB_PINS[4] = { D0, D6, D7, D8 };
const char* USB_NAMES[4]  = { "USB1", "USB2", "USB3", "USB4" };

// ------------------------------------------------------
// Global objects
// ------------------------------------------------------
SensorManager sensor;
USBController usbController(USB_PINS, USB_NAMES);
// LCDManager lcd;
BoronComm boron(usbController);
WebServerManager web(usbController);

//unsigned long lastLCD = 0;
unsigned long lastSend = 0;

// ------------------------------------------------------
// One-shot threshold configuration
// ------------------------------------------------------
const float THRESHOLD1 = 11.8f;
const float THRESHOLD2 = 11.5f;
const float RECOVER    = 12.0f;

const int   AVG_SAMPLES = 200;
const float EQUAL_A     = 0.005f;   // 5 mA

bool shed118Done = false;
bool shed115Done = false;

// ------------------------------------------------------
// Helpers
// ------------------------------------------------------
static float sampleAvgCurrentA(SensorManager &s, int outletIdx) {
    double sum = 0.0;

    for (int k = 0; k < AVG_SAMPLES; k++) {
        float a = 0.0f;

        switch (outletIdx) {
            case 0: a = s.readUsb1Current(); break;
            case 1: a = s.readUsb2Current(); break;
            case 2: a = s.readUsb3Current(); break;
            case 3: a = s.readUsb4Current(); break;
            default: a = 0.0f; break;
        }

        sum += a;
        yield();
    }

    return (float)(sum / (double)AVG_SAMPLES);
}

static void shedOneLevel(SensorManager &sensor, USBController &usbController) {
    int candidates[USBController::NUM_USB];
    int n = 0;

    // Only consider low-priority outlets that are currently ON
    for (int i = 0; i < USBController::NUM_USB; i++) {
        bool isLowPriority = !usbController.isHighPriority(i);
        bool isOn = (usbController.usbState[i] == "ON");
        bool alreadyEmergencyOff = usbController.isEmergencyOff(i);

        if (isLowPriority && isOn && !alreadyEmergencyOff) {
            candidates[n++] = i;
        }
    }

    if (n == 0) return;

    float avgA[USBController::NUM_USB] = {0, 0, 0, 0};

    for (int j = 0; j < n; j++) {
        int idx = candidates[j];
        avgA[idx] = sampleAvgCurrentA(sensor, idx);
    }

    float maxA = -1.0f;
    float minA = 999999.0f;
    int maxIdx = candidates[0];

    for (int j = 0; j < n; j++) {
        int idx = candidates[j];
        float a = avgA[idx];

        if (a > maxA) {
            maxA = a;
            maxIdx = idx;
        }

        if (a < minA) {
            minA = a;
        }
    }

    // If all low-priority candidates are within 5 mA, turn all of them off
    if ((maxA - minA) <= EQUAL_A) {
        for (int j = 0; j < n; j++) {
            usbController.setEmergencyOff(candidates[j], true);
        }
    } else {
        // Otherwise only turn off the one drawing the most current
        usbController.setEmergencyOff(maxIdx, true);
    }
}

// ------------------------------------------------------
// Setup
// ------------------------------------------------------
void setup() {
    Serial.begin(9600);

    sensor.begin();
    usbController.begin();
    //lcd.begin();
    boron.begin();
    web.begin();

    // Optional startup test priorities:
    //usbController.setPriority(0, true);   // USB1 high
    //usbController.setPriority(1, false);  // USB2 low
    //usbController.setPriority(2, false);  // USB3 low
    //usbController.setPriority(3, true);   // USB4 high
}

// ------------------------------------------------------
// Loop
// ------------------------------------------------------
void loop() {
    usbController.tickEpoch();
    web.handle();
    boron.process();

    float c1, c2, c3, c4;
    float v1, v2, v3, v4;
    float vSupply;

    sensor.readAll(
        c1, c2, c3, c4,
        v1, v2, v3, v4,
        vSupply
    );

    // --------------------------------------------------
    // One-shot threshold shedding
    // --------------------------------------------------

    // Reset latches after voltage recovers
    if (vSupply >= RECOVER) {
        shed118Done = false;
        shed115Done = false;
        usbController.clearEmergencyAll();
    }

    // First threshold: shed once
    if (!shed118Done && vSupply <= THRESHOLD1) {
        shed118Done = true;
        shedOneLevel(sensor, usbController);
        Serial.println("11.8V threshold reached -> shed one level");
    }

    // Second threshold: shed once more
    if (!shed115Done && vSupply <= THRESHOLD2) {
        shed115Done = true;
        shedOneLevel(sensor, usbController);
        Serial.println("11.5V threshold reached -> shed one more level");
    }

    // --------------------------------------------------
    // LCD update every 500ms
    // --------------------------------------------------
    /*if (millis() - lastLCD > 500) {
        lastLCD = millis();

        lcd.update(
            v1, v2, v3, v4,
            c1, c2, c3, c4,
            vSupply,
            usbController.usbState,
            usbController.scheduleEnabled,
            usbController.schedStart,
            usbController.schedEnd,
            usbController.schedTarget,
            usbController.nowEpoch,
            usbController.schedAck
        );
    }

    // --------------------------------------------------
    // Telemetry to Boron every 2 seconds
    // --------------------------------------------------
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        boron.sendAll(
            c1, c2, c3, c4,
            v1, v2, v3, v4,
            vSupply
        );
    }
} */

////////////////////////////////
//LAST GOOD CODE////////////////
////////////////////////////////
/*
#include <Arduino.h>
#include "SensorManager.h"
#include "USBController.h"
#include "LCDManager.h"
#include "BoronComm.h"
#include "WebServerManager.h"
#include "BatteryEstimator.h"

// ------------------------------------------------------
// Define USB pins + names
// ------------------------------------------------------
const uint8_t USB_PINS[4] = { D0, D6, D7, D8 };
const char* USB_NAMES[4]  = { "USB1", "USB2", "USB3", "USB4" };

// ------------------------------------------------------
// Base current draw of the ESP8266 + supporting circuitry
// (measured / estimated: 80–100 mA).  Used in runtime calc.
// ------------------------------------------------------
static const float BASE_CURRENT_A = 0.090f;   // 90 mA

// ------------------------------------------------------
// Global objects
// ------------------------------------------------------
SensorManager    sensor;
USBController    usbController(USB_PINS, USB_NAMES);
BatteryEstimator battEstimator;                         // ← NEW
BoronComm        boron(usbController, battEstimator);   // ← updated ctor
WebServerManager web(usbController);

unsigned long lastSend = 0;

// ------------------------------------------------------
// One-shot threshold configuration
// ------------------------------------------------------
const float THRESHOLD1 = 13.0f;
const float THRESHOLD2 = 12.8f;
const float RECOVER    = 13.1f;

const int   AVG_SAMPLES = 200;
const float EQUAL_A     = 0.005f;   // 5 mA

bool shed118Done = false;
bool shed115Done = false;

// ------------------------------------------------------
// Helpers
// ------------------------------------------------------
static float sampleAvgCurrentA(SensorManager &s, int outletIdx) {
    double sum = 0.0;

    for (int k = 0; k < AVG_SAMPLES; k++) {
        float a = 0.0f;

        switch (outletIdx) {
            case 0: a = s.readUsb1Current(); break;
            case 1: a = s.readUsb2Current(); break;
            case 2: a = s.readUsb3Current(); break;
            case 3: a = s.readUsb4Current(); break;
            default: break;
        }

        sum += a;
        yield();
    }

    return (float)(sum / (double)AVG_SAMPLES);
}

static void shedOneLevel(SensorManager &s, USBController &usb) {
    int candidates[USBController::NUM_USB];
    int n = 0;

    for (int i = 0; i < USBController::NUM_USB; i++) {
        if (!usb.isHighPriority(i) &&
             usb.usbState[i] == "ON" &&
            !usb.isEmergencyOff(i)) {
            candidates[n++] = i;
        }
    }

    if (n == 0) return;

    float avgA[USBController::NUM_USB] = {0, 0, 0, 0};
    for (int j = 0; j < n; j++) {
        avgA[candidates[j]] = sampleAvgCurrentA(s, candidates[j]);
    }

    float maxA = -1.0f, minA = 999999.0f;
    int   maxIdx = candidates[0];

    for (int j = 0; j < n; j++) {
        int   idx = candidates[j];
        float a   = avgA[idx];
        if (a > maxA) { maxA = a; maxIdx = idx; }
        if (a < minA)   minA = a;
    }

    if ((maxA - minA) <= EQUAL_A) {
        // All within 5 mA — shed them all
        for (int j = 0; j < n; j++) {
            usb.setEmergencyOff(candidates[j], true);
        }
    } else {
        // Shed only the highest-draw outlet
        usb.setEmergencyOff(maxIdx, true);
    }
}

// ------------------------------------------------------
// Setup
// ------------------------------------------------------
void setup() {
    Serial.begin(9600);

    sensor.begin();
    usbController.begin();
    boron.begin();    // ← sends "REQ:BATT" immediately
    web.begin();
}

// ------------------------------------------------------
// Loop
// ------------------------------------------------------
void loop() {
    usbController.tickEpoch();
    web.handle();
    boron.process();   // ← parses BATT reply and configures battEstimator

    float c1, c2, c3, c4;
    float v1, v2, v3, v4;
    float vSupply;

    sensor.readAll(
        c1, c2, c3, c4,
        v1, v2, v3, v4,
        vSupply
    );

    // --------------------------------------------------
    // One-shot threshold shedding
    // --------------------------------------------------
    if (vSupply >= RECOVER) {
        shed118Done = false;
        shed115Done = false;
        usbController.clearEmergencyAll();
    }

    if (!shed118Done && vSupply <= THRESHOLD1) {
        shed118Done = true;
        shedOneLevel(sensor, usbController);
        Serial.println("DBG:13.0V threshold -> shed level 1");
    }

    if (!shed115Done && vSupply <= THRESHOLD2) {
        shed115Done = true;
        shedOneLevel(sensor, usbController);
        Serial.println("DBG:12.8V threshold -> shed level 2");
    }

    // --------------------------------------------------
    // Runtime estimate
    //
    // Total current = all four outlet sensors + ESP8266 base draw.
    // If the estimator is not yet configured (Boron hasn't replied yet)
    // estimateRuntime() returns -1, which sendAll() encodes as "RUNTIME=NA".
    // --------------------------------------------------
    float totalCurrentA = c1 + c2 + c3 + c4 + BASE_CURRENT_A;
    float runtimeHours  = battEstimator.estimateRuntime(vSupply, totalCurrentA);

    // --------------------------------------------------
    // Telemetry to Boron every 2 seconds
    // --------------------------------------------------
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        boron.sendAll(
            c1, c2, c3, c4,
            v1, v2, v3, v4,
            vSupply,
            runtimeHours    // ← NEW
        );
    }
}
*/
///////////////////////////
// SYSTEM RUNTIME CODE/////
///////////////////////////

/*
#include <Arduino.h>
#include <math.h>

#include "SensorManager.h"
#include "USBController.h"
#include "BoronComm.h"
#include "WebServerManager.h"
#include "BatteryEstimator.h"

// USB pins + names
const uint8_t USB_PINS[4] = { D0, D6, D7, D8 };
const char* USB_NAMES[4]  = { "USB1", "USB2", "USB3", "USB4" };

// ESP/base board current draw
static const float BASE_CURRENT_A = 0.090f;

// Global objects
SensorManager    sensor;
USBController    usbController(USB_PINS, USB_NAMES);
BatteryEstimator battEstimator;
BoronComm        boron(usbController, battEstimator);
WebServerManager web(usbController);

unsigned long lastSend = 0;

// Load shedding thresholds
const float THRESHOLD1 = 13.0f;
const float THRESHOLD2 = 12.8f;
const float RECOVER    = 13.1f;

const int   AVG_SAMPLES = 200;
const float EQUAL_A     = 0.005f;

bool shed118Done = false;
bool shed115Done = false;

static float sampleAvgCurrentA(SensorManager &s, int outletIdx) {
    double sum = 0.0;

    for (int k = 0; k < AVG_SAMPLES; k++) {
        float a = 0.0f;

        switch (outletIdx) {
            case 0: a = s.readUsb1Current(); break;
            case 1: a = s.readUsb2Current(); break;
            case 2: a = s.readUsb3Current(); break;
            case 3: a = s.readUsb4Current(); break;
            default: break;
        }

        sum += fabs(a);
        yield();
    }

    return (float)(sum / (double)AVG_SAMPLES);
}

static void shedOneLevel(SensorManager &s, USBController &usb) {
    int candidates[USBController::NUM_USB];
    int n = 0;

    for (int i = 0; i < USBController::NUM_USB; i++) {
        bool lowPriority = !usb.isHighPriority(i);
        bool isOn = usb.usbState[i] == "ON";
        bool alreadyOff = usb.isEmergencyOff(i);

        if (lowPriority && isOn && !alreadyOff) {
            candidates[n++] = i;
        }
    }

    if (n == 0) return;

    float avgA[USBController::NUM_USB] = {0, 0, 0, 0};

    for (int j = 0; j < n; j++) {
        int idx = candidates[j];
        avgA[idx] = sampleAvgCurrentA(s, idx);
    }

    float maxA = -1.0f;
    float minA = 999999.0f;
    int maxIdx = candidates[0];

    for (int j = 0; j < n; j++) {
        int idx = candidates[j];
        float a = avgA[idx];

        if (a > maxA) {
            maxA = a;
            maxIdx = idx;
        }

        if (a < minA) {
            minA = a;
        }
    }

    if ((maxA - minA) <= EQUAL_A) {
        for (int j = 0; j < n; j++) {
            usb.setEmergencyOff(candidates[j], true);
        }
    } else {
        usb.setEmergencyOff(maxIdx, true);
    }
}

void setup() {
    Serial.begin(9600);

    sensor.begin();
    usbController.begin();
    boron.begin();
    web.begin();
}

void loop() {
    usbController.tickEpoch();
    web.handle();
    boron.process();

    float c1, c2, c3, c4;
    float v1, v2, v3, v4;
    float vSupply;

    sensor.readAll(
        c1, c2, c3, c4,
        v1, v2, v3, v4,
        vSupply
    );

    // Load shedding reset
    if (vSupply >= RECOVER) {
        shed118Done = false;
        shed115Done = false;
        usbController.clearEmergencyAll();
    }

    // First threshold
    if (!shed118Done && vSupply <= THRESHOLD1) {
        shed118Done = true;
        shedOneLevel(sensor, usbController);
        Serial.println("DBG:13.0V threshold -> shed level 1");
    }

    // Second threshold
    if (!shed115Done && vSupply <= THRESHOLD2) {
        shed115Done = true;
        shedOneLevel(sensor, usbController);
        Serial.println("DBG:12.8V threshold -> shed level 2");
    }

    // Runtime calculation
    float totalCurrentA =
        fabs(c1) +
        fabs(c2) +
        fabs(c3) +
        fabs(c4) +
        BASE_CURRENT_A;

    float runtimeHours = battEstimator.estimateRuntime(vSupply, totalCurrentA);

    // Send telemetry + runtime to Boron every 2 seconds
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        boron.sendAll(
            c1, c2, c3, c4,
            v1, v2, v3, v4,
            vSupply,
            runtimeHours
        );
    }
}
    */

#include <Arduino.h>
#include <math.h>

#include "SensorManager.h"
#include "USBController.h"
#include "BoronComm.h"
#include "WebServerManager.h"
#include "BatteryEstimator.h"

// USB pins + names
const uint8_t USB_PINS[4] = { D0, D6, D7, D8 };
const char* USB_NAMES[4]  = { "USB1", "USB2", "USB3", "USB4" };

// ESP/base board current draw
static const float BASE_CURRENT_A = 0.090f;

// Global objects
SensorManager    sensor;
USBController    usbController(USB_PINS, USB_NAMES);
BatteryEstimator battEstimator;
BoronComm        boron(usbController, battEstimator);
WebServerManager web(usbController);

unsigned long lastSend = 0;

// Current comparison settings
const int   AVG_SAMPLES = 200;
const float EQUAL_A     = 0.005f;   // 5 mA

// One-shot shedding latches
bool shedLevel1Done = false;
bool shedLevel2Done = false;

// ------------------------------------------------------
// Battery-based threshold selection
// ------------------------------------------------------
struct BatteryThresholds {
    float normalHigh;
    float normalLow;
    float threshold1;
    float threshold2;
    float recover;
};

BatteryThresholds getBatteryThresholds() {
    if (battEstimator.battChem == BATT_LEAD) {
        return {
            12.7f,  // normalHigh
            12.4f,  // normalLow
            12.3f,  // threshold1
            12.0f,  // threshold2
            12.4f   // recover
        };
    }

    // Default to LFP
    return {
        13.6f,  // normalHigh
        13.1f,  // normalLow
        13.0f,  // threshold1
        12.8f,  // threshold2
        13.1f   // recover
    };
}

// ------------------------------------------------------
// Helpers
// ------------------------------------------------------
static float sampleAvgCurrentA(SensorManager &s, int outletIdx) {
    double sum = 0.0;

    for (int k = 0; k < AVG_SAMPLES; k++) {
        float a = 0.0f;

        switch (outletIdx) {
            case 0: a = s.readUsb1Current(); break;
            case 1: a = s.readUsb2Current(); break;
            case 2: a = s.readUsb3Current(); break;
            case 3: a = s.readUsb4Current(); break;
            default: break;
        }

        sum += fabs(a);
        yield();
    }

    return (float)(sum / (double)AVG_SAMPLES);
}

static void shedOneLevel(SensorManager &s, USBController &usb) {
    int candidates[USBController::NUM_USB];
    int n = 0;

    // Only shed low-priority outlets that are currently ON
    for (int i = 0; i < USBController::NUM_USB; i++) {
        bool lowPriority = !usb.isHighPriority(i);
        bool isOn = usb.usbState[i] == "ON";
        bool alreadyEmergencyOff = usb.isEmergencyOff(i);

        if (lowPriority && isOn && !alreadyEmergencyOff) {
            candidates[n++] = i;
        }
    }

    if (n == 0) return;

    float avgA[USBController::NUM_USB] = {0, 0, 0, 0};

    for (int j = 0; j < n; j++) {
        int idx = candidates[j];
        avgA[idx] = sampleAvgCurrentA(s, idx);
    }

    float maxA = -1.0f;
    float minA = 999999.0f;
    int maxIdx = candidates[0];

    for (int j = 0; j < n; j++) {
        int idx = candidates[j];
        float a = avgA[idx];

        if (a > maxA) {
            maxA = a;
            maxIdx = idx;
        }

        if (a < minA) {
            minA = a;
        }
    }

    // If all candidates are within 5 mA, shed all low-priority candidates
    if ((maxA - minA) <= EQUAL_A) {
        for (int j = 0; j < n; j++) {
            usb.setEmergencyOff(candidates[j], true);
        }
    } else {
        // Otherwise shed only the highest-current low-priority outlet
        usb.setEmergencyOff(maxIdx, true);
    }
}

void setup() {
    Serial.begin(9600);

    sensor.begin();
    usbController.begin();
    boron.begin();
    web.begin();
}

void loop() {
    usbController.tickEpoch();
    web.handle();
    boron.process();

    float c1, c2, c3, c4;
    float v1, v2, v3, v4;
    float vSupply;

    sensor.readAll(
        c1, c2, c3, c4,
        v1, v2, v3, v4,
        vSupply
    );

    BatteryThresholds th = getBatteryThresholds();

    // Reset latches after recovery
    if (vSupply >= th.recover) {
        shedLevel1Done = false;
        shedLevel2Done = false;
        usbController.clearEmergencyAll();
    }

    // First threshold
    if (!shedLevel1Done && vSupply <= th.threshold1) {
        shedLevel1Done = true;
        shedOneLevel(sensor, usbController);

        if (battEstimator.battChem == BATT_LEAD) {
            Serial.println("DBG:LEAD 12.3V threshold -> shed level 1");
        } else {
            Serial.println("DBG:LFP 13.0V threshold -> shed level 1");
        }
    }

    // Second threshold
    if (!shedLevel2Done && vSupply <= th.threshold2) {
        shedLevel2Done = true;
        shedOneLevel(sensor, usbController);

        if (battEstimator.battChem == BATT_LEAD) {
            Serial.println("DBG:LEAD 12.0V threshold -> shed level 2");
        } else {
            Serial.println("DBG:LFP 12.8V threshold -> shed level 2");
        }
    }

    // Runtime calculation
/*
    float totalCurrentA =
        fabs(c1) +
        fabs(c2) +
        fabs(c3) +
        fabs(c4) +
        BASE_CURRENT_A;

    float runtimeHours = battEstimator.estimateRuntime(vSupply, totalCurrentA);
*/
// ------------------------------------------------------
// Runtime calculation
// Only update runtime when total current changes significantly
// ------------------------------------------------------
/*
static bool runtimeReady = false;
static float lastRuntimeCurrentA = 0.0f;
static float displayedRuntimeH = -1.0f;

const float CURRENT_CHANGE_THRESHOLD_A = 0.050f;  // 50 mA

float totalCurrentA =
    fabs(c1) +
    fabs(c2) +
    fabs(c3) +
    fabs(c4) +
    BASE_CURRENT_A;

float rawRuntimeHours = battEstimator.estimateRuntime(vSupply, totalCurrentA);

if (rawRuntimeHours < 0.0f) {
    displayedRuntimeH = -1.0f;
    runtimeReady = false;
} else if (!runtimeReady) {
    displayedRuntimeH = rawRuntimeHours;
    lastRuntimeCurrentA = totalCurrentA;
    runtimeReady = true;
} else {
    float currentChange = fabs(totalCurrentA - lastRuntimeCurrentA);

    if (currentChange >= CURRENT_CHANGE_THRESHOLD_A) {
        displayedRuntimeH = rawRuntimeHours;
        lastRuntimeCurrentA = totalCurrentA;
    }
}
*/
// ------------------------------------------------------
// Runtime calculation
// Update runtime when current changes significantly
// OR when battery voltage changes significantly
// ------------------------------------------------------
static bool runtimeReady = false;
static float lastRuntimeCurrentA = 0.0f;
static float lastRuntimeVoltageV = 0.0f;
static float displayedRuntimeH = -1.0f;

const float CURRENT_CHANGE_THRESHOLD_A = 0.050f;  // 50 mA
const float VOLTAGE_CHANGE_THRESHOLD_V = 0.10f;   // 0.10 V
const float MIN_VALID_SUPPLY_V = 1.0f;            // treat below this as no battery/input

float totalCurrentA =
    fabs(c1) +
    fabs(c2) +
    fabs(c3) +
    fabs(c4) +
    BASE_CURRENT_A;

// If battery/supply reads as disconnected or zero, force runtime to 0
if (vSupply < MIN_VALID_SUPPLY_V) {
    displayedRuntimeH = 0.0f;
    runtimeReady = true;
    lastRuntimeCurrentA = totalCurrentA;
    lastRuntimeVoltageV = vSupply;
} else {
    float rawRuntimeHours = battEstimator.estimateRuntime(vSupply, totalCurrentA);

    if (rawRuntimeHours < 0.0f) {
        displayedRuntimeH = -1.0f;   // sends RUNTIME,NA
        runtimeReady = false;
    } else if (!runtimeReady) {
        displayedRuntimeH = rawRuntimeHours;
        lastRuntimeCurrentA = totalCurrentA;
        lastRuntimeVoltageV = vSupply;
        runtimeReady = true;
    } else {
        float currentChange = fabs(totalCurrentA - lastRuntimeCurrentA);
        float voltageChange = fabs(vSupply - lastRuntimeVoltageV);

        if (currentChange >= CURRENT_CHANGE_THRESHOLD_A ||
            voltageChange >= VOLTAGE_CHANGE_THRESHOLD_V) {
            displayedRuntimeH = rawRuntimeHours;
            lastRuntimeCurrentA = totalCurrentA;
            lastRuntimeVoltageV = vSupply;
        }
    }
}

float runtimeHours = displayedRuntimeH;
    // Telemetry to Boron every 2 seconds
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        boron.sendAll(
            c1, c2, c3, c4,
            v1, v2, v3, v4,
            vSupply,
            runtimeHours
        );
    }
}
