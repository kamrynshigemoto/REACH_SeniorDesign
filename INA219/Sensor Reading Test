#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_INA219.h>


// INA219 instance
Adafruit_INA219 ina219;


void setup() {
  Serial.begin(9600);
  delay(100);


  // Initialize I²C (D2 = SDA, D1 = SCL on NodeMCU)
  Wire.begin(D2, D1);


  // Initialize INA219
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }


  // Apply finer calibration for low-voltage, low-current systems
  ina219.setCalibration_16V_400mA();


  Serial.println("INA219 sensor ready with 16V/400mA calibration");
}


void loop() {
  // Read raw values
  float shunt_mV    = ina219.getShuntVoltage_mV();       // in millivolts
  float shunt_V     = shunt_mV / 1000.0;                 // in volts
  float bus_V       = ina219.getBusVoltage_V();          // voltage at V-
  float load_V      = bus_V + shunt_V;                   // estimated supply/load voltage
  float current_mA  = ina219.getCurrent_mA();            // current in mA
  float power_mW    = ina219.getPower_mW();              // power in mW


  // Print with precision
  Serial.print("Bus (V-):       "); Serial.println(bus_V, 3);
 
  Serial.print("Load Voltage:   "); Serial.println(load_V, 3);
  Serial.print("Current (mA):   "); Serial.println(current_mA, 3);
  Serial.print("Power (mW):     "); Serial.println(power_mW, 3);
  Serial.println("-----------------------------");


  delay(1000);
}
