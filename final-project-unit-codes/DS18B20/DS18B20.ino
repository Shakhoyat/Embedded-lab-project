#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into digital pin 6 on Arduino
#define ONE_WIRE_BUS 6

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  Serial.println("DS18B20 Temperature Sensor Test");
  Serial.println("================================");
  
  // Start up the library
  sensors.begin();
  
  // Check how many sensors are found
  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 sensor(s)");
  
  delay(2000);
}

void loop() {
  Serial.println("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  
  Serial.println("=== DS18B20 Readings ===");
  
  // Loop through each device and print temperature
  for (int i = 0; i < sensors.getDeviceCount(); i++) {
    float tempC = sensors.getTempCByIndex(i);
    
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    
    if (tempC != DEVICE_DISCONNECTED_C) {
      Serial.print(tempC);
      Serial.print(" °C | ");
      Serial.print((tempC * 9.0 / 5.0) + 32.0); // Convert to Fahrenheit
      Serial.println(" °F");
    } else {
      Serial.println("ERROR: Could not read temperature");
    }
  }
  
  Serial.println("========================");
  Serial.println();
  
  delay(3000); // Wait 3 seconds between readings
}