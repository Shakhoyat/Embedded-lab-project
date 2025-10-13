#include <WiFi.h>
#include <FirebaseESP32.h>
#include "wifi_config.h"
#include "firebase_handler.h"
#include "sensors/dht11.h"
#include "sensors/ds18b20.h"
#include "sensors/flame_sensor.h"
#include "sensors/mq2.h"
#include "sensors/mq135.h"
#include "components/buzzer.h"
#include "components/led.h"

#define FIREBASE_HOST "your_firebase_project.firebaseio.com"
#define FIREBASE_AUTH "your_firebase_database_secret"

FirebaseData firebaseData;

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);

    // Initialize sensors
    dht11_init();
    ds18b20_init();
    flame_sensor_init();
    mq2_init();
    mq135_init();

    // Initialize components
    buzzer_init();
    led_init();
}

void loop() {
    float temperature = dht11_read_temperature();
    float humidity = dht11_read_humidity();
    float ds_temp = ds18b20_read_temperature();
    bool flame_detected = flame_sensor_read();
    float gas_level_mq2 = mq2_read();
    float gas_level_mq135 = mq135_read();

    // Send data to Firebase
    Firebase.setFloat(firebaseData, "/room/temperature", temperature);
    Firebase.setFloat(firebaseData, "/room/humidity", humidity);
    Firebase.setFloat(firebaseData, "/room/ds_temp", ds_temp);
    Firebase.setBool(firebaseData, "/room/flame_detected", flame_detected);
    Firebase.setFloat(firebaseData, "/room/gas_level_mq2", gas_level_mq2);
    Firebase.setFloat(firebaseData, "/room/gas_level_mq135", gas_level_mq135);

    // Control components based on sensor data
    if (flame_detected) {
        buzzer_on();
        led_red_on();
    } else {
        buzzer_off();
        led_red_off();
    }

    if (gas_level_mq2 > THRESHOLD_MQ2 || gas_level_mq135 > THRESHOLD_MQ135) {
        buzzer_on();
        led_red_on();
    } else {
        buzzer_off();
        led_red_off();
    }

    delay(2000); // Adjust delay as needed
}