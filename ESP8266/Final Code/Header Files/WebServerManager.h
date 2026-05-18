//-----------------------------
// Title: WebServerManager.h
//-----------------------------
// Purpose: Declares the WebServerManager class used by the ESP8266 to host a local web server
// for viewing USB outlet status and manually controlling USB outputs through HTTP routes.
// Dependencies: Arduino.h, ESP8266WebServer.h, USBController.h
// Compiler: Arduino IDE / PlatformIO for ESP8266
// Author: Juan Jimenez
// OUTPUTS: Web server routes for root page, status page, and USB control requests
// INPUTS: HTTP requests from browser/client, USBController reference
// Versions:
//      V1.0: 5/18/2026 - Declared local web server manager for USB control and status
//-----------------------------
#pragma once
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "USBController.h"

class WebServerManager {
public:
    WebServerManager(USBController &usb);

    void begin();
    void handle();

private:
    ESP8266WebServer server;
    USBController &usbRef;

    void handleRoot();
    void handleStatus();
    void handleControl();
};
