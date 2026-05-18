//-----------------------------
// Title: WebServerManager.cpp
//-----------------------------
// Purpose: Implements the ESP8266 web server used to display a USB control page,
// return USB status information, and process browser-based ON/OFF commands.
// Dependencies: Arduino.h, WebServerManager.h
// Compiler: Platform IO
// Author: Juan Jimenez
// OUTPUTS: HTML control page, HTTP status responses, USB control updates
// INPUTS: HTTP route requests, idx/state query parameters, USBController data
// Versions:
//      V1.0: 5/18/2026 - Added web page, status route, and USB control route
//-----------------------------

#include <Arduino.h>
#include "WebServerManager.h"


const char CONTROL_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
<title>USB Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
<h2>USB MOSFET Switches</h2>


<script>
async function ctrl(i, s){
  await fetch('/control?idx='+i+'&state='+s);
  location.reload();
}
</script>


<button onclick="ctrl(0,'ON')">USB1 ON</button>
<button onclick="ctrl(0,'OFF')">USB1 OFF</button><br>


<button onclick="ctrl(1,'ON')">USB2 ON</button>
<button onclick="ctrl(1,'OFF')">USB2 OFF</button><br>


<button onclick="ctrl(2,'ON')">USB3 ON</button>
<button onclick="ctrl(2,'OFF')">USB3 OFF</button><br>


<button onclick="ctrl(3,'ON')">USB4 ON</button>
<button onclick="ctrl(3,'OFF')">USB4 OFF</button><br>


<pre id="s"></pre>


<script>
async function refresh(){
  let r = await fetch('/status');
  document.getElementById('s').textContent = await r.text();
}
refresh();
</script>


</body>
</html>
)HTML";


WebServerManager::WebServerManager(USBController &usb)
    : server(80), usbRef(usb) {}


void WebServerManager::begin() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("esp8266PCB_hotspot", "SeniorDesign25");


    server.on("/", std::bind(&WebServerManager::handleRoot, this));
    server.on("/status", std::bind(&WebServerManager::handleStatus, this));
    server.on("/control", std::bind(&WebServerManager::handleControl, this));


    server.begin();
}


void WebServerManager::handle() {
    server.handleClient();
}


void WebServerManager::handleRoot() {
    server.send_P(200, "text/html", CONTROL_PAGE);
}


void WebServerManager::handleStatus() {
    server.send(200, "text/plain", usbRef.statusLine());
}


void WebServerManager::handleControl() {
    if (!server.hasArg("idx") || !server.hasArg("state")) {
        server.send(400, "text/plain", "Missing args");
        return;
    }


    int idx = server.arg("idx").toInt();
    String state = server.arg("state");


    int target = (state == "ON") ? 1 : 0;


    usbRef.setOverride(idx, target, usbRef.nowEpoch + 86400);
    usbRef.applyManual(idx, target);


    server.send(200, "text/plain", usbRef.statusLine());
}



