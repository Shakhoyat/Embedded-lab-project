/*
 * Smart Building Monitoring System - ESP32 Gateway
 * Receives JSON data from Arduino Mega (4 segments) via Serial2 and manages Firebase integration
 * 
 * Segments monitored:
 * - Kitchen: DHT11, MQ135, Flame Sensor, Buzzer, LEDs
 * - Bedroom: DS18B20, MQ2, Flame Sensor, Buzzer, LEDs  
 * - Parking: MQ2, Flame Sensor, LEDs
 * - Central Gas Chamber: MQ2, Buzzer
 * 
 * COMMUNICATION:
 * - Serial2 (pins 16/17): Bidirectional JSON communication with Arduino Mega
 * - Receives: DATA, INIT, HEARTBEAT, EMERGENCY_ALERT, EMERGENCY_CLEAR messages
 * - Sends: ACK, STATUS, FB_STATUS responses
 * 
 * Features:
 * - WiFi connectivity and Firebase integration
 * - LCD display with cyclic segment data visualization
 * - Emergency alert system with KUET fire brigade notification
 * - Admin and caretaker alerting system
 * - Real-time monitoring and data logging
 * - Bidirectional Arduino communication with acknowledgments
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <time.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// ========== WIFI CONFIGURATION ==========
const char* WIFI_SSID = "skt_pie";        // Replace with your WiFi name
const char* WIFI_PASSWORD = "12104053";   // Replace with your WiFi password

// ========== FIREBASE CONFIGURATION ==========
#define FIREBASE_HOST "smart-building-monitoring-iot-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "wMpzysAkHQ2DnAUe9uoT8Y7YrmV3B6WUe5VHSYIE"
#define API_KEY "AIzaSyDC9lCy5fUDA_zuUAnxVy0pZSqI3F5NuDM"

// ========== BUILDING & EMERGENCY CONTACT CONFIGURATION ==========
#define BUILDING_NAME "KUET Smart Apartment Complex"
#define BUILDING_ADDRESS "Fulbarigate, Khulna, KUET Area"
#define ADMIN_EMAIL "admin@kuetapartment.com"           // Building admin
#define ADMIN_PHONE "+8801XXXXXXXXX"                    // Admin contact
#define CARETAKER_EMAIL "caretaker@kuetapartment.com"   // Caretaker
#define CARETAKER_PHONE "+8801YYYYYYYYY"                // Caretaker contact
#define FIRE_BRIGADE_PHONE "01303488507"                // KUET Area Fire Station

// ========== PIN DEFINITIONS ==========
#define STATUS_LED 2      // Built-in LED for connection status
#define EMERGENCY_LED 4   // External LED for emergency indication
#define WIFI_LED 5        // LED for WiFi status

// Serial2 pins for Arduino Mega communication
#define RXD2 16
#define TXD2 17

// LCD Display (I2C) - Connected to ESP32
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ========== GLOBAL VARIABLES ==========
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

String receivedData = "";
unsigned long lastHeartbeat = 0;
unsigned long lastEmergencyCheck = 0;
unsigned long lastLCDUpdate = 0;
bool emergencyState = false;
int dataReceiveCount = 0;
bool firebaseConnected = false;
unsigned long lastNotificationId = 0;
int totalAlertsGenerated = 0;
int currentLCDSegment = 0; // For cycling through segments: 0=system, 1=kitchen, 2=bedroom, 3=parking, 4=central

// ========== SEGMENT DATA STRUCTURES ==========
struct SegmentInfo {
  String name;
  float temperature;
  float humidity;
  int gasLevel;
  int airQuality;
  bool flameDetected;
  bool isEmergency;
  bool isDangerous;
  String sensorTypes;
  String components;
  unsigned long lastUpdate;
};

SegmentInfo kitchen = {"Kitchen", 0, 0, 0, 0, false, false, false, "", "", 0};
SegmentInfo bedroom = {"Bedroom", 0, 0, 0, 0, false, false, false, "", "", 0};
SegmentInfo parking = {"Parking", 0, 0, 0, 0, false, false, false, "", "", 0};
SegmentInfo centralGas = {"Central_Gas", 0, 0, 0, 0, false, false, false, "", "", 0};

bool systemEmergency = false;
float globalTemperature = 0;
float globalHumidity = 0;
unsigned long emergencyDuration = 0;

// LCD cycling timing
const unsigned long LCD_UPDATE_INTERVAL = 3000; // 3 seconds per segment

// ========== TIME CONFIGURATION ==========
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;      // GMT+5:30 (Bangladesh time - adjust for your timezone)
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);  // Initialize Serial2 for Arduino Mega communication
  Serial.println("Smart Building ESP32 Gateway");
  Serial.println("4 Segments: Kitchen, Bedroom, Parking, Central Gas Chamber");
  Serial.println("Serial2 initialized for Arduino Mega communication");
  Serial.println("Initializing system...");
  
  // Initialize I2C for LCD
  Wire.begin();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Building");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 Gateway");
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait");
  
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
  
  // Update LCD with WiFi status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  lcd.print(WiFi.status() == WL_CONNECTED ? "Connected" : "Failed");
  lcd.setCursor(0, 1);
  lcd.print("Syncing time...");
  
  // Initialize time synchronization
  Serial.println("Synchronizing time with NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov", "time.google.com");
  
  // Wait for time synchronization (max 10 seconds)
  int retries = 0;
  time_t now = time(nullptr);
  while (now < 1000000000 && retries < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    retries++;
  }
  Serial.println();
  
  if (now > 1000000000) {
    Serial.println("Time synchronized successfully");
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    Serial.print("Current time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time synced OK");
    lcd.setCursor(0, 1);
    lcd.print("Init Firebase...");
  } else {
    Serial.println("Time sync failed - timestamps may be incorrect");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time sync failed");
    lcd.setCursor(0, 1);
    lcd.print("Init Firebase...");
  }
  
  // Initialize Firebase
  initializeFirebase();
  
  // Update LCD with Firebase status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Firebase: ");
  lcd.print(firebaseConnected ? "OK" : "Failed");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  
  // Send initial system status and contact info
  sendSystemInfo();
  sendContactConfiguration();
  
  // Initialize LCD display cycle
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("Arduino data...");
  
  Serial.println("ESP32 Gateway ready to receive data from Arduino Mega");
  Serial.println("LCD Display: Cycling through segment data every 3 seconds");
  Serial.println("Serial2 communication active on pins RX:" + String(RXD2) + ", TX:" + String(TXD2));
  Serial.println("Waiting for sensor data...");
  
  // Send ready signal to Arduino Mega
  Serial2.println("ESP32_READY");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_LED, LOW);
    Serial.println("WiFi disconnected - attempting reconnection...");
    reconnectWiFi();
  } else {
    digitalWrite(WIFI_LED, HIGH);
  }
  
  // Update LCD display cycling through segments
  if (currentTime - lastLCDUpdate >= LCD_UPDATE_INTERVAL) {
    lastLCDUpdate = currentTime;
    updateLCDDisplay();
    currentLCDSegment = (currentLCDSegment + 1) % 5; // 0: system, 1: kitchen, 2: bedroom, 3: parking, 4: central
  }
  
  // Read data from Arduino Mega via Serial2
  if (Serial2.available()) {
    String rawData = Serial2.readStringUntil('\n');
    rawData.trim();
    
    if (rawData.length() > 0) {
      Serial.println("Raw data from Arduino: " + rawData);
      
      if (rawData.startsWith("DATA:")) {
        Serial.println("JSON data received from Arduino Mega");
        receivedData = rawData.substring(5); // Remove "DATA:" prefix
        handleDataMessage(receivedData);
        processArduinoData();
        dataReceiveCount++;
        sendAckToArduino("DATA"); // Send acknowledgment
        Serial.println("Total data packets received: " + String(dataReceiveCount));
      } 
      else if (rawData.startsWith("INIT:")) {
        Serial.println("Arduino initialization message received");
        handleInitMessage(rawData.substring(5));
        sendAckToArduino("INIT");
        notifyArduinoFirebaseStatus(); // Send Firebase status
      }
      else if (rawData.startsWith("HEARTBEAT:")) {
        Serial.println("Arduino heartbeat received");
        handleHeartbeatMessage(rawData.substring(10));
        sendAckToArduino("HEARTBEAT");
      }
      else if (rawData.startsWith("EMERGENCY_ALERT:")) {
        Serial.println("Emergency alert from Arduino: " + rawData.substring(16));
        handleEmergencyMessage(rawData.substring(16));
      }
      else if (rawData.startsWith("EMERGENCY_CLEAR:")) {
        Serial.println("Emergency cleared from Arduino: " + rawData.substring(16));
        handleEmergencyClear(rawData.substring(16));
      }
      else {
        Serial.println("Arduino Debug: " + rawData);
      }
    }
  }
  
  // Send heartbeat every 30 seconds
  if (currentTime - lastHeartbeat >= 30000) {
    Serial.println("Sending heartbeat to Firebase...");
    sendHeartbeat();
    sendStatusToArduino(); // Also send status to Arduino
    lastHeartbeat = currentTime;
  }
  
  // Check for emergency conditions every 5 seconds
  if (currentTime - lastEmergencyCheck >= 5000) {
    handleEmergencyAlerts();
    lastEmergencyCheck = currentTime;
  }
  
  // Blink status LED to show system is running
  digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
  
  // Print system status every 60 seconds
  static unsigned long lastStatusPrint = 0;
  if (currentTime - lastStatusPrint >= 60000) {
    Serial.println("System Status Summary:");
    Serial.println("WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
    Serial.println("Signal: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("Firebase: " + String(firebaseConnected ? "Connected" : "Disconnected"));
    Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
    Serial.println("Data packets: " + String(dataReceiveCount));
    Serial.println("Emergency state: " + String(emergencyState ? "ACTIVE" : "Normal"));
    Serial.println("Status summary complete");
    lastStatusPrint = currentTime;
  }
  
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
    Serial.println("WiFi connected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
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
    Serial.println("WiFi reconnected");
    digitalWrite(WIFI_LED, HIGH);
  }
}

void initializeFirebase() {
  Serial.println("Initializing Firebase connection...");
  Serial.println("Database URL: https://" + String(FIREBASE_HOST));
  
  // Configure Firebase using config objects
  Serial.println("Configuring Firebase with config objects...");
  
  // Set database URL and API key
  config.database_url = "https://" + String(FIREBASE_HOST);
  config.api_key = API_KEY;
  
  // Use database secret for authentication (legacy token)
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  // Initialize Firebase with config objects
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Testing Firebase connection...");
  delay(3000); // Give time for connection
  
  // Test connection by trying to set a simple value
  if (Firebase.setString(firebaseData, "/test/connection", "testing")) {
    firebaseConnected = true;
    Serial.println("Firebase connection test successful");
    
    // Set timeouts
    Firebase.setReadTimeout(firebaseData, 30000);
    Firebase.setwriteSizeLimit(firebaseData, "tiny");
    
    Serial.println("Firebase initialized successfully");
    Serial.println("Ready to send/receive data to/from Firebase");
  } else {
    firebaseConnected = false;
    Serial.println("Firebase connection test failed");
    Serial.println("Error reason: " + firebaseData.errorReason());
    Serial.println("System will continue running but data won't be sent to Firebase");
    
    // Print troubleshooting info
    Serial.println("TROUBLESHOOTING TIPS:");
    Serial.println("1. Check your Firebase database URL");
    Serial.println("2. Verify your database secret key");
    Serial.println("3. Ensure Firebase Realtime Database is enabled");
    Serial.println("4. Check database rules (should allow read/write)");
    Serial.println("5. Try using Web API Key instead of database secret");
  }
}

// Handle incoming JSON data message from Arduino Mega
void handleDataMessage(String jsonStr) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);
  
  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract and print key variables for debugging
  bool emergency = doc["systemEmergency"];
  float gTemp = doc["environment"]["globalTemperature"];
  float gHum = doc["environment"]["globalHumidity"];

  float kitchenTemp = doc["kitchen"]["temperature"];
  bool kitchenFlame = doc["kitchen"]["flameDetected"];

  float bedroomTemp = doc["bedroom"]["temperature"];
  int bedroomGas = doc["bedroom"]["gasLevel"];

  int parkingGas = doc["parking"]["gasLevel"];
  bool parkingFlame = doc["parking"]["flameDetected"];

  int centralGas = doc["centralGas"]["gasLevel"];

  // Print variable values for monitoring
  Serial.println("=== Arduino Data Variables ===");
  Serial.println("emergency: " + String(emergency));
  Serial.println("globalTemp: " + String(gTemp));
  Serial.println("globalHumidity: " + String(gHum));
  Serial.println("kitchenTemp: " + String(kitchenTemp));
  Serial.println("kitchenFlame: " + String(kitchenFlame));
  Serial.println("bedroomTemp: " + String(bedroomTemp));
  Serial.println("bedroomGas: " + String(bedroomGas));
  Serial.println("parkingGas: " + String(parkingGas));
  Serial.println("parkingFlame: " + String(parkingFlame));
  Serial.println("centralGas: " + String(centralGas));
  Serial.println("===============================");
}

// Handle Arduino initialization message
void handleInitMessage(String jsonStr) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);
  
  if (!error) {
    String deviceID = doc["deviceID"];
    String status = doc["status"];
    int segments = doc["segments"];
    
    Serial.println("Arduino Mega initialized: " + deviceID);
    Serial.println("Status: " + status + ", Segments: " + String(segments));
    
    // Send acknowledgment to Firebase
    if (firebaseConnected) {
      Firebase.setString(firebaseData, "/smartBuilding/system/arduinoStatus", status);
      Firebase.setString(firebaseData, "/smartBuilding/system/arduinoDevice", deviceID);
      Firebase.setInt(firebaseData, "/smartBuilding/system/segmentCount", segments);
    }
  }
}

// Handle Arduino heartbeat message
void handleHeartbeatMessage(String jsonStr) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonStr);
  
  if (!error) {
    String deviceID = doc["deviceID"];
    unsigned long uptime = doc["uptime"];
    bool sysEmergency = doc["systemEmergency"];
    int dataCount = doc["dataSendCount"];
    
    Serial.println("Arduino heartbeat - Uptime: " + String(uptime) + "s, Data count: " + String(dataCount));
    
    // Update Firebase with Arduino heartbeat info
    if (firebaseConnected) {
      Firebase.setInt(firebaseData, "/smartBuilding/system/arduinoUptime", uptime);
      Firebase.setInt(firebaseData, "/smartBuilding/system/arduinoDataCount", dataCount);
      Firebase.setBool(firebaseData, "/smartBuilding/system/arduinoEmergency", sysEmergency);
      Firebase.setString(firebaseData, "/smartBuilding/system/lastArduinoHeartbeat", getFormattedTime());
    }
  }
}

// Handle emergency alert from Arduino
void handleEmergencyMessage(String message) {
  Serial.println("EMERGENCY ALERT: " + message);
  
  // Create immediate emergency alert in Firebase
  if (firebaseConnected) {
    unsigned long alertId = millis();
    String alertPath = "/smartBuilding/alerts/" + String(alertId);
    
    Firebase.setString(firebaseData, alertPath + "/type", "ARDUINO_EMERGENCY");
    Firebase.setString(firebaseData, alertPath + "/message", message);
    Firebase.setString(firebaseData, alertPath + "/timestamp", getFormattedTime());
    Firebase.setBool(firebaseData, alertPath + "/acknowledged", false);
    Firebase.setString(firebaseData, alertPath + "/source", "Arduino_Mega");
  }
  
  // Activate emergency LED
  digitalWrite(EMERGENCY_LED, HIGH);
  emergencyState = true;
}

// Handle emergency clear from Arduino
void handleEmergencyClear(String message) {
  Serial.println("EMERGENCY CLEARED: " + message);
  
  // Send all-clear to Firebase
  if (firebaseConnected) {
    unsigned long clearId = millis();
    String clearPath = "/smartBuilding/alerts/" + String(clearId);
    
    Firebase.setString(firebaseData, clearPath + "/type", "ALL_CLEAR");
    Firebase.setString(firebaseData, clearPath + "/message", "Arduino reports: " + message);
    Firebase.setString(firebaseData, clearPath + "/timestamp", getFormattedTime());
    Firebase.setString(firebaseData, clearPath + "/source", "Arduino_Mega");
  }
  
  // Turn off emergency LED
  digitalWrite(EMERGENCY_LED, LOW);
  emergencyState = false;
}

void processArduinoData() {
  Serial.println("Processing data from Arduino Mega (4 segments)...");
  
  // Parse JSON data
  DynamicJsonDocument doc(1536); // Increased size for 4 segments
  DeserializationError error = deserializeJson(doc, receivedData);
  
  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    Serial.println("Raw data received: " + receivedData);
    return;
  }
  
  // Extract system-wide data
  systemEmergency = doc["systemEmergency"];
  emergencyDuration = doc["emergencyDuration"];
  unsigned long timestamp = doc["timestamp"];
  
  // Extract global environmental data
  JsonObject env = doc["environment"];
  globalTemperature = env["globalTemperature"];
  globalHumidity = env["globalHumidity"];
  
  Serial.println("System Status:");
  Serial.println("  System Emergency: " + String(systemEmergency ? "YES" : "NO"));
  Serial.println("  Global Temperature: " + String(globalTemperature) + "°C");
  Serial.println("  Global Humidity: " + String(globalHumidity) + "%");
  
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  // Process segment data and update local structures
  processSegmentData("kitchen", doc["kitchen"], kitchen);
  processSegmentData("bedroom", doc["bedroom"], bedroom);
  processSegmentData("parking", doc["parking"], parking);
  processSegmentData("centralGas", doc["centralGas"], centralGas);
  
  // Send consolidated data to Firebase
  if (firebaseConnected) {
    Serial.println("Sending data to Firebase...");
    
    // System-wide status
    String systemPath = "/smartBuilding/system";
    Firebase.setBool(firebaseData, systemPath + "/systemEmergency", systemEmergency);
    Firebase.setFloat(firebaseData, systemPath + "/globalTemperature", globalTemperature);
    Firebase.setFloat(firebaseData, systemPath + "/globalHumidity", globalHumidity);
    Firebase.setInt(firebaseData, systemPath + "/emergencyDuration", emergencyDuration);
    Firebase.setString(firebaseData, systemPath + "/lastUpdated", timeString);
    Firebase.setInt(firebaseData, systemPath + "/dataReceiveCount", dataReceiveCount);
    Firebase.setString(firebaseData, systemPath + "/esp32Status", "online");
    
    Serial.println("System data sent to Firebase");
  } else {
    Serial.println("Firebase not connected - skipping data upload");
  }
  
  Serial.println("Data processing completed");
  Serial.println("Segments processed: Kitchen, Bedroom, Parking, Central Gas");
}

// Process individual segment data
void processSegmentData(String segmentName, JsonObject segmentData, SegmentInfo &segment) {
  // Update segment information
  segment.temperature = segmentData["temperature"];
  segment.humidity = segmentData["humidity"];
  segment.gasLevel = segmentData["gasLevel"];
  segment.airQuality = segmentData["airQuality"];
  segment.flameDetected = segmentData["flameDetected"];
  segment.isEmergency = segmentData["isEmergency"];
  segment.isDangerous = segmentData["isDangerous"];
  segment.sensorTypes = segmentData["sensorTypes"].as<String>();
  segment.components = segmentData["hasComponents"].as<String>();
  segment.lastUpdate = millis();
  
  // Get current time
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  // Determine segment status
  String status;
  if (segment.isEmergency) {
    status = "EMERGENCY";
  } else if (segment.isDangerous) {
    status = "WARNING";
  } else {
    status = "SAFE";
  }
  
  // Send segment data to Firebase
  if (firebaseConnected) {
    String segmentPath = "/smartBuilding/segments/" + segmentName;
    
    Firebase.setFloat(firebaseData, segmentPath + "/temperature", segment.temperature);
    Firebase.setFloat(firebaseData, segmentPath + "/humidity", segment.humidity);
    Firebase.setInt(firebaseData, segmentPath + "/gasLevel", segment.gasLevel);
    Firebase.setInt(firebaseData, segmentPath + "/airQuality", segment.airQuality);
    Firebase.setBool(firebaseData, segmentPath + "/flameDetected", segment.flameDetected);
    Firebase.setBool(firebaseData, segmentPath + "/isEmergency", segment.isEmergency);
    Firebase.setBool(firebaseData, segmentPath + "/isDangerous", segment.isDangerous);
    Firebase.setString(firebaseData, segmentPath + "/status", status);
    Firebase.setString(firebaseData, segmentPath + "/sensorTypes", segment.sensorTypes);
    Firebase.setString(firebaseData, segmentPath + "/components", segment.components);
    Firebase.setString(firebaseData, segmentPath + "/lastUpdated", timeString);
  }
  
  // Handle emergency alerts
  if (segment.isEmergency) {
    handleEmergencyAlert(segmentName, segment, timeString);
  } else if (segment.isDangerous) {
    handleWarningAlert(segmentName, segment, timeString);
  }
  
  // Log segment status
  Serial.println(segmentName + ": " + status);
  Serial.println("  Sensors: " + segment.sensorTypes);
  if (segment.temperature > 0) Serial.println("  Temperature: " + String(segment.temperature, 1) + "°C");
  if (segment.humidity > 0) Serial.println("  Humidity: " + String(segment.humidity, 1) + "%");
  if (segment.gasLevel > 0) Serial.println("  Gas Level: " + String(segment.gasLevel));
  if (segment.airQuality > 0) Serial.println("  Air Quality: " + String(segment.airQuality));
  if (segment.flameDetected) Serial.println("  FLAME DETECTED");
}

// LCD Display Management
void updateLCDDisplay() {
  lcd.clear();
  
  switch (currentLCDSegment) {
    case 0: // System Overview
      lcd.setCursor(0, 0);
      lcd.print("System:");
      lcd.print(systemEmergency ? "EMRG" : "OK");
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(globalTemperature, 1);
      lcd.print(" H:");
      lcd.print(globalHumidity, 0);
      lcd.print("%");
      break;
      
    case 1: // Kitchen
      lcd.setCursor(0, 0);
      lcd.print("Kitchen:");
      if (kitchen.flameDetected) lcd.print("FIRE");
      else if (kitchen.isEmergency) lcd.print("EMRG");
      else if (kitchen.isDangerous) lcd.print("WARN");
      else lcd.print("SAFE");
      
      lcd.setCursor(0, 1);
      if (kitchen.temperature > 0) {
        lcd.print("T:");
        lcd.print(kitchen.temperature, 1);
        lcd.print(" AQ:");
        lcd.print(kitchen.airQuality);
      } else {
        lcd.print("No data");
      }
      break;
      
    case 2: // Bedroom
      lcd.setCursor(0, 0);
      lcd.print("Bedroom:");
      if (bedroom.flameDetected) lcd.print("FIRE");
      else if (bedroom.isEmergency) lcd.print("EMRG");
      else if (bedroom.isDangerous) lcd.print("WARN");
      else lcd.print("SAFE");
      
      lcd.setCursor(0, 1);
      if (bedroom.temperature > 0) {
        lcd.print("T:");
        lcd.print(bedroom.temperature, 1);
        lcd.print(" G:");
        lcd.print(bedroom.gasLevel);
      } else {
        lcd.print("No data");
      }
      break;
      
    case 3: // Parking
      lcd.setCursor(0, 0);
      lcd.print("Parking:");
      if (parking.flameDetected) lcd.print("FIRE");
      else if (parking.isEmergency) lcd.print("EMRG");
      else if (parking.isDangerous) lcd.print("WARN");
      else lcd.print("SAFE");
      
      lcd.setCursor(0, 1);
      lcd.print("Gas:");
      lcd.print(parking.gasLevel);
      break;
      
    case 4: // Central Gas Chamber
      lcd.setCursor(0, 0);
      lcd.print("Central Gas:");
      if (centralGas.isEmergency) lcd.print("EMRG");
      else if (centralGas.isDangerous) lcd.print("WARN");
      else lcd.print("OK");
      
      lcd.setCursor(0, 1);
      lcd.print("Level:");
      lcd.print(centralGas.gasLevel);
      break;
  }
  
  // Show emergency indicator on all screens if system emergency
  if (systemEmergency) {
    lcd.setCursor(15, 0);
    lcd.print("!");
  }
}

// Emergency Alert Handling
void handleEmergencyAlert(String segmentName, SegmentInfo &segment, String timeString) {
  // Create unique alert ID
  unsigned long alertId = millis();
  String alertPath = "/smartBuilding/alerts/" + String(alertId);
  
  Serial.println("EMERGENCY in " + segmentName + "! Creating alert...");
  
  // Determine cause of emergency
  String cause = "Multiple critical readings";
  if (segment.flameDetected) {
    cause = "Flame detected";
  } else if (segment.temperature > 80) {
    cause = "Critical temperature";
  } else if (segment.gasLevel > 800) {
    cause = "Critical gas level";
  }
  
  // Create notification content
  String title = "FIRE EMERGENCY!";
  String body = "Emergency in " + segmentName + " at " + BUILDING_NAME + ". Cause: " + cause + ".";
  
  // Send data to Firebase
  Firebase.setString(firebaseData, alertPath + "/segment", segmentName);
  Firebase.setString(firebaseData, alertPath + "/type", "EMERGENCY");
  Firebase.setString(firebaseData, alertPath + "/cause", cause);
  Firebase.setString(firebaseData, alertPath + "/timestamp", timeString);
  Firebase.setBool(firebaseData, alertPath + "/acknowledged", false);
  
  // Send notification via Firebase
  Firebase.setString(firebaseData, alertPath + "/notification/title", title);
  Firebase.setString(firebaseData, alertPath + "/notification/body", body);
  
  Serial.println("Emergency alert created for " + segmentName);
  
  // Trigger special notification for KUET Fire Brigade
  createKUETFireBrigadeNotification(segmentName, cause, timeString);
}

// Warning Alert Handling
void handleWarningAlert(String segmentName, SegmentInfo &segment, String timeString) {
  unsigned long alertId = millis();
  String alertPath = "/smartBuilding/warnings/" + String(alertId);
  
  Serial.println("WARNING in " + segmentName + ". Creating alert...");
  
  // Determine cause of warning
  String cause = "High readings detected";
  if (segment.temperature > 50) {
    cause = "High temperature";
  } else if (segment.gasLevel > 400) {
    cause = "High gas level";
  }
  
  // Create notification content
  String title = "Warning: " + segmentName;
  String body = "Potential danger detected in " + segmentName + " at " + BUILDING_NAME + ". Cause: " + cause + ".";
  
  // Send data to Firebase
  Firebase.setString(firebaseData, alertPath + "/segment", segmentName);
  Firebase.setString(firebaseData, alertPath + "/type", "WARNING");
  Firebase.setString(firebaseData, alertPath + "/cause", cause);
  Firebase.setString(firebaseData, alertPath + "/timestamp", timeString);
  
  // Send notification
  Firebase.setString(firebaseData, alertPath + "/notification/title", title);
  Firebase.setString(firebaseData, alertPath + "/notification/body", body);
  
  Serial.println("Warning alert sent for " + segmentName);
}

// Special notification for KUET Fire Brigade
void createKUETFireBrigadeNotification(String segmentName, String cause, String timeString) {
  unsigned long notifyId = millis();
  String notifyPath = "/smartBuilding/emergency_notifications/" + String(notifyId);
  
  Serial.println("Creating special notification for KUET Fire Brigade...");
  
  String title = "FIRE EMERGENCY!";
  String subject = "FIRE EMERGENCY - " + String(BUILDING_NAME);
  String body = "This is an automated fire alert.\n\n"
                "Building: " + String(BUILDING_NAME) + "\n"
                "Address: " + String(BUILDING_ADDRESS) + "\n"
                "Location of Fire: " + segmentName + "\n"
                "Cause: " + cause + "\n"
                "Time: " + timeString + "\n\n"
                "Please dispatch emergency services immediately.\n"
                "Admin Contact: " + String(ADMIN_PHONE) + "\n"
                "Caretaker Contact: " + String(CARETAKER_PHONE);

  // Send notification data to Firebase (for email/SMS trigger)
  Firebase.setString(firebaseData, notifyPath + "/recipient", "KUET Fire Brigade");
  Firebase.setString(firebaseData, notifyPath + "/timestamp", timeString);
  Firebase.setString(firebaseData, notifyPath + "/details/building", BUILDING_NAME);
  Firebase.setString(firebaseData, notifyPath + "/details/address", BUILDING_ADDRESS);
  Firebase.setString(firebaseData, notifyPath + "/details/location", segmentName);
  Firebase.setString(firebaseData, notifyPath + "/details/cause", cause);
  
  // Email action
  Firebase.setString(firebaseData, notifyPath + "/actions/email/to", "kuetfire@example.com");
  Firebase.setString(firebaseData, notifyPath + "/actions/email/subject", subject);
  Firebase.setString(firebaseData, notifyPath + "/actions/email/body", body);
  
  // SMS action
  Firebase.setString(firebaseData, notifyPath + "/actions/sms/to", FIRE_BRIGADE_PHONE);
  Firebase.setString(firebaseData, notifyPath + "/actions/sms/message", title + " at " + BUILDING_NAME + ". Location: " + segmentName + ". Cause: " + cause + ". Dispatch immediately.");
  
  Serial.println("KUET Fire Brigade notification prepared and sent to Firebase queue");
}

// Send system information to Firebase
void sendSystemInfo() {
  Serial.println("Sending system info to Firebase...");
  String path = "/smartBuilding/system/info";
  
  Firebase.setString(firebaseData, path + "/buildingName", BUILDING_NAME);
  Firebase.setString(firebaseData, path + "/address", BUILDING_ADDRESS);
  Firebase.setString(firebaseData, path + "/esp32IP", WiFi.localIP().toString());
  Firebase.setString(firebaseData, path + "/wifiSSID", WIFI_SSID);
  
  Serial.println("System info sent successfully");
}

// Send contact configuration to Firebase
void sendContactConfiguration() {
  Serial.println("Sending contact configuration to Firebase...");
  String path = "/smartBuilding/config/contacts";
  
  Firebase.setString(firebaseData, path + "/admin/email", ADMIN_EMAIL);
  Firebase.setString(firebaseData, path + "/admin/phone", ADMIN_PHONE);
  Firebase.setString(firebaseData, path + "/caretaker/email", CARETAKER_EMAIL);
  Firebase.setString(firebaseData, path + "/caretaker/phone", CARETAKER_PHONE);
  Firebase.setString(firebaseData, path + "/emergency_services/fire_brigade_phone", FIRE_BRIGADE_PHONE);
  
  Serial.println("Contact configuration sent successfully");
}

void sendHeartbeat() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  if (firebaseConnected) {
    if (Firebase.setString(firebaseData, "/smartBuilding/system/esp32Heartbeat", timeString)) {
      Firebase.setInt(firebaseData, "/smartBuilding/system/uptime", millis() / 1000);
      Firebase.setInt(firebaseData, "/smartBuilding/system/wifiSignalStrength", WiFi.RSSI());
      Firebase.setString(firebaseData, "/smartBuilding/system/esp32IP", WiFi.localIP().toString());
      Serial.println("Heartbeat sent - System uptime: " + String(millis() / 1000) + " seconds");
    } else {
      Serial.println("Failed to send heartbeat to Firebase");
      firebaseConnected = false; // Mark as disconnected
    }
  } else {
    Serial.println("Heartbeat skipped - Firebase not connected (Uptime: " + String(millis() / 1000) + "s)");
    // Try simple reconnection every 5 heartbeats (2.5 minutes)
    static int skipCount = 0;
    skipCount++;
    if (skipCount >= 5) {
      Serial.println("Attempting simplified Firebase reconnection...");
      // Use config objects for reconnection too
      config.database_url = "https://" + String(FIREBASE_HOST);
      config.api_key = API_KEY;
      config.signer.tokens.legacy_token = FIREBASE_AUTH;
      
      Firebase.begin(&config, &auth);
      delay(2000);
      if (Firebase.setString(firebaseData, "/test/reconnect", "testing")) {
        firebaseConnected = true;
        Serial.println("Firebase reconnected successfully");
      } else {
        Serial.println("Firebase reconnection failed");
      }
      skipCount = 0;
    }
  }
}

// Handle overall emergency state
void handleEmergencyAlerts() {
  bool currentEmergency = systemEmergency || kitchen.isEmergency || bedroom.isEmergency || parking.isEmergency || centralGas.isEmergency;
  
  if (currentEmergency) {
    if (!emergencyState) {
      // New emergency detected
      emergencyState = true;
      digitalWrite(EMERGENCY_LED, HIGH);
      Serial.println("System-wide emergency state ACTIVATED");
    }
    // Emergency is ongoing
    digitalWrite(EMERGENCY_LED, !digitalRead(EMERGENCY_LED)); // Blink LED
  } else {
    if (emergencyState) {
      // Emergency has ended
      emergencyState = false;
      digitalWrite(EMERGENCY_LED, LOW);
      Serial.println("System-wide emergency state DEACTIVATED. All clear.");
      
      // Send "all clear" notification
      unsigned long clearId = millis();
      String clearPath = "/smartBuilding/alerts/" + String(clearId);
      Firebase.setString(firebaseData, clearPath + "/type", "ALL_CLEAR");
      Firebase.setString(firebaseData, clearPath + "/message", "Emergency condition resolved. System is now safe.");
      
      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);
      char timeString[64];
      strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Firebase.setString(firebaseData, clearPath + "/timestamp", timeString);
    }
  }
}

// ========== HELPER FUNCTIONS ==========

// Get formatted timestamp
String getFormattedTime() {
  time_t now = time(nullptr);
  
  if (now < 1000000000) {
    return "Time Not Set";
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time Error";
  }
  
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Capitalize first letter of string
String capitalizeFirst(String str) {
  if (str.length() == 0) return str;
  String result = str;
  result[0] = toupper(result[0]);
  return result;
}

// Send status update to Arduino Mega via Serial2
void sendStatusToArduino() {
  StaticJsonDocument<256> statusDoc;
  statusDoc["messageType"] = "ESP32_STATUS";
  statusDoc["timestamp"] = millis();
  statusDoc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
  statusDoc["firebaseConnected"] = firebaseConnected;
  statusDoc["emergencyState"] = emergencyState;
  statusDoc["uptime"] = millis() / 1000;
  
  Serial2.print("STATUS:");
  serializeJson(statusDoc, Serial2);
  Serial2.println();
  
  Serial.println("Status sent to Arduino Mega");
}

// Send acknowledgment to Arduino for received data
void sendAckToArduino(String messageType) {
  StaticJsonDocument<128> ackDoc;
  ackDoc["messageType"] = "ACK";
  ackDoc["received"] = messageType;
  ackDoc["timestamp"] = millis();
  
  Serial2.print("ACK:");
  serializeJson(ackDoc, Serial2);
  Serial2.println();
}

// Send Firebase connection status to Arduino
void notifyArduinoFirebaseStatus() {
  StaticJsonDocument<128> fbDoc;
  fbDoc["messageType"] = "FIREBASE_STATUS";
  fbDoc["connected"] = firebaseConnected;
  fbDoc["timestamp"] = millis();
  
  Serial2.print("FB_STATUS:");
  serializeJson(fbDoc, Serial2);
  Serial2.println();
}
