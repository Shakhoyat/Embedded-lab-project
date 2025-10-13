#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <FirebaseESP32.h>
#include <WiFi.h>
#include "wifi_config.h"

// Firebase configuration
#define FIREBASE_HOST "your-firebase-project.firebaseio.com"
#define FIREBASE_AUTH "your-firebase-database-secret"

// Firebase data object
FirebaseData firebaseData;

// Function to initialize Firebase
void initFirebase() {
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);
}

// Function to send data to Firebase
void sendDataToFirebase(const String& path, const String& data) {
    if (Firebase.setString(firebaseData, path, data)) {
        Serial.println("Data sent successfully: " + data);
    } else {
        Serial.print("Error sending data: ");
        Serial.println(firebaseData.errorReason());
    }
}

// Function to retrieve data from Firebase
String getDataFromFirebase(const String& path) {
    if (Firebase.getString(firebaseData, path)) {
        if (firebaseData.dataType() == "string") {
            return firebaseData.stringData();
        }
    } else {
        Serial.print("Error retrieving data: ");
        Serial.println(firebaseData.errorReason());
    }
    return "";
}

#endif // FIREBASE_HANDLER_H