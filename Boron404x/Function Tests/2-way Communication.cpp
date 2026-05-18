#include "Particle.h"


unsigned long lastToggle = 0;
bool usbState = false;


void setup() {
  Serial.begin(9600);      
  Serial1.begin(9600);     // UART to ESP
  pinMode(D6, OUTPUT);     // LED indicator
  Serial.println("Boron ready to send ON/OFF commands to ESP");
}


void loop() {
  // Every 5 seconds, alternate ON/OFF
  if (millis() - lastToggle > 5000) {
    usbState = !usbState;  // flip state


    if (usbState) {
      Serial1.println("USB1=ON");   // send ON to ESP
      Serial.println("Sent to ESP: USB1=ON");
      digitalWrite(D6, HIGH);       // LED ON when ON command sent
    } else {
      Serial1.println("USB1=OFF");  // send OFF to ESP
      Serial.println("Sent to ESP: USB1=OFF");
      digitalWrite(D6, LOW);        // LED OFF when OFF command sent
    }


    lastToggle = millis();
  }


  // Optional: still listen for replies from ESP
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();
    Serial.println("Received from ESP: " + msg);
  }
}
