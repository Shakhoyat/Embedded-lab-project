/*
 * Smart Building Monitoring System - ESP32 Gateway
 * Receives data from Arduino Mega (4 segments) and manages Firebase integration
 * 
 * Segments monitored:
 * - Kitchen: DHT11, MQ135, Flame Sensor, Buzzer, LEDs
 * - Bedroom: DS18B20, MQ2, Flame Sensor, Buzzer, LEDs  
 * - Parking: MQ2, Flame Sensor, LEDs
 * - Central Gas Chamber: MQ2, Buzzer
 * 
 * Features:
 * - WiFi connectivity
 * - Firebase real-time database integration
 * - JSON data parsing from Arduino Mega
 * - LCD display showing sensor data in cyclic order
 * - Emergency alert system with KUET fire brigade notification
 * - Admin/caretaker alerting system
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
  Serial.println("\n=== SMART BUILDING ESP32 GATEWAY ===");
  Serial.println("4 Segments: Kitchen, Bedroom, Parking, Central Gas Chamber");
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
  Serial.println("🕐 Synchronizing time with NTP server...");
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
    Serial.println("✅ Time synchronized successfully!");
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    Serial.print("   Current time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time synced OK");
    lcd.setCursor(0, 1);
    lcd.print("Init Firebase...");
  } else {
    Serial.println("⚠️ Time sync failed - timestamps may be incorrect");
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
  Serial.println("Waiting for sensor data...");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_LED, LOW);
    Serial.println("⚠️ WiFi disconnected! Attempting reconnection...");
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
  
  // Read data from Arduino Mega
  if (Serial.available()) {
    String incomingData = Serial.readString();
    incomingData.trim();
    
    if (incomingData.startsWith("DATA:")) {
      Serial.println("📨 Received data from Arduino Mega");
      receivedData = incomingData.substring(5); // Remove "DATA:" prefix
      processArduinoData();
      dataReceiveCount++;
      Serial.println("📊 Total data packets received: " + String(dataReceiveCount));
    } else if (incomingData.length() > 0) {
      Serial.println("📝 Arduino Debug: " + incomingData);
    }
  }
  
  // Send heartbeat every 30 seconds
  if (currentTime - lastHeartbeat >= 30000) {
    Serial.println("💓 Sending heartbeat to Firebase...");
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
  
  // Print system status every 60 seconds
  static unsigned long lastStatusPrint = 0;
  if (currentTime - lastStatusPrint >= 60000) {
    Serial.println("\n=== SYSTEM STATUS SUMMARY ===");
    Serial.println("🔗 WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
    Serial.println("📡 Signal: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("🗄️ Firebase: " + String(firebaseConnected ? "Connected" : "Disconnected"));
    Serial.println("⏱️ Uptime: " + String(millis() / 1000) + " seconds");
    Serial.println("📦 Data packets: " + String(dataReceiveCount));
    Serial.println("🔥 Emergency state: " + String(emergencyState ? "ACTIVE" : "Normal"));
    Serial.println("=============================\n");
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
  Serial.println("🔧 Database URL: https://" + String(FIREBASE_HOST));
  
  // Configure Firebase using config objects
  Serial.println("🔄 Configuring Firebase with config objects...");
  
  // Set database URL and API key
  config.database_url = "https://" + String(FIREBASE_HOST);
  config.api_key = API_KEY;
  
  // Use database secret for authentication (legacy token)
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  // Initialize Firebase with config objects
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("⏳ Testing Firebase connection...");
  delay(3000); // Give time for connection
  
  // Test connection by trying to set a simple value
  if (Firebase.setString(firebaseData, "/test/connection", "testing")) {
    firebaseConnected = true;
    Serial.println("✅ Firebase connection test successful!");
    
    // Set timeouts
    Firebase.setReadTimeout(firebaseData, 30000);
    Firebase.setwriteSizeLimit(firebaseData, "tiny");
    
    Serial.println("✅ Firebase initialized successfully!");
    Serial.println("🗄️ Ready to send/receive data to/from Firebase");
  } else {
    firebaseConnected = false;
    Serial.println("❌ Firebase connection test failed");
    Serial.println("🔍 Error reason: " + firebaseData.errorReason());
    Serial.println("⚠️ System will continue running but data won't be sent to Firebase");
    
    // Print troubleshooting info
    Serial.println("\n🛠️ TROUBLESHOOTING TIPS:");
    Serial.println("1. Check your Firebase database URL");
    Serial.println("2. Verify your database secret key");
    Serial.println("3. Ensure Firebase Realtime Database is enabled");
    Serial.println("4. Check database rules (should allow read/write)");
    Serial.println("5. Try using Web API Key instead of database secret\n");
  }
}

void processArduinoData() {
  Serial.println("📊 Processing data from Arduino Mega (4 segments)...");
  
  // Parse JSON data
  DynamicJsonDocument doc(1536); // Increased size for 4 segments
  DeserializationError error = deserializeJson(doc, receivedData);
  
  if (error) {
    Serial.print("❌ JSON parsing error: ");
    Serial.println(error.c_str());
    Serial.println("📝 Raw data received: " + receivedData);
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
  
  Serial.println("🌍 System Status:");
  Serial.println("   System Emergency: " + String(systemEmergency ? "YES" : "NO"));
  Serial.println("   Global Temperature: " + String(globalTemperature) + "°C");
  Serial.println("   Global Humidity: " + String(globalHumidity) + "%");
  
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
    Serial.println("🔄 Sending data to Firebase...");
    
    // System-wide status
    String systemPath = "/smartBuilding/system";
    Firebase.setBool(firebaseData, systemPath + "/systemEmergency", systemEmergency);
    Firebase.setFloat(firebaseData, systemPath + "/globalTemperature", globalTemperature);
    Firebase.setFloat(firebaseData, systemPath + "/globalHumidity", globalHumidity);
    Firebase.setInt(firebaseData, systemPath + "/emergencyDuration", emergencyDuration);
    Firebase.setString(firebaseData, systemPath + "/lastUpdated", timeString);
    Firebase.setInt(firebaseData, systemPath + "/dataReceiveCount", dataReceiveCount);
    Firebase.setString(firebaseData, systemPath + "/esp32Status", "online");
    
    Serial.println("✅ System data sent to Firebase");
  } else {
    Serial.println("⚠️ Firebase not connected - skipping data upload");
  }
  
  Serial.println("✅ Data processing completed");
  Serial.println("📊 Segments processed: Kitchen, Bedroom, Parking, Central Gas");
  Serial.println("---");
}

// ====== Process individual segment data ======
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
  Serial.println("🏢 " + segmentName + ": " + status);
  Serial.println("   Sensors: " + segment.sensorTypes);
  if (segment.temperature > 0) Serial.println("   Temperature: " + String(segment.temperature, 1) + "°C");
  if (segment.humidity > 0) Serial.println("   Humidity: " + String(segment.humidity, 1) + "%");
  if (segment.gasLevel > 0) Serial.println("   Gas Level: " + String(segment.gasLevel));
  if (segment.airQuality > 0) Serial.println("   Air Quality: " + String(segment.airQuality));
  if (segment.flameDetected) Serial.println("   🔥 FLAME DETECTED!");
}

// ====== LCD Display Management ======
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

// ====== Emergency Alert Handling ======
void handleEmergencyAlert(String segmentName, SegmentInfo &segment, String timeString) {
  // Create unique alert ID
  unsigned long alertId = millis();
  String alertPath = "/smartBuilding/alerts/" + String(alertId);
  
  // Determine alert type
  String alertType = segment.flameDetected ? "FIRE_EMERGENCY" : "CRITICAL_CONDITIONS";
  String severity = "CRITICAL";
  
  // Store comprehensive emergency data
  Firebase.setString(firebaseData, alertPath + "/type", alertType);
  Firebase.setString(firebaseData, alertPath + "/severity", severity);
  Firebase.setString(firebaseData, alertPath + "/segment", segmentName);
  Firebase.setString(firebaseData, alertPath + "/segmentDisplayName", capitalizeFirst(segmentName));
  Firebase.setString(firebaseData, alertPath + "/timestamp", timeString);
  Firebase.setInt(firebaseData, alertPath + "/timestampUnix", time(nullptr));
  
  String message;
  if (segment.flameDetected) {
    message = "🔥 FIRE DETECTED in " + capitalizeFirst(segmentName) + "! Immediate evacuation required!";
  } else {
    message = "🚨 CRITICAL CONDITIONS in " + capitalizeFirst(segmentName) + "! Immediate attention required!";
  }
  Firebase.setString(firebaseData, alertPath + "/message", message);
  Firebase.setBool(firebaseData, alertPath + "/acknowledged", false);
  
  // Store detailed sensor data
  Firebase.setBool(firebaseData, alertPath + "/details/flameDetected", segment.flameDetected);
  Firebase.setInt(firebaseData, alertPath + "/details/gasLevel", segment.gasLevel);
  Firebase.setFloat(firebaseData, alertPath + "/details/temperature", segment.temperature);
  Firebase.setFloat(firebaseData, alertPath + "/details/humidity", segment.humidity);
  Firebase.setInt(firebaseData, alertPath + "/details/airQuality", segment.airQuality);
  
  // Store building information for KUET fire brigade
  Firebase.setString(firebaseData, alertPath + "/buildingName", BUILDING_NAME);
  Firebase.setString(firebaseData, alertPath + "/buildingAddress", BUILDING_ADDRESS);
  Firebase.setString(firebaseData, alertPath + "/fireBrigadePhone", FIRE_BRIGADE_PHONE);
  
  // Store contact information
  Firebase.setString(firebaseData, alertPath + "/contacts/adminEmail", ADMIN_EMAIL);
  Firebase.setString(firebaseData, alertPath + "/contacts/adminPhone", ADMIN_PHONE);
  Firebase.setString(firebaseData, alertPath + "/contacts/caretakerEmail", CARETAKER_EMAIL);
  Firebase.setString(firebaseData, alertPath + "/contacts/caretakerPhone", CARETAKER_PHONE);
  
  // Mark actions as pending
  Firebase.setBool(firebaseData, alertPath + "/actions/emailSent", false);
  Firebase.setBool(firebaseData, alertPath + "/actions/smsSent", false);
  Firebase.setBool(firebaseData, alertPath + "/actions/fireBrigadeCalled", false);
  Firebase.setBool(firebaseData, alertPath + "/actions/notificationSent", false);
  
  totalAlertsGenerated++;
  Firebase.setInt(firebaseData, "/smartBuilding/system/totalAlerts", totalAlertsGenerated);
  
  // Create notification for KUET fire brigade and admin
  createKUETFireBrigadeNotification(alertId, segmentName, alertType, timeString, segment);
  
  Serial.println("🚨 EMERGENCY ALERT: " + alertType + " in " + segmentName);
  Serial.println("   Alert ID: " + String(alertId));
  Serial.println("   KUET Fire Brigade notification created");
}

void handleWarningAlert(String segmentName, SegmentInfo &segment, String timeString) {
  unsigned long alertId = millis();
  String alertPath = "/smartBuilding/alerts/" + String(alertId);
  
  Firebase.setString(firebaseData, alertPath + "/type", "WARNING");
  Firebase.setString(firebaseData, alertPath + "/severity", "HIGH");
  Firebase.setString(firebaseData, alertPath + "/segment", segmentName);
  Firebase.setString(firebaseData, alertPath + "/timestamp", timeString);
  Firebase.setString(firebaseData, alertPath + "/message", "⚠️ Warning conditions detected in " + capitalizeFirst(segmentName));
  Firebase.setBool(firebaseData, alertPath + "/acknowledged", false);
  
  // Store sensor data
  Firebase.setInt(firebaseData, alertPath + "/details/gasLevel", segment.gasLevel);
  Firebase.setFloat(firebaseData, alertPath + "/details/temperature", segment.temperature);
  Firebase.setInt(firebaseData, alertPath + "/details/airQuality", segment.airQuality);
  
  totalAlertsGenerated++;
  Firebase.setInt(firebaseData, "/smartBuilding/system/totalAlerts", totalAlertsGenerated);
  
  Serial.println("⚠️ WARNING ALERT: Conditions in " + segmentName + " require attention");
}

// ====== KUET Fire Brigade Notification System ======
void createKUETFireBrigadeNotification(unsigned long alertId, String segment, String alertType, String timestamp, SegmentInfo &segmentInfo) {
  String notifyPath = "/smartBuilding/emergencyNotifications/" + String(alertId);
  
  // Emergency notification metadata
  Firebase.setString(firebaseData, notifyPath + "/id", String(alertId));
  Firebase.setString(firebaseData, notifyPath + "/type", alertType);
  Firebase.setString(firebaseData, notifyPath + "/segment", segment);
  Firebase.setString(firebaseData, notifyPath + "/timestamp", timestamp);
  Firebase.setBool(firebaseData, notifyPath + "/processed", false);
  Firebase.setString(firebaseData, notifyPath + "/status", "URGENT");
  Firebase.setString(firebaseData, notifyPath + "/priority", "CRITICAL");
  
  // Building information for emergency responders
  Firebase.setString(firebaseData, notifyPath + "/buildingInfo/name", BUILDING_NAME);
  Firebase.setString(firebaseData, notifyPath + "/buildingInfo/address", BUILDING_ADDRESS);
  Firebase.setString(firebaseData, notifyPath + "/buildingInfo/area", "Fulbarigate, KUET Campus Area");
  Firebase.setString(firebaseData, notifyPath + "/buildingInfo/coordinates", "22.8145° N, 89.5145° E");
  
  // Emergency details for KUET Fire Brigade
  String emergencyMessage;
  if (alertType == "FIRE_EMERGENCY") {
    emergencyMessage = "FIRE EMERGENCY at " + String(BUILDING_NAME) + ", " + String(BUILDING_ADDRESS) + ". " +
                      "FIRE DETECTED in " + capitalizeFirst(segment) + " area. " +
                      "Temperature: " + String(segmentInfo.temperature, 1) + "°C, " +
                      "Gas Level: " + String(segmentInfo.gasLevel) + ". " +
                      "IMMEDIATE RESPONSE REQUIRED!";
  } else {
    emergencyMessage = "CRITICAL EMERGENCY at " + String(BUILDING_NAME) + ", " + String(BUILDING_ADDRESS) + ". " +
                      "Critical conditions in " + capitalizeFirst(segment) + " area. " +
                      "Temperature: " + String(segmentInfo.temperature, 1) + "°C, " +
                      "Gas Level: " + String(segmentInfo.gasLevel) + ". " +
                      "PLEASE RESPOND IMMEDIATELY!";
  }
  
  Firebase.setString(firebaseData, notifyPath + "/emergencyMessage", emergencyMessage);
  
  // Contact information
  Firebase.setString(firebaseData, notifyPath + "/contacts/fireBrigadePhone", FIRE_BRIGADE_PHONE);
  Firebase.setString(firebaseData, notifyPath + "/contacts/adminPhone", ADMIN_PHONE);
  Firebase.setString(firebaseData, notifyPath + "/contacts/caretakerPhone", CARETAKER_PHONE);
  
  // Action flags for automated response
  Firebase.setBool(firebaseData, notifyPath + "/actions/callFireBrigade", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/notifyAdmin", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/notifyCaretaker", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/sendSMS", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/sendEmail", true);
  
  // Completion tracking
  Firebase.setBool(firebaseData, notifyPath + "/completion/fireBrigadeCalled", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/adminNotified", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/caretakerNotified", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/smsSent", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/emailSent", false);
  
  // Emergency response instructions
  Firebase.setString(firebaseData, notifyPath + "/instructions/immediate", "1. Call KUET Fire Brigade: " + String(FIRE_BRIGADE_PHONE));
  Firebase.setString(firebaseData, notifyPath + "/instructions/secondary", "2. Evacuate building, 3. Check all occupants, 4. Secure area");
  Firebase.setString(firebaseData, notifyPath + "/instructions/location", "Building located near KUET campus at Fulbarigate");
  
  Serial.println("📞 KUET Fire Brigade notification created:");
  Serial.println("   Emergency: " + alertType + " in " + segment);
  Serial.println("   Fire Brigade: " + String(FIRE_BRIGADE_PHONE));
  Serial.println("   Building: " + String(BUILDING_NAME));
  Serial.println("   Location: " + String(BUILDING_ADDRESS));
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
      Serial.println("💓 Heartbeat sent - System uptime: " + String(millis() / 1000) + " seconds");
    } else {
      Serial.println("❌ Failed to send heartbeat to Firebase");
      firebaseConnected = false; // Mark as disconnected
    }
  } else {
    Serial.println("💓 Heartbeat skipped - Firebase not connected (Uptime: " + String(millis() / 1000) + "s)");
    // Try simple reconnection every 5 heartbeats (2.5 minutes)
    static int skipCount = 0;
    skipCount++;
    if (skipCount >= 5) {
      Serial.println("🔄 Attempting simplified Firebase reconnection...");
      // Use config objects for reconnection too
      config.database_url = "https://" + String(FIREBASE_HOST);
      config.api_key = API_KEY;
      config.signer.tokens.legacy_token = FIREBASE_AUTH;
      
      Firebase.begin(&config, &auth);
      delay(2000);
      if (Firebase.setString(firebaseData, "/test/reconnect", "testing")) {
        firebaseConnected = true;
        Serial.println("✅ Firebase reconnected successfully!");
      } else {
        Serial.println("❌ Firebase reconnection failed");
      }
      skipCount = 0;
    }
  }
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
  
  // Only print if emergency state changes
  if (anyEmergency != emergencyState) {
    emergencyState = anyEmergency;
    
    if (emergencyState) {
      digitalWrite(EMERGENCY_LED, HIGH);
      Firebase.setString(firebaseData, "/smartBuilding/system/buildingStatus", "EMERGENCY");
      Serial.println("🚨🚨🚨 BUILDING IN EMERGENCY STATE! 🚨🚨🚨");
      Serial.println("🔥 Emergency rooms:");
      if (bedroomEmergency) Serial.println("   - BEDROOM");
      if (kitchenEmergency) Serial.println("   - KITCHEN");
      if (parkingEmergency) Serial.println("   - PARKING LOT");
      
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
  Firebase.setString(firebaseData, "/smartBuilding/system/esp32Version", "2.0.0");
  Firebase.setString(firebaseData, "/smartBuilding/system/description", "Smart Building Monitoring System - 3 Room Configuration");
  Firebase.setInt(firebaseData, "/smartBuilding/system/totalRooms", 3);
  
  // Room configuration
  Firebase.setString(firebaseData, "/smartBuilding/config/room1", "Bedroom");
  Firebase.setString(firebaseData, "/smartBuilding/config/room2", "Kitchen");
  Firebase.setString(firebaseData, "/smartBuilding/config/room3", "Parking Lot");
  
  Serial.println("📋 System information sent to Firebase");
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

// Send contact configuration to Firebase for web app
void sendContactConfiguration() {
  Serial.println("📞 Sending contact configuration to Firebase...");
  
  String contactPath = "/smartBuilding/contacts";
  
  // Owner information
  Firebase.setString(firebaseData, contactPath + "/owner/email", OWNER_EMAIL);
  Firebase.setString(firebaseData, contactPath + "/owner/phone", OWNER_PHONE);
  Firebase.setString(firebaseData, contactPath + "/owner/name", "Building Owner");
  Firebase.setBool(firebaseData, contactPath + "/owner/notifyOnEmergency", true);
  Firebase.setBool(firebaseData, contactPath + "/owner/notifyOnWarning", true);
  
  // Fire station information
  Firebase.setString(firebaseData, contactPath + "/fireStation/phone", FIRE_STATION_PHONE);
  Firebase.setString(firebaseData, contactPath + "/fireStation/name", "Fire Station");
  Firebase.setBool(firebaseData, contactPath + "/fireStation/autoCallOnFire", true);
  
  // Building information
  Firebase.setString(firebaseData, contactPath + "/building/name", BUILDING_NAME);
  Firebase.setString(firebaseData, contactPath + "/building/address", BUILDING_ADDRESS);
  
  Serial.println("✅ Contact configuration sent successfully");
}

// Create notification entry in queue for web app to process
void createNotificationEntry(unsigned long alertId, String room, String type, String timestamp, int gasLevel, bool flameDetected) {
  String notifyPath = "/smartBuilding/notificationQueue/" + String(alertId);
  
  // Notification metadata
  Firebase.setString(firebaseData, notifyPath + "/id", String(alertId));
  Firebase.setString(firebaseData, notifyPath + "/type", type);
  Firebase.setString(firebaseData, notifyPath + "/room", room);
  Firebase.setString(firebaseData, notifyPath + "/timestamp", timestamp);
  Firebase.setBool(firebaseData, notifyPath + "/processed", false);
  Firebase.setString(firebaseData, notifyPath + "/status", "PENDING");
  
  // Alert details
  Firebase.setInt(firebaseData, notifyPath + "/details/gasLevel", gasLevel);
  Firebase.setBool(firebaseData, notifyPath + "/details/flameDetected", flameDetected);
  
  // Determine message and priority based on type
  String message, title, priority;
  bool requiresCall = false;
  
  if (type == "FIRE") {
    title = "🔥 FIRE EMERGENCY!";
    message = "CRITICAL: Fire detected in " + capitalizeFirst(room) + " of " + String(BUILDING_NAME) + 
              ". Location: " + String(BUILDING_ADDRESS) + ". EVACUATE IMMEDIATELY!";
    priority = "CRITICAL";
    requiresCall = true;
    
    Firebase.setString(firebaseData, notifyPath + "/actions/email/subject", "🚨 FIRE EMERGENCY - " + String(BUILDING_NAME));
    Firebase.setString(firebaseData, notifyPath + "/actions/email/body", 
      "FIRE ALERT!\n\n" +
      String("Building: ") + BUILDING_NAME + "\n" +
      String("Location: ") + BUILDING_ADDRESS + "\n" +
      String("Room: ") + capitalizeFirst(room) + "\n" +
      String("Time: ") + timestamp + "\n" +
      String("Gas Level: ") + String(gasLevel) + "\n\n" +
      "ACTION REQUIRED:\n" +
      "1. Evacuate building immediately\n" +
      "2. Call fire station: " + String(FIRE_STATION_PHONE) + "\n" +
      "3. Check security camera footage\n" +
      "4. Account for all occupants\n\n" +
      "This is an automated alert from your Smart Building Monitoring System.");
    
    Firebase.setString(firebaseData, notifyPath + "/actions/sms/message", 
      "🔥 FIRE ALERT! " + capitalizeFirst(room) + " at " + String(BUILDING_NAME) + ". EVACUATE NOW! Fire Station: " + String(FIRE_STATION_PHONE));
    
    Firebase.setString(firebaseData, notifyPath + "/actions/call/number", FIRE_STATION_PHONE);
    Firebase.setString(firebaseData, notifyPath + "/actions/call/message", 
      "Emergency fire alert at " + String(BUILDING_NAME) + ", " + String(BUILDING_ADDRESS) + ". Fire detected in " + capitalizeFirst(room) + ". Please respond immediately.");
    
  } else if (type == "GAS") {
    title = "⚠️ Gas/Smoke Warning";
    message = "WARNING: High gas/smoke level (" + String(gasLevel) + ") detected in " + capitalizeFirst(room) + 
              " of " + String(BUILDING_NAME) + ". Please investigate.";
    priority = "HIGH";
    requiresCall = false;
    
    Firebase.setString(firebaseData, notifyPath + "/actions/email/subject", "⚠️ Gas Warning - " + String(BUILDING_NAME));
    Firebase.setString(firebaseData, notifyPath + "/actions/email/body", 
      "Gas/Smoke Warning\n\n" +
      String("Building: ") + BUILDING_NAME + "\n" +
      String("Location: ") + BUILDING_ADDRESS + "\n" +
      String("Room: ") + capitalizeFirst(room) + "\n" +
      String("Time: ") + timestamp + "\n" +
      String("Gas Level: ") + String(gasLevel) + " (Threshold: 300)\n\n" +
      "RECOMMENDED ACTIONS:\n" +
      "1. Check the affected room\n" +
      "2. Ventilate the area\n" +
      "3. Investigate the source\n" +
      "4. Monitor for escalation\n\n" +
      "This is an automated alert from your Smart Building Monitoring System.");
    
    Firebase.setString(firebaseData, notifyPath + "/actions/sms/message", 
      "⚠️ GAS WARNING: High gas level (" + String(gasLevel) + ") in " + capitalizeFirst(room) + " at " + String(BUILDING_NAME) + ". Please check.");
  }
  
  Firebase.setString(firebaseData, notifyPath + "/title", title);
  Firebase.setString(firebaseData, notifyPath + "/message", message);
  Firebase.setString(firebaseData, notifyPath + "/priority", priority);
  
  // Contact information
  Firebase.setString(firebaseData, notifyPath + "/contacts/ownerEmail", OWNER_EMAIL);
  Firebase.setString(firebaseData, notifyPath + "/contacts/ownerPhone", OWNER_PHONE);
  Firebase.setString(firebaseData, notifyPath + "/contacts/fireStationPhone", FIRE_STATION_PHONE);
  Firebase.setString(firebaseData, notifyPath + "/contacts/buildingName", BUILDING_NAME);
  Firebase.setString(firebaseData, notifyPath + "/contacts/buildingAddress", BUILDING_ADDRESS);
  
  // Action flags
  Firebase.setBool(firebaseData, notifyPath + "/actions/sendEmail", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/sendSMS", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/sendPushNotification", true);
  Firebase.setBool(firebaseData, notifyPath + "/actions/callFireStation", requiresCall);
  
  // Completion tracking
  Firebase.setBool(firebaseData, notifyPath + "/completion/emailSent", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/smsSent", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/pushSent", false);
  Firebase.setBool(firebaseData, notifyPath + "/completion/callMade", false);
  Firebase.setString(firebaseData, notifyPath + "/completion/lastAttempt", "");
  
  Serial.println("📬 Notification queue entry created: ID " + String(alertId));
  Serial.println("   Type: " + type + " | Room: " + room + " | Priority: " + priority);
}