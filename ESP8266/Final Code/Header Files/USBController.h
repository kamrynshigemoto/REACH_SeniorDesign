/*#pragma once
#include <Arduino.h>


class USBController {
public:
    static const int NUM_USB = 4;
    bool schedAck = false;


    USBController(const uint8_t pins[NUM_USB], const char* names[NUM_USB]);


    void begin();
    void tickEpoch();
    int computeDesired(int idx);
    void apply(int idx, int state01);
    void applyManual(int idx, int state01);   // <-- NEW
    String statusLine();


    void setSchedule(int idx, unsigned long start, unsigned long end, int target, bool enabled);
    void setOverride(int idx, int target, unsigned long until);


    unsigned long nowEpoch = 0;


    String usbState[NUM_USB];
    bool scheduleEnabled[NUM_USB];


    unsigned long schedStart[NUM_USB];
    unsigned long schedEnd[NUM_USB];
    int schedTarget[NUM_USB];


    bool overrideActive[NUM_USB];
    int overrideTarget[NUM_USB];
    unsigned long overrideUntil[NUM_USB];


    int defaultState[NUM_USB];   // <-- NEW baseline state


private:
    const uint8_t* mosfetPins;
    const char** usbNames;


    unsigned long lastEpochMillis = 0;
};

*/
/* #pragma once
#include <Arduino.h>

class USBController {
public:
    static const int NUM_USB = 4;
    bool schedAck = false;

    USBController(const uint8_t pins[NUM_USB], const char* names[NUM_USB]);

    void begin();
    void tickEpoch();
    int computeDesired(int idx);
    void apply(int idx, int state01);
    void applyManual(int idx, int state01);
    String statusLine();

    void setSchedule(int idx, unsigned long start, unsigned long end, int target, bool enabled);
    void setOverride(int idx, int target, unsigned long until);

    // --- Priority + Emergency layer ---
    void setPriority(int idx, bool isHigh);
    bool isHighPriority(int idx) const;

    void setEmergencyOff(int idx, bool off);
    bool isEmergencyOff(int idx) const;

    void clearEmergencyAll();

    unsigned long nowEpoch = 0;

    String usbState[NUM_USB];
    bool scheduleEnabled[NUM_USB];

    unsigned long schedStart[NUM_USB];
    unsigned long schedEnd[NUM_USB];
    int schedTarget[NUM_USB];

    bool overrideActive[NUM_USB];
    int overrideTarget[NUM_USB];
    unsigned long overrideUntil[NUM_USB];

    int defaultState[NUM_USB];   // baseline state (manual default)

private:
    const uint8_t* mosfetPins;
    const char** usbNames;

    unsigned long lastEpochMillis = 0;

   
    bool priorityHigh[NUM_USB];   // 1 = high priority, 0 = low priority
    bool emergencyOff[NUM_USB];   // true forces OFF regardless of override/sched/default
};
*/
#pragma once
#include <Arduino.h>

class USBController {
public:
    static const int NUM_USB = 4;
    bool schedAck = false;

    USBController(const uint8_t pins[NUM_USB], const char* names[NUM_USB]);

    void begin();
    void tickEpoch();
    int computeDesired(int idx);
    void apply(int idx, int state01);
    void applyManual(int idx, int state01);
    String statusLine();

    void setSchedule(int idx, unsigned long start, unsigned long end, int target, bool enabled);
    void setOverride(int idx, int target, unsigned long until);

    // Priority control
    void setPriority(int idx, bool isHigh);
    bool isHighPriority(int idx) const;

    // Emergency load shedding
    void setEmergencyOff(int idx, bool off);
    bool isEmergencyOff(int idx) const;
    void clearEmergencyAll();

    unsigned long nowEpoch = 0;

    String usbState[NUM_USB];
    bool scheduleEnabled[NUM_USB];

    unsigned long schedStart[NUM_USB];
    unsigned long schedEnd[NUM_USB];
    int schedTarget[NUM_USB];

    bool overrideActive[NUM_USB];
    int overrideTarget[NUM_USB];
    unsigned long overrideUntil[NUM_USB];

    int defaultState[NUM_USB];

private:
    const uint8_t* mosfetPins;
    const char** usbNames;
    unsigned long lastEpochMillis = 0;

    bool priorityHigh[NUM_USB];
    bool emergencyOff[NUM_USB];
};
