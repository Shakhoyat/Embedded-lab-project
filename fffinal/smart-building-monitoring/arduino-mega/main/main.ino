#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "sensors/dht11.h"
#include "sensors/ds18b20.h"
#include "sensors/flame_sensor.h"
#include "sensors/mq2.h"
#include "sensors/mq135.h"
#include "components/buzzer.h"
#include "components/led.h"
#include "components/lcd_display.h"
#include "config/pin_definitions.h"

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(9600);
    dht.begin();
    sensors.begin();
    initBuzzer();
    initLEDs();
    initLCD();

    // Initial display message
    displayMessage("Initializing...");
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    sensors.requestTemperatures();
    float ds18b20Temp = sensors.getTempCByIndex(0);
    int flameStatus = readFlameSensor();
    int mq2Value = readMQ2();
    int mq135Value = readMQ135();

    // Display sensor readings
    displaySensorData(temperature, humidity, ds18b20Temp, flameStatus, mq2Value, mq135Value);

    // Control LEDs and Buzzer based on sensor data
    if (flameStatus == HIGH) {
        activateBuzzer();
        turnOnRedLED();
    } else {
        deactivateBuzzer();
        turnOffRedLED();
    }

    if (mq2Value > MQ2_THRESHOLD) {
        activateBuzzer();
        turnOnRedLED();
    }

    if (mq135Value > MQ135_THRESHOLD) {
        activateBuzzer();
        turnOnRedLED();
    }

    // Send data to ESP32 for Firebase transmission
    sendDataToESP(temperature, humidity, ds18b20Temp, flameStatus, mq2Value, mq135Value);

    delay(2000); // Delay for stability
}