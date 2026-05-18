//-----------------------------
// Title: ReachBoronManagement.h
//-----------------------------
// Purpose: This program runs on a Particle Boron and manages the REACH power system.
// It communicates with Particle Cloud webhooks to receive outlet schedules, outlet commands,
// outlet priorities, and battery configuration data. It sends schedule, command, priority,
// battery config, and time-sync messages to an ESP8266 over UART. The ESP8266 returns USB
// telemetry, battery voltage, and runtime estimates. The Boron also updates a 2.9" e-ink
// display with USB output status, current draw, voltage, battery voltage, and runtime.
// Dependencies: Particle.h, SPI.h, GxEPD2_BW.h, vector, FreeMonoBold9pt7b.h
// Compiler: Particle Workbench / Particle Device OS
// Author: Jair Pacheco
// OUTPUTS: Serial Monitor, Serial1 UART to ESP8266, Particle Cloud events, 2.9" e-ink display
// INPUTS: Particle webhook responses, Serial1 UART from ESP8266, BUTTON_SCHED D2,
// BUTTON_CMD D3, BUTTON_SENSOR D4
// Expected Cloud Inputs:
//      serverSchedule - CSV schedule data for each outlet
//      serverCommand  - CSV command data in command_id,outlet,state format
//      reachPrio      - CSV priority data in outlet,priority format
//      batteryConfig  - CSV battery config in battery_ah,chemistry,nominal_voltage,updated_at format
// Expected ESP8266 Inputs:
//      RUNTIME,hours
//      USBx,CURRENT=amps,VOLTAGE=volts,STATE=ON/OFF,SUPPLY=battery_voltage
// Expected Outputs:
//      TIME=epoch_time
//      USBx=ON/OFF
//      SCHED,outlet,start,end,target,enabled
//      PRIORITY,outlet,priority
//      BATCFG,batteryAh,chemistry,nominalVoltage
//      sendTelemetry JSON payloads to Particle Cloud
// Versions:
//      V1.X: 5/17/2026 - Added cloud schedule, command, priority, telemetry, battery config,
//                        ESP8266 UART communication, runtime display, and e-ink dashboard support
//-----------------------------

#include "Particle.h"
#include <SPI.h>
#include <GxEPD2_BW.h>
#include <vector>
#include "Fonts/FreeMonoBold9pt7b.h"

// Particle cloud connection is automatic, and the user loop runs in its own thread.
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// =========================
// E-INK DISPLAY SETUP
// Defines the display pins and creates the display object.
// =========================
#define EPD_CS   D5
#define EPD_DC   D6
#define EPD_RST  D7
#define EPD_BUSY D8

GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> display(
    GxEPD2_290_T94_V2(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// Controls when the e-ink display should refresh.
bool displayDirty = true;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_MIN_INTERVAL = 5000;

// Tracks whether a schedule was successfully acknowledged.
bool schedAck = false;

// =========================
// BATTERY CONFIG STATE
// Stores battery configuration received from the database webhook.
// =========================
float batteryAh = 0.0;
String batteryChemistry = "";
float batteryNominalVoltage = 0.0;
bool batteryConfigValid = false;

// =========================
// SYSTEM RUNTIME STATE
// Stores estimated runtime received from the ESP8266.
// =========================
float systemRuntimeHours = 0.0;
bool systemRuntimeValid = false;

// =========================
// UART COMMUNICATION TO ESP8266
// Sends commands/configuration from the Boron to the ESP8266.
// =========================
void sendToESP(const String &msg) {
    Serial1.println(msg);
    Serial.println("Sent to ESP: " + msg);
}

// =========================
// PHYSICAL BUTTON INPUTS
// Each button manually triggers a poll or telemetry action.
// =========================
#define BUTTON_SCHED   D2
#define BUTTON_CMD     D3
#define BUTTON_SENSOR  D4

bool lastSched     = HIGH;
bool lastCmd       = HIGH;
bool lastSensorBtn = HIGH;

// =========================
// USB TELEMETRY STORAGE
// Holds state, current, voltage, and validity for each of the 4 USB outputs.
// =========================
struct UsbTelemetry {
    String state;
    float current;
    float voltage;
    bool  valid;
};

UsbTelemetry usbData[4];

// =========================
// BATTERY VOLTAGE TELEMETRY
// Stores battery/supply voltage reported by the ESP8266.
// =========================
float batteryVoltage = 0.0;
bool batteryValid = false;

// =========================
// FORWARD DECLARATIONS
// Lets setup/loop call functions that are defined later.
// =========================
void scheduleHandler(const char *event, const char *data);
void commandHandler(const char *event, const char *data);
void priorityHandler(const char *event, const char *data);
void batteryConfigHandler(const char *event, const char *data);

void handleESP(const String &lineRaw);
void publishUSBTelemetry(int usbIndex);
void pollScheduleNow();
void pollCommandNow();
void pollBatteryConfigNow();

// =========================
// DASHBOARD DRAWING
// Draws the current state of all 4 USB outputs plus battery/runtime info.
// This only draws to the display buffer; the caller handles the actual refresh.
// =========================
void drawReachDashboard() {
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    int y = 25;
    int rowH = 28;

    // USB 1 row.
    display.setCursor(5, y);
    if (usbData[0].valid) {
        display.printf("sw1: %.1fV %dmA %s",
                       usbData[0].voltage,
                       (int)(usbData[0].current * 1000.0f),
                       usbData[0].state == "ON" ? "1" : "0");
    } else {
        display.print("sw1: --.-V ----mA -");
    }

    display.setCursor(200, y);
    display.print("VS:");

    // USB 2 row and battery voltage.
    y += rowH;
    display.setCursor(5, y);
    if (usbData[1].valid) {
        display.printf("sw2: %.1fV %dmA %s",
                       usbData[1].voltage,
                       (int)(usbData[1].current * 1000.0f),
                       usbData[1].state == "ON" ? "1" : "0");
    } else {
        display.print("sw2: --.-V ----mA -");
    }

    display.setCursor(200, y);
    if (batteryValid) {
        display.printf("%.1f", batteryVoltage);
    } else {
        display.print("--.-");
    }

    // USB 3 row and runtime / schedule ACK indicator.
    y += rowH;
    display.setCursor(5, y);
    if (usbData[2].valid) {
        display.printf("sw3: %.1fV %dmA %s",
                       usbData[2].voltage,
                       (int)(usbData[2].current * 1000.0f),
                       usbData[2].state == "ON" ? "1" : "0");
    } else {
        display.print("sw3: --.-V ----mA -");
    }

    display.setCursor(200, y);
    if (systemRuntimeValid) {
        display.printf("RT:%.1fh", systemRuntimeHours);
    } else {
        display.printf("ACK:%d", schedAck ? 1 : 0);
    }

    // USB 4 row.
    y += rowH;
    display.setCursor(5, y);
    if (usbData[3].valid) {
        display.printf("sw4: %.1fV %dmA %s",
                       usbData[3].voltage,
                       (int)(usbData[3].current * 1000.0f),
                       usbData[3].state == "ON" ? "1" : "0");
    } else {
        display.print("sw4: --.-V ----mA -");
    }
}

// =========================
// SCHEDULE POLLING
// Requests schedule data for all 4 outlets from the Particle webhook/cloud.
// =========================
void pollScheduleNow() {
    for (int outlet = 1; outlet <= 4; outlet++) {
        Particle.publish("serverSchedule", String(outlet), PRIVATE);
        Serial.println("Published serverSchedule: " + String(outlet));
        delay(150);
    }
}

// =========================
// COMMAND POLLING
// Requests pending commands for all 4 outlets.
// =========================
void pollCommandNow() {
    for (int outlet = 1; outlet <= 4; outlet++) {
        String payload = String::format("{\"outlet_id\":%d}", outlet);
        Particle.publish("pollCommand", payload, PRIVATE);
        Serial.println("Published pollCommand: " + payload);
        delay(150);
    }
}

// =========================
// BATTERY CONFIG POLLING
// Requests global battery configuration from the database webhook.
// =========================
void pollBatteryConfigNow() {
    Particle.publish("batteryConfig", "", PRIVATE);
    Serial.println("Published batteryConfig request");
}

// =========================
// SETUP
// Runs once at boot.
// Initializes serial, buttons, telemetry defaults, display, subscriptions,
// and does initial schedule/battery config polling.
// =========================
void setup() {
    delay(500);
    Serial.begin(9600);
    Serial1.begin(9600);

    pinMode(BUTTON_SCHED,  INPUT_PULLUP);
    pinMode(BUTTON_CMD,    INPUT_PULLUP);
    pinMode(BUTTON_SENSOR, INPUT_PULLUP);

    // Initialize all USB telemetry slots with safe defaults.
    for (int i = 0; i < 4; i++) {
        usbData[i].state   = "OFF";
        usbData[i].current = 0.0;
        usbData[i].voltage = 0.0;
        usbData[i].valid   = false;
    }

    Serial.println("=== REACH FIRMWARE (BORON) + E-INK STARTED ===");

    // Initialize e-ink display.
    pinMode(EPD_BUSY, INPUT);
    SPI.begin();
    display.init(115200);
    display.setRotation(1);
    display.setFullWindow();

    // Draw initial dashboard.
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        drawReachDashboard();
    } while (display.nextPage());

    Serial.println("Initial dashboard drawn.");

    // Subscribe to webhook/cloud responses.
    Particle.subscribe("serverSchedule", scheduleHandler, MY_DEVICES);
    Particle.subscribe("serverCommand",  commandHandler,  MY_DEVICES);
    Particle.subscribe("reachPrio",      priorityHandler, MY_DEVICES);
    Particle.subscribe("batteryConfig",  batteryConfigHandler, MY_DEVICES);

    // Get schedules immediately after boot.
    pollScheduleNow();

    // Get battery configuration shortly after boot.
    delay(500);
    Serial.println("BOOT: checking battery config webhook now...");
    pollBatteryConfigNow();
}

// =========================
// MAIN LOOP
// Repeats continuously.
// Handles timed polling, incoming ESP messages, button presses,
// telemetry uploads, and display refreshes.
// =========================
void loop() {
    static unsigned long lastTimeSend          = 0;
    static unsigned long lastTelemetry         = 0;
    static unsigned long lastSchedulePoll      = 0;
    static unsigned long lastCommandPoll       = 0;
    static unsigned long lastPriorityPoll      = 0;
    static unsigned long lastBatteryConfigPoll = millis();

    unsigned long now = millis();

    // Send current Particle cloud time to ESP once per second.
    if (now - lastTimeSend > 1000) {
        sendToESP("TIME=" + String(Time.now()));
        lastTimeSend = now;
    }

    // Upload telemetry for valid USB lines every 10 minutes.
    if (now - lastTelemetry >= 10UL * 60UL * 1000UL) {
        Serial.println("Auto: uploading telemetry for all USB lines...");
        for (int i = 0; i < 4; i++) {
            if (usbData[i].valid) publishUSBTelemetry(i);
        }
        lastTelemetry = now;
    }

    // Refresh all outlet schedules every 60 seconds.
    if (now - lastSchedulePoll >= 60UL * 1000UL) {
        Serial.println("Auto: polling ALL schedules...");
        pollScheduleNow();
        lastSchedulePoll = now;
    }

    // Refresh pending outlet commands every 30 seconds.
    if (now - lastCommandPoll >= 30UL * 1000UL) {
        Serial.println("Auto: polling ALL commands...");
        pollCommandNow();
        lastCommandPoll = now;
    }

    // Poll outlet priorities every 45 seconds.
    if (now - lastPriorityPoll >= 45UL * 1000UL) {
        Serial.println("Auto: BURST polling PRIORITY for all outlets...");

        for (int outlet = 1; outlet <= 4; outlet++) {
            Particle.publish("reachPrio", String(outlet), PRIVATE);
            Serial.println("Published reachPrio: " + String(outlet));
            delay(300);
        }

        lastPriorityPoll = now;
    }

    // Battery configuration changes rarely, so poll once per hour.
    if (now - lastBatteryConfigPoll >= 60UL * 60UL * 1000UL) {
        Serial.println("Auto: polling battery config...");
        pollBatteryConfigNow();
        lastBatteryConfigPoll = now;
    }

    // Read and process messages from ESP8266 over UART.
    while (Serial1.available()) {
        String line = Serial1.readStringUntil('\n');
        if (line.length() > 0) {
            Serial.println("From ESP: " + line);
            handleESP(line);
        }
    }

    // Manual schedule refresh button.
    bool schedNow = digitalRead(BUTTON_SCHED);
    if (lastSched == HIGH && schedNow == LOW) pollScheduleNow();
    lastSched = schedNow;

    // Manual command refresh button.
    bool cmdNow = digitalRead(BUTTON_CMD);
    if (lastCmd == HIGH && cmdNow == LOW) pollCommandNow();
    lastCmd = cmdNow;

    // Manual telemetry upload and battery config refresh button.
    bool sensorNow = digitalRead(BUTTON_SENSOR);
    if (lastSensorBtn == HIGH && sensorNow == LOW) {
        for (int i = 0; i < 4; i++) {
            if (usbData[i].valid) publishUSBTelemetry(i);
        }

        pollBatteryConfigNow();
    }
    lastSensorBtn = sensorNow;

    // Refresh the e-ink display only when data changed and enough time has passed.
    if (displayDirty && (now - lastDisplayUpdate > DISPLAY_MIN_INTERVAL)) {
        display.setFullWindow();
        display.firstPage();
        do {
            display.fillScreen(GxEPD_WHITE);
            drawReachDashboard();
        } while (display.nextPage());

        lastDisplayUpdate = now;
        displayDirty = false;
        Serial.println("Dashboard refreshed.");
    }
}

// =========================
// SCHEDULE HANDLER
// Parses schedule CSV from the webhook.
// Chooses the active schedule if one is currently running;
// otherwise chooses the next upcoming schedule.
// Sends the selected schedule to the ESP8266.
// =========================
void scheduleHandler(const char *event, const char *data) {
    Serial.println("=== scheduleHandler triggered ===");

    if (!data) return;

    String csv = String(data);
    csv.trim();
    Serial.println("Raw CSV:\n" + csv);

    // Ignore simple echo responses like "1".
    if (csv.length() <= 2 && isdigit(csv.charAt(0))) return;

    // Split CSV response into individual lines.
    std::vector<String> lines;
    int start = 0;
    while (true) {
        int idx = csv.indexOf('\n', start);
        if (idx < 0) {
            String line = csv.substring(start);
            line.trim();
            if (line.length() > 0) lines.push_back(line);
            break;
        }
        String line = csv.substring(start, idx);
        line.trim();
        if (line.length() > 0) lines.push_back(line);
        start = idx + 1;
    }

    if (lines.size() < 3) return;

    // Extract outlet number from the CSV metadata.
    int outlet = 0;
    {
        String line = lines[1];
        int comma = line.indexOf(',');
        if (comma > 0) outlet = line.substring(comma + 1).toInt();
    }

    if (outlet < 1 || outlet > 4) return;

    // Hold parsed current and next schedule values.
    int currentStart = 0, currentEnd = 0, currentTarget = 0, currentEnabled = 0;
    int nextStart = 0, nextEnd = 0, nextTarget = 0, nextEnabled = 0;

    // Parse rows labeled "current" and "next".
    for (size_t i = 3; i < lines.size(); i++) {
        String line = lines[i];
        std::vector<String> parts;

        int s = 0;
        while (true) {
            int c = line.indexOf(',', s);
            if (c < 0) {
                parts.push_back(line.substring(s));
                break;
            }
            parts.push_back(line.substring(s, c));
            s = c + 1;
        }

        if (parts.size() < 8) continue;

        String which = parts[0];
        which.trim();

        bool emptyRow = true;
        for (size_t k = 1; k < parts.size(); k++) {
            if (parts[k].length() > 0) emptyRow = false;
        }

        if (which == "current" && !emptyRow) {
            currentStart   = parts[4].toInt();
            currentEnd     = parts[5].toInt();
            currentTarget  = parts[6].toInt();
            currentEnabled = parts[7].toInt();
        }

        if (which == "next" && !emptyRow) {
            nextStart   = parts[4].toInt();
            nextEnd     = parts[5].toInt();
            nextTarget  = parts[6].toInt();
            nextEnabled = parts[7].toInt();
        }
    }

    // Decide which schedule should be sent to the ESP.
    int nowEpoch = Time.now();
    int startToSend = 0, endToSend = 0, targetToSend = 0, enabledToSend = 0;

    if (currentStart > 0 && nowEpoch >= currentStart && nowEpoch < currentEnd) {
        startToSend   = currentStart;
        endToSend     = currentEnd;
        targetToSend  = currentTarget;
        enabledToSend = currentEnabled;
    } else if (nextStart > 0) {
        startToSend   = nextStart;
        endToSend     = nextEnd;
        targetToSend  = nextTarget;
        enabledToSend = nextEnabled;
    } else {
        schedAck = false;
        displayDirty = true;
        return;
    }

    // Format and send schedule command to ESP.
    String schedMsg =
        "SCHED," +
        String(outlet) + "," +
        String(startToSend) + "," +
        String(endToSend) + "," +
        String(targetToSend) + "," +
        String(enabledToSend);

    sendToESP(schedMsg);

    schedAck = (enabledToSend == 1);
    displayDirty = true;
}

// =========================
// PRIORITY HANDLER
// Parses priority CSV from the webhook and forwards each outlet priority to ESP.
// =========================
void priorityHandler(const char *event, const char *data) {
    Serial.println("=== priorityHandler triggered ===");

    if (!data) return;

    String csv = String(data);
    csv.trim();
    Serial.println("Raw priority CSV:\n" + csv);

    // Split response into lines.
    std::vector<String> lines;
    int start = 0;
    while (true) {
        int idx = csv.indexOf('\n', start);
        if (idx < 0) {
            String line = csv.substring(start);
            line.trim();
            if (line.length() > 0) lines.push_back(line);
            break;
        }
        String line = csv.substring(start, idx);
        line.trim();
        if (line.length() > 0) lines.push_back(line);
        start = idx + 1;
    }

    // Each line should be outlet,priority.
    for (size_t i = 0; i < lines.size(); i++) {
        String line = lines[i];
        int comma = line.indexOf(',');
        if (comma < 0) continue;

        int outlet   = line.substring(0, comma).toInt();
        int priority = line.substring(comma + 1).toInt();

        if (outlet < 1 || outlet > 4) continue;

        Serial.printlnf("Parsed priority: outlet=%d priority=%d", outlet, priority);

        String msg = String::format("PRIORITY,%d,%d", outlet, priority);
        sendToESP(msg);
    }
}

// =========================
// BATTERY CONFIG HANDLER
// Parses battery configuration from webhook CSV and sends it to ESP.
//
// Expected CSV:
// battery_ah,chemistry,nominal_voltage,updated_at
// 100,LFP,12.8,2026-04-28 14:32:10
//
// Message sent to ESP:
// BATCFG,100.0,LFP,12.80
// =========================
void batteryConfigHandler(const char *event, const char *data) {
    Serial.println("=== batteryConfigHandler triggered ===");

    if (!data) return;

    String csv = String(data);
    csv.trim();

    Serial.println("Raw battery CSV:\n" + csv);

    if (csv.length() == 0) {
        Serial.println("Battery config response was empty.");
        return;
    }

    if (csv.startsWith("error")) {
        Serial.println("Battery config webhook returned error.");
        return;
    }

    // Expect first line to be header and second line to contain data.
    int newline = csv.indexOf('\n');
    if (newline < 0) {
        Serial.println("Battery config CSV missing data row.");
        return;
    }

    String dataLine = csv.substring(newline + 1);
    dataLine.trim();

    // Split the battery data row into fields.
    String parts[4];
    int idx = 0;
    int last = 0;

    for (int i = 0; i <= (int)dataLine.length(); i++) {
        if (i == (int)dataLine.length() || dataLine.charAt(i) == ',') {
            if (idx < 4) {
                parts[idx] = dataLine.substring(last, i);
                parts[idx].trim();
                idx++;
            }
            last = i + 1;
        }
    }

    if (idx < 3) {
        Serial.println("Battery config CSV parse failed.");
        return;
    }

    // Store battery config locally.
    batteryAh = parts[0].toFloat();
    batteryChemistry = parts[1];
    batteryNominalVoltage = parts[2].toFloat();
    batteryConfigValid = true;

    Serial.printlnf(
        "Parsed battery config: %.1fAh %s %.2fV",
        batteryAh,
        batteryChemistry.c_str(),
        batteryNominalVoltage
    );

    // Forward battery config to ESP.
    String msg = String::format(
        "BATCFG,%.1f,%s,%.2f",
        batteryAh,
        batteryChemistry.c_str(),
        batteryNominalVoltage
    );

    sendToESP(msg);
    Serial.println("Battery config sent to ESP: " + msg);
}

// =========================
// COMMAND HANDLER
// Parses a pending command from the webhook and sends the USB ON/OFF command to ESP.
// Then marks the command as completed.
// =========================
void commandHandler(const char *event, const char *data) {
    Serial.println("=== commandHandler triggered ===");
    Serial.println("Raw CMD CSV: " + String(data));

    String csv = String(data);
    csv.trim();

    if (csv == "" || csv == "none") return;

    // Expected format: command_id,outlet,state
    int parts[3] = {0};
    int idx = 0;
    int last = 0;

    for (int i = 0; i < (int)csv.length(); i++) {
        if (csv.charAt(i) == ',' || i == (int)csv.length() - 1) {
            String token;
            if (i == (int)csv.length() - 1)
                token = csv.substring(last);
            else
                token = csv.substring(last, i);

            token.trim();
            if (idx < 3) parts[idx++] = token.toInt();
            last = i + 1;
        }
    }

    int command_id = parts[0];
    int outlet     = parts[1];
    int state      = parts[2];

    if (outlet < 1 || outlet > 4) return;

    // Convert command into ESP USB command format.
    String usbCmd = "USB" + String(outlet) + "=" + (state == 1 ? "ON" : "OFF");
    sendToESP(usbCmd);

    // Tell the backend that this command was handled.
    Particle.publish("markCommandDone", String(command_id), PRIVATE);
}

// =========================
// ESP TELEMETRY PARSER
// Parses incoming UART messages from ESP8266.
// Handles runtime updates, USB telemetry, and battery/supply voltage.
// =========================
void handleESP(const String &lineRaw) {
    String line = lineRaw;
    line.trim();

    // Runtime message format:
    // RUNTIME,12.5
    if (line.startsWith("RUNTIME,")) {
        String runtimeStr = line.substring(8);
        runtimeStr.trim();

        systemRuntimeHours = runtimeStr.toFloat();
        systemRuntimeValid = true;
        displayDirty = true;

        Serial.println("System runtime updated: " + String(systemRuntimeHours, 2) + " hours");
        return;
    }

    // Ignore anything that is not USB telemetry.
    if (!line.startsWith("USB")) return;

    int usb = line.substring(3, 4).toInt();
    if (usb < 1 || usb > 4) return;

    int idx = usb - 1;

    float current = 0.0;
    float voltage = 0.0;
    String state = "OFF";

    // Parse current field.
    int cPos = line.indexOf("CURRENT=");
    if (cPos > 0) {
        int end = line.indexOf(',', cPos);
        if (end < 0) end = line.length();
        current = line.substring(cPos + 8, end).toFloat();
    }

    // Parse voltage field.
    int vPos = line.indexOf("VOLTAGE=");
    if (vPos > 0) {
        int end = line.indexOf(',', vPos);
        if (end < 0) end = line.length();
        voltage = line.substring(vPos + 8, end).toFloat();
    }

    // Parse state field.
    int sPos = line.indexOf("STATE=");
    if (sPos > 0) {
        int end = line.indexOf(',', sPos);
        if (end < 0) end = line.length();
        state = line.substring(sPos + 6, end);
        state.trim();
    }

    // Parse shared battery/supply voltage if included.
    int supPos = line.indexOf("SUPPLY=");
    if (supPos > 0) {
        int end = line.indexOf(',', supPos);
        if (end < 0) end = line.length();
        batteryVoltage = line.substring(supPos + 7, end).toFloat();
        batteryValid = true;
        Serial.println("Battery voltage updated: " + String(batteryVoltage, 3));
    }

    // Store telemetry for this USB channel.
    usbData[idx].state   = state;
    usbData[idx].current = current;
    usbData[idx].voltage = voltage;
    usbData[idx].valid   = true;

    Serial.println("Updated USB" + String(usb) +
        " state=" + state +
        " I=" + String(current, 3) +
        " V=" + String(voltage, 3));

    displayDirty = true;
}

// =========================
// TELEMETRY PUBLISH
// Builds and publishes a JSON telemetry payload for one USB outlet.
// =========================
void publishUSBTelemetry(int usbIndex) {
    int outlet_id = usbIndex + 1;

    float battery_v = batteryValid ? batteryVoltage : 0.0;
    float current   = usbData[usbIndex].current * 1000.0f;
    float voltage   = usbData[usbIndex].voltage;
    float power     = current * voltage;
    String state    = usbData[usbIndex].state;

    String ts = Time.format(Time.now(), "%Y-%m-%d %H:%M:%S");

    String payload = String::format(
        "{"
        "\"api_key\":\"REACH25\","
        "\"outlet_id\":%d,"
        "\"ts\":\"%s\","
        "\"battery_v\":%.3f,"
        "\"sensor_v\":%.3f,"
        "\"current_a\":%.3f,"
        "\"power_w\":%.3f,"
        "\"state\":\"%s\""
        "}",
        outlet_id,
        ts.c_str(),
        battery_v,
        voltage,
        current,
        power,
        state.c_str()
    );

    Particle.publish("sendTelemetry", payload, PRIVATE);
    Serial.println("Published sendTelemetry (USB" + String(outlet_id) + "): " + payload);
}
