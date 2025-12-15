

#include <Arduino.h>
#include "DHT.h"


#define DHTPIN 2      // GPIO2 (D4 on NodeMCU)
#define DHTTYPE DHT11 // Sensor type


DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial.begin(9600); // UART to Boron
  dht.begin();
}






void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius


  if (!isnan(humidity) && !isnan(temperature)) {
    // Format as query string for Boron webhook
    String payload = "node_name=node-1&temperature=" + String(temperature, 1) +
                     "&humidity=" + String(humidity, 1);
    Serial.println(payload); // Send to Boron
  } else {
    Serial.println("Error reading DHT11");
  }


  delay(2000); // Send every 2 seconds
}
