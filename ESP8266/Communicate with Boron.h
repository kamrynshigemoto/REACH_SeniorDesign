//--------------------------------------------------
//Purpose: The purpose of this program to set up an ESP8266 so that it can act as a hotspot for other devices (our project).
//Inputs:
//Outputs: Connectivity for other devices (like my laptop) to connect to ESP8266 and access local webpage.
//Date: November 11, 2025
//Compiler: Platformio
//Author: Kamryn Shigemoto
//Versions:
//          V1 - original version: hosts local webpage
//--------------------------------------------------
//File Dependencies:
//--------------------------------------------------
// "Arduino.h" for Arduino functions, "ESP8266WiFi.h" to connect to WiFI
//--------------------------------------------------
//Main Program
//--------------------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>


//credentials to connect to ESP8266's WiFi ntwk (it's acting as AP)
#define ESP_ssid "esp8266_hotspot"
#define ESP_password "SeniorDesign25"


//instantiate HTTP server object to listen on TCP port 80 (HTTP)
ESP8266WebServer server(80);


//initialize relay pin
#define relayPin D5


//set up serial connection to Boron
SoftwareSerial boronSerial(D6, D7); //the RX and TX pins


//simple state model (in place of Boron communication to intialize usb1 status)
String usb1 = "OFF";  //usb1 stores status of usb1
String statusLine() {
  return "STATUS:USB1=" + usb1;
}


//make sure no cache used to store old status, want current updates
void noCache() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
}


//format local webpage
const char CONTROL_PAGE[] PROGMEM = R"HTML(
<!doctype html><meta name=viewport content="width=device-width,initial-scale=1">
<title>REACH - Remote Energy Access and Control Hub</title>
<style>
  body{font-family:system-ui;margin:16px}
  button{padding:10px 14px;margin:6px;border:0;border-radius:8px;font-size:16px}
  #s{white-space:pre-wrap;border:1px solid #ddd;padding:10px;border-radius:8px;margin-top:12px}
</style>


<h2>REACH Control Hub</h2>
<div>
  <button onclick="ctrl(1,'ON')">USB1 ON</button>
  <button onclick="ctrl(1, 'OFF')">USB1 OFF</button>
  <button onclick="refresh()">Refresh Status</button>
</div>
<pre id="s">loading...</pre>
<script>
const out = () => document.getElementById('s');


function showusb1(text) {
  const m = text.match(/USB1=(ON|OFF)/);
  out().textContent = 'USB1: ' + (m ? m[1] : 'unknown');
}


async function refresh() {
  const r = await fetch('/status', {cache: 'no-store'});
  showusb1(await r.text());
}


async function ctrl(port,state){
  const r = await fetch(`/control?port=${port}&state=${state}`, {cache: 'no-store'});
  await refresh();
}


//initial refresh and then periodically check status every 1 minute
refresh();
setInterval(refresh, 60000);
</script>
)HTML";


//handle / (home page)
void handleRoot() {
  server.setContentLength(strlen_P(CONTROL_PAGE));  //hints response size to the server
  server.send_P(200, "text/html", CONTROL_PAGE);  //send content with HTTP status 200, and content type is text/html
}


//handle to get status of outlet (HTTP GET)
void handleStatus() {
  noCache();
  //placeholder for Boron communication; for now return fake data as plain text
  server.send(200, "text/plain", statusLine() + "\n");
}


//function to send data to Boron
  void sendtoboron(String message) {
    boronSerial.println(message);
    Serial.print("Sent to Boron: ");
    Serial.println(message);
  }


//handles control and reads query parameters
void handleControl() {
  String port = server.arg("port");
  String state = server.arg("state");
  if (port != "1" || (state != "ON" && state != "OFF")) {
    noCache();
    server.send(400, "text/plain", "Error: invalid port or state\n");
    return;
  }


  //placeholder for Boron communication
  usb1 = state;
  noCache();
  String msg = "ACK for USB" + port + "=" + state + "\n";
  server.send(200, "text/plain", msg);


  //update relay status and send to Boron
  digitalWrite(relayPin, usb1 == "ON" ? HIGH : LOW);
  sendtoboron("ESP: USB1 changed to " + usb1);
}


//code for setup
void setup() {
  Serial.begin(9600); //was 115200
  boronSerial.begin(9600);


  //set up for relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);


  //configure esp8266 into AP
  Serial.println("Setting up ESP8266 as AP...");
  WiFi.mode(WIFI_AP);   //set up esp8266 as AP: creating its own WiFi ntwk
  WiFi.softAP(ESP_ssid, ESP_password);
  IPAddress myIP = WiFi.softAPIP(); //returns IP address of ESP8266's WiFi ntwk
  Serial.print("ESP8266 AP IP Address: ");
  Serial.println(myIP);
  Serial.println("ESP8266 Network Available");


  //print url user can connect to
  Serial.println("Connect to the Wi-Fi network: esp8266_hotspot");
  Serial.println();
  Serial.println("Open this URL in your browser:");
  Serial.print("    http://");
  Serial.print(myIP);
  Serial.println("/");


  server.on("/", handleRoot);   //for HTML page
  server.on("/status", handleStatus);   //returns status text
  server.on("/control", handleControl); //accepts control commands
  server.begin();
  Serial.println("REACH HTTP server started");
}


void loop() {
  //delay(2500);
  static unsigned long lastPrint = 0;
  static unsigned long lastStatus = 0;


  //process incoming HTTP requests and send to handler functions
  server.handleClient();


  //print number of devices connected to network
  if (millis() - lastPrint > 5000) {
    Serial.printf("Connected clients: %d\n", WiFi.softAPgetStationNum());
    lastPrint = millis();
  }


  //periodic status update sent to Boron
  if (millis() - lastStatus > 10000) {
    sendtoboron("ESP status: USB1=" + usb1);
    lastStatus = millis();
  }


  //listen for info from Boron
  if (boronSerial.available()) {
    String command = boronSerial.readStringUntil('\n');
    command.trim();
    Serial.print("Received from Boron: ");
    Serial.println(command);


    if (command == "USB1=ON") {
      usb1 = "ON";
    }
    else if (command == "USB1=OFF") {
      usb1 = "OFF";
    }
    else {
      Serial.println("Error: Unknown command from Boron");
    }


    //update relay status
    digitalWrite(relayPin, usb1 == "ON" ? HIGH : LOW);
  }
}
