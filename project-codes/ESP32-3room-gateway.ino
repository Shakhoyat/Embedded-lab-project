/*
 * Smart Building Monitoring System - ESP32 Code
 * Receives data from Arduino Mega and sends to Firebase
 * 
 * Features:
 * - WiFi connectivity
 * - Firebase real-time database integration
 * - JSON data parsing from Arduino Mega
 * - Emergency alert system
 * - Web dashboard data provision
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <time.h>

// ========== WIFI CONFIGURATION ==========
const char* WIFI_SSID = "YOUR_WIFI_SSID";        // Replace with your WiFi name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"; // Replace with your WiFi password

// ========== FIREBASE CONFIGURATION ==========
#define FIREBASE_HOST "smart-building-monitoring-iot.firebaseio.com"  // Replace with your Firebase project
#define FIREBASE_AUTH "wMpzysAkHQ2DnAUe9uoT8Y7YrmV3B6WUe5VHSYIE"          // Replace with your database secret

// ========== PIN DEFINITIONS ==========
#define STATUS_LED 2      // Built-in LED for connection status
#define EMERGENCY_LED 4   // External LED for emergency indication
#define WIFI_LED 5        // LED for WiFi status

// ========== GLOBAL VARIABLES ==========
FirebaseData firebaseData;
String receivedData = "";
unsigned long lastHeartbeat = 0;
unsigned long lastEmergencyCheck = 0;
bool emergencyState = false;
int dataReceiveCount = 0;

// ========== TIME CONFIGURATION ==========
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;      // Adjust for your timezone
const int daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== SMART BUILDING ESP32 GATEWAY ===");
  Serial.println("Initializing system...");
  
  // Initialize pins
  pinMode(STATUS_LED, OUTPUT);
  pinMode(EMERGENCY_LED, OUTPUT);
  pinMode(WIFI_LED, OUTPUT);
  
  // Initial LED state
  digitalWrite(STATUS_LED, LOW);
  digitalWrite(EMERGENCY_LED, LOW);
  digitalWrite(WIFI_LED, LOW);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initialize Firebase
  initializeFirebase();
  
  // Initialize time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Send initial system status
  sendSystemInfo();
  
  Serial.println("ESP32 Gateway ready to receive data from Arduino Mega");
  Serial.println("Waiting for sensor data...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_LED, LOW);
    reconnectWiFi();
  } else {
    digitalWrite(WIFI_LED, HIGH);
  }
  
  // Read data from Arduino Mega
  if (Serial.available()) {
    String incomingData = Serial.readString();
    incomingData.trim();
    
    if (incomingData.startsWith("DATA:")) {
      receivedData = incomingData.substring(5); // Remove "DATA:" prefix
      processArduinoData();
      dataReceiveCount++;
    }
  }
  
  // Send heartbeat every 30 seconds
  if (currentTime - lastHeartbeat >= 30000) {
    sendHeartbeat();
    lastHeartbeat = currentTime;
  }
  
  // Check for emergency conditions every 5 seconds
  if (currentTime - lastEmergencyCheck >= 5000) {
    handleEmergencyAlerts();
    lastEmergencyCheck = currentTime;
  }
  
  // Blink status LED to show system is running
  digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
  delay(500);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_LED, HIGH);
    Serial.println();
    Serial.println("✅ WiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("❌ Failed to connect to WiFi");
    Serial.println("Please check your credentials and try again");
  }
}

void reconnectWiFi() {
  Serial.println("WiFi connection lost. Reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi reconnected!");
    digitalWrite(WIFI_LED, HIGH);
  }
}

void initializeFirebase() {
  Serial.println("Initializing Firebase connection...");
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  
  // Set database read timeout to 1 minute
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  
  // Set database write timeout to 1 minute
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  
  Serial.println("✅ Firebase initialized successfully!");
}

void processArduinoData() {
  Serial.println("Processing data from Arduino Mega...");
  
  // Parse JSON data
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, receivedData);
  
  if (error) {
    Serial.print("❌ JSON parsing error: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Extract data
  float temperature = doc["temperature"];
  float humidity = doc["humidity"];
  float preciseTemp = doc["preciseTemp"];
  int airQuality = doc["airQuality"];
  unsigned long timestamp = doc["timestamp"];
  
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  // Send environmental data to Firebase
  String envPath = "/smartBuilding/environmental";
  Firebase.setFloat(firebaseData, envPath + "/temperature", temperature);
  Firebase.setFloat(firebaseData, envPath + "/humidity", humidity);
  Firebase.setFloat(firebaseData, envPath + "/preciseTemperature", preciseTemp);
  Firebase.setInt(firebaseData, envPath + "/airQuality", airQuality);
  Firebase.setString(firebaseData, envPath + "/lastUpdated", timeString);
  Firebase.setInt(firebaseData, envPath + "/arduinoTimestamp", timestamp);
  
  // Process room data
  processRoomData("bedroom", doc["bedroom"]);
  processRoomData("kitchen", doc["kitchen"]);
  processRoomData("parking", doc["parking"]);
  
  // Update system statistics
  Firebase.setInt(firebaseData, "/smartBuilding/system/dataReceiveCount", dataReceiveCount);
  Firebase.setString(firebaseData, "/smartBuilding/system/lastDataReceived", timeString);
  Firebase.setString(firebaseData, "/smartBuilding/system/esp32Status", "online");
  
  Serial.println("✅ Data successfully sent to Firebase");
  Serial.println("Environmental: T=" + String(temperature) + "°C, H=" + String(humidity) + "%, AQ=" + String(airQuality));
}

void processRoomData(String roomName, JsonObject roomData) {
  bool flame = roomData["flame"];
  int gas = roomData["gas"];
  bool emergency = roomData["emergency"];
  bool dangerous = roomData["dangerous"];
  
  String roomPath = "/smartBuilding/rooms/" + roomName;
  
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  // Send room data to Firebase
  Firebase.setBool(firebaseData, roomPath + "/flameDetected", flame);
  Firebase.setInt(firebaseData, roomPath + "/gasLevel", gas);
  Firebase.setBool(firebaseData, roomPath + "/emergency", emergency);
  Firebase.setBool(firebaseData, roomPath + "/dangerous", dangerous);
  Firebase.setString(firebaseData, roomPath + "/lastUpdated", timeString);
  
  // Determine room status
  String status;
  if (emergency) {
    status = "EMERGENCY";
  } else if (dangerous) {
    status = "WARNING";
  } else {
    status = "SAFE";
  }
  Firebase.setString(firebaseData, roomPath + "/status", status);
  
  // Log emergency events
  if (emergency) {
    String alertPath = "/smartBuilding/alerts/" + String(millis());
    Firebase.setString(firebaseData, alertPath + "/type", "FIRE_EMERGENCY");
    Firebase.setString(firebaseData, alertPath + "/room", roomName);
    Firebase.setString(firebaseData, alertPath + "/timestamp", timeString);
    Firebase.setString(firebaseData, alertPath + "/message", "Fire detected in " + roomName + "! Immediate evacuation required!");
    Firebase.setBool(firebaseData, alertPath + "/acknowledged", false);
    
    Serial.println("🚨 EMERGENCY ALERT: Fire detected in " + roomName + "!");
  }
  
  Serial.println(roomName + ": " + status + " (Gas: " + String(gas) + ")");
}

void sendHeartbeat() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  Firebase.setString(firebaseData, "/smartBuilding/system/esp32Heartbeat", timeString);
  Firebase.setInt(firebaseData, "/smartBuilding/system/uptime", millis() / 1000);
  Firebase.setInt(firebaseData, "/smartBuilding/system/wifiSignalStrength", WiFi.RSSI());
  Firebase.setString(firebaseData, "/smartBuilding/system/esp32IP", WiFi.localIP().toString());
  
  Serial.println("💓 Heartbeat sent - System uptime: " + String(millis() / 1000) + " seconds");
}

void handleEmergencyAlerts() {
  // Check if any room is in emergency state
  Firebase.getString(firebaseData, "/smartBuilding/rooms/bedroom/emergency");
  bool bedroomEmergency = firebaseData.stringData() == "true";
  
  Firebase.getString(firebaseData, "/smartBuilding/rooms/kitchen/emergency");
  bool kitchenEmergency = firebaseData.stringData() == "true";
  
  Firebase.getString(firebaseData, "/smartBuilding/rooms/parking/emergency");
  bool parkingEmergency = firebaseData.stringData() == "true";
  
  bool anyEmergency = bedroomEmergency || kitchenEmergency || parkingEmergency;
  
  if (anyEmergency != emergencyState) {
    emergencyState = anyEmergency;
    
    if (emergencyState) {
      digitalWrite(EMERGENCY_LED, HIGH);
      Firebase.setString(firebaseData, "/smartBuilding/system/buildingStatus", "EMERGENCY");
      Serial.println("🚨 BUILDING IN EMERGENCY STATE!");
      
      // Send emergency notification
      sendEmergencyNotification();
    } else {
      digitalWrite(EMERGENCY_LED, LOW);
      Firebase.setString(firebaseData, "/smartBuilding/system/buildingStatus", "NORMAL");
      Serial.println("✅ Building status returned to normal");
    }
  }
}

void sendEmergencyNotification() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  String notificationPath = "/smartBuilding/notifications/" + String(millis());
  Firebase.setString(firebaseData, notificationPath + "/type", "BUILDING_EMERGENCY");
  Firebase.setString(firebaseData, notificationPath + "/title", "EMERGENCY ALERT");
  Firebase.setString(firebaseData, notificationPath + "/message", "Fire detected in building! Contact emergency services immediately!");
  Firebase.setString(firebaseData, notificationPath + "/timestamp", timeString);
  Firebase.setString(firebaseData, notificationPath + "/priority", "HIGH");
  Firebase.setBool(firebaseData, notificationPath + "/sent", true);
  
  // Set flag for web app to show emergency alert
  Firebase.setBool(firebaseData, "/smartBuilding/system/showEmergencyAlert", true);
  
  Serial.println("📢 Emergency notification sent to Firebase for web app");
}

void sendSystemInfo() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  Firebase.setString(firebaseData, "/smartBuilding/system/startupTime", timeString);
  Firebase.setString(firebaseData, "/smartBuilding/system/esp32Version", "1.0.0");
  Firebase.setString(firebaseData, "/smartBuilding/system/description", "Smart Building Monitoring System - 3 Room Configuration");
  Firebase.setInt(firebaseData, "/smartBuilding/system/totalRooms", 3);
  
  // Room configuration
  Firebase.setString(firebaseData, "/smartBuilding/config/room1", "Bedroom");
  Firebase.setString(firebaseData, "/smartBuilding/config/room2", "Kitchen");
  Firebase.setString(firebaseData, "/smartBuilding/config/room3", "Parking Lot");
  
  Serial.println("📋 System information sent to Firebase");
}