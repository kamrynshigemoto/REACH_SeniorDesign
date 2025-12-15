#include "Particle.h"


#define BUTTON_PIN D3
#define NODE_NAME "node-3"


void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Button active LOW
    Serial.begin(9600);                // USB debug
    Serial1.begin(9600);               // UART from ESP8266
    delay(3000);
    Serial.println("Ready to send ESP DHT11 data via webhook on button press");
}






void loop() {
    static bool lastState = HIGH;
    bool currentState = digitalRead(BUTTON_PIN);


    if (lastState == HIGH && currentState == LOW) {
        // Button pressed → grab latest UART payload
        String uartData = Serial1.readStringUntil('\n');
        uartData.trim();


        if (uartData.length() > 0) {
            Particle.publish("send_data", uartData, PRIVATE);
            Serial.printf("Published (button press): %s\n", uartData.c_str());
        } else {
            Serial.println("Button pressed, but no UART data available");
        }
    }


    lastState = currentState;
    delay(50); // Debounce
}
