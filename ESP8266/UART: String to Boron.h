#include <Arduino.h>


void setup() {
    Serial.begin(9600);      // UART to Boron
}


void loop() {
    Serial.println("Hello from ESP8266");
    delay(1000);
}
