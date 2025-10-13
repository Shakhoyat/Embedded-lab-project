#ifndef LED_H
#define LED_H

// Pin definitions for LEDs
#define GREEN_LED_PIN 10
#define RED_LED_PIN 11

// Function to initialize the LEDs
void initLEDs() {
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
}

// Function to turn on the green LED
void turnOnGreenLED() {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
}

// Function to turn on the red LED
void turnOnRedLED() {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
}

// Function to turn off both LEDs
void turnOffLEDs() {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
}

#endif // LED_H