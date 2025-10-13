#ifndef DS18B20_H
#define DS18B20_H

#include <OneWire.h>
#include <DallasTemperature.h>

// Pin definition for DS18B20 sensor
#define DS18B20_PIN 2  // Change this to the appropriate pin

// Create a OneWire instance
OneWire oneWire(DS18B20_PIN);

// Create a DallasTemperature instance
DallasTemperature sensors(&oneWire);

// Function to initialize the DS18B20 sensor
void initDS18B20() {
    sensors.begin();
}

// Function to read temperature from the DS18B20 sensor
float readTemperature() {
    sensors.requestTemperatures(); // Request temperature readings
    return sensors.getTempCByIndex(0); // Get temperature in Celsius
}

#endif // DS18B20_H