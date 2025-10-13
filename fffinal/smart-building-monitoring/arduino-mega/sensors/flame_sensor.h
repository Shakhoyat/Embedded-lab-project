#ifndef FLAME_SENSOR_H
#define FLAME_SENSOR_H

#include <Arduino.h>

// Define the pin for the flame sensor
const int flameSensorPin = A0; // Adjust pin as necessary

// Function to initialize the flame sensor
void initFlameSensor() {
    pinMode(flameSensorPin, INPUT);
}

// Function to read the flame sensor status
bool isFlameDetected() {
    int sensorValue = analogRead(flameSensorPin);
    // Assuming a threshold value to determine flame presence
    const int flameThreshold = 300; // Adjust threshold as necessary
    return sensorValue > flameThreshold;
}

#endif // FLAME_SENSOR_H