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
