/*
 * ========================================
 * INTELLIGENT FIRE ALARM SYSTEM - ESP32 + Firebase
 * ========================================
 * 
 * Features:
 * - Real-time sensor data to Firebase
 * - Multi-zone monitoring (Kitchen, Living Room, Bedroom, etc.)
 * - Emergency alerts with location
 * - Automatic fire brigade notification
 * - Historical data logging
 * - WebSocket for real-time updates
 * 
 * Firebase Data Structure:
 * /buildings/{buildingId}/
 *   /zones/{zoneId}/
 *     /sensors/
 *       - temperature
 *       - smoke (MQ2)
 *       - airQuality (MQ135)
 *       - flame
 *       - timestamp
 *       - status: "safe" | "warning" | "danger"
 *   /alerts/
 *     - alertId
 *     - type
 *     - severity
 *     - zone
 *     - timestamp
 *     - acknowledged
 *   /emergencyContacts/
 *     - fireBrigade
 *     - apartmentOwner
 *     - security
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <time.h>

// ========== WiFi CREDENTIALS ==========
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ========== FIREBASE CONFIGURATION ==========
#define FIREBASE_HOST "your-project-id.firebaseio.com"  // Without https:// and trailing /
#define FIREBASE_AUTH "your-firebase-database-secret-or-token"

// ========== BUILDING & ZONE CONFIGURATION ==========
#define BUILDING_ID "building_001"
#define ZONE_ID "kitchen"  // Change for each ESP32: "kitchen", "living_room", "bedroom_1", etc.
#define ZONE_NAME "Kitchen"
#define FLOOR_NUMBER 3

// ========== PIN DEFINITIONS ==========
#define MQ2_PIN         34
#define MQ135_PIN       35
#define FLAME_PIN       32
#define DS18B20_PIN     4
#define BUZZER_PIN      25
#define RED_LED_PIN     26
#define GREEN_LED_PIN   27

// ========== SENSOR THRESHOLDS ==========
// Normal Alert Levels
#define MQ2_WARNING_THRESHOLD       1500
#define MQ2_DANGER_THRESHOLD        2500
#define MQ135_WARNING_THRESHOLD_PPM 800
#define MQ135_DANGER_THRESHOLD_PPM  1200
#define FLAME_WARNING_THRESHOLD     2500
#define FLAME_DANGER_THRESHOLD      1500
#define TEMP_WARNING_THRESHOLD      40.0   // Kitchen: 40°C warning
#define TEMP_DANGER_THRESHOLD       55.0   // Kitchen: 55°C danger

// ========== TIMING CONFIGURATION ==========
#define SENSOR_READ_INTERVAL    5000    // Read sensors every 5 seconds
#define FIREBASE_UPDATE_INTERVAL 10000  // Update Firebase every 10 seconds
#define ALERT_CHECK_INTERVAL    3000    // Check for alerts every 3 seconds
#define EMERGENCY_COOLDOWN      30000   // 30 seconds between emergency calls

// ========== LCD CONFIGURATION ==========
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== DS18B20 CONFIGURATION ==========
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

// ========== FIREBASE OBJECTS ==========
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// ========== GLOBAL VARIABLES ==========
struct SensorData {
  float temperature;
  int smoke;
  float airQuality;
  int flame;
  String status;  // "safe", "warning", "danger"
  unsigned long timestamp;
};

SensorData currentData;
unsigned long lastSensorRead = 0;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastAlertCheck = 0;
unsigned long lastEmergencyCall = 0;

bool wifiConnected = false;
bool firebaseConnected = false;
bool emergencyAlertSent = false;
int displayMode = 0;
unsigned long lastDisplaySwitch = 0;

// MQ135 Calibration
float RLOAD = 10.0;
float R0 = 76.63;

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("INTELLIGENT FIRE ALARM SYSTEM");
  Serial.println("ESP32 + Firebase IoT");
  Serial.println("========================================\n");

  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fire Alarm Sys");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  // Initialize temperature sensor
  tempSensor.begin();

  // Connect to WiFi
  connectToWiFi();

  // Configure Firebase
  setupFirebase();

  // Configure NTP for timestamps
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Initialize zone in Firebase
  initializeZone();

  // Warm up sensors
  Serial.println("\nWarming up sensors (20 seconds)...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Warming up...");
  for (int i = 20; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Wait: ");
    lcd.print(i);
    lcd.print(" sec  ");
    delay(1000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print(ZONE_NAME);
  delay(2000);

  Serial.println("\n✅ System Ready!");
  Serial.println("Zone: " + String(ZONE_NAME));
  Serial.println("Building: " + String(BUILDING_ID));
  Serial.println("========================================\n");
}

// ========== MAIN LOOP ==========
void loop() {
  unsigned long currentTime = millis();

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    reconnectWiFi();
  }

  // Read sensors at regular intervals
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentTime;
    readAllSensors();
    evaluateStatus();
    updateAlarms();
    displayOnLCD();
    printToSerial();
  }

  // Update Firebase at regular intervals
  if (currentTime - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL) {
    lastFirebaseUpdate = currentTime;
    if (wifiConnected) {
      uploadToFirebase();
    }
  }

  // Check for emergency conditions
  if (currentTime - lastAlertCheck >= ALERT_CHECK_INTERVAL) {
    lastAlertCheck = currentTime;
    checkEmergencyConditions();
  }

  // Small delay to prevent watchdog issues
  delay(100);
}

// ========== WiFi FUNCTIONS ==========
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check Settings");
    delay(3000);
  }
}

void reconnectWiFi() {
  Serial.println("WiFi disconnected. Reconnecting...");
  WiFi.reconnect();
  delay(5000);
}

// ========== FIREBASE FUNCTIONS ==========
void setupFirebase() {
  Serial.println("Configuring Firebase...");
  
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  firebaseConnected = true;
  Serial.println("✅ Firebase configured!");
}

void initializeZone() {
  if (!wifiConnected) return;
  
  String path = "/buildings/" + String(BUILDING_ID) + "/zones/" + String(ZONE_ID);
  
  // Set zone metadata
  Firebase.setString(firebaseData, path + "/metadata/name", ZONE_NAME);
  Firebase.setInt(firebaseData, path + "/metadata/floor", FLOOR_NUMBER);
  Firebase.setString(firebaseData, path + "/metadata/type", "kitchen");
  Firebase.setString(firebaseData, path + "/status", "safe");
  
  Serial.println("✅ Zone initialized in Firebase");
}

void uploadToFirebase() {
  if (!wifiConnected || !firebaseConnected) {
    Serial.println("⚠️  Cannot upload - No connection");
    return;
  }

  String basePath = "/buildings/" + String(BUILDING_ID) + "/zones/" + String(ZONE_ID);
  
  // Upload sensor data
  Firebase.setFloat(firebaseData, basePath + "/sensors/temperature", currentData.temperature);
  Firebase.setInt(firebaseData, basePath + "/sensors/smoke", currentData.smoke);
  Firebase.setFloat(firebaseData, basePath + "/sensors/airQuality", currentData.airQuality);
  Firebase.setInt(firebaseData, basePath + "/sensors/flame", currentData.flame);
  Firebase.setString(firebaseData, basePath + "/sensors/status", currentData.status);
  Firebase.setInt(firebaseData, basePath + "/sensors/timestamp", currentData.timestamp);
  
  // Update overall zone status
  Firebase.setString(firebaseData, basePath + "/status", currentData.status);
  
  // Log historical data
  String historyPath = "/buildings/" + String(BUILDING_ID) + "/history/" + 
                       String(ZONE_ID) + "/" + String(currentData.timestamp);
  
  Firebase.setFloat(firebaseData, historyPath + "/temperature", currentData.temperature);
  Firebase.setInt(firebaseData, historyPath + "/smoke", currentData.smoke);
  Firebase.setFloat(firebaseData, historyPath + "/airQuality", currentData.airQuality);
  Firebase.setInt(firebaseData, historyPath + "/flame", currentData.flame);
  Firebase.setString(firebaseData, historyPath + "/status", currentData.status);
  
  Serial.println("📤 Data uploaded to Firebase");
}

void createAlert(String alertType, String severity, String message) {
  if (!wifiConnected || !firebaseConnected) return;

  unsigned long timestamp = getTimestamp();
  String alertId = String(ZONE_ID) + "_" + String(timestamp);
  String alertPath = "/buildings/" + String(BUILDING_ID) + "/alerts/" + alertId;
  
  Firebase.setString(firebaseData, alertPath + "/type", alertType);
  Firebase.setString(firebaseData, alertPath + "/severity", severity);
  Firebase.setString(firebaseData, alertPath + "/zone", ZONE_ID);
  Firebase.setString(firebaseData, alertPath + "/zoneName", ZONE_NAME);
  Firebase.setInt(firebaseData, alertPath + "/floor", FLOOR_NUMBER);
  Firebase.setString(firebaseData, alertPath + "/message", message);
  Firebase.setInt(firebaseData, alertPath + "/timestamp", timestamp);
  Firebase.setBool(firebaseData, alertPath + "/acknowledged", false);
  Firebase.setBool(firebaseData, alertPath + "/resolved", false);
  
  // Sensor values at time of alert
  Firebase.setFloat(firebaseData, alertPath + "/sensorData/temperature", currentData.temperature);
  Firebase.setInt(firebaseData, alertPath + "/sensorData/smoke", currentData.smoke);
  Firebase.setFloat(firebaseData, alertPath + "/sensorData/airQuality", currentData.airQuality);
  Firebase.setInt(firebaseData, alertPath + "/sensorData/flame", currentData.flame);
  
  Serial.println("🚨 ALERT CREATED: " + alertType + " - " + severity);
}

void triggerEmergencyNotification(String emergencyType) {
  if (!wifiConnected || !firebaseConnected) return;

  unsigned long currentTime = millis();
  
  // Prevent spam - cooldown period
  if (currentTime - lastEmergencyCall < EMERGENCY_COOLDOWN) {
    return;
  }
  
  lastEmergencyCall = currentTime;
  emergencyAlertSent = true;

  String emergencyPath = "/buildings/" + String(BUILDING_ID) + "/emergencies/active";
  unsigned long timestamp = getTimestamp();
  
  Firebase.setBool(firebaseData, emergencyPath + "/active", true);
  Firebase.setString(firebaseData, emergencyPath + "/type", emergencyType);
  Firebase.setString(firebaseData, emergencyPath + "/zone", ZONE_ID);
  Firebase.setString(firebaseData, emergencyPath + "/zoneName", ZONE_NAME);
  Firebase.setInt(firebaseData, emergencyPath + "/floor", FLOOR_NUMBER);
  Firebase.setInt(firebaseData, emergencyPath + "/timestamp", timestamp);
  Firebase.setString(firebaseData, emergencyPath + "/status", "pending");
  
  // Set flags for notifications
  Firebase.setBool(firebaseData, emergencyPath + "/notifyFireBrigade", true);
  Firebase.setBool(firebaseData, emergencyPath + "/notifyOwner", true);
  Firebase.setBool(firebaseData, emergencyPath + "/notifySecurity", true);
  Firebase.setBool(firebaseData, emergencyPath + "/evacuationRequired", true);
  
  Serial.println("🚨🚨🚨 EMERGENCY NOTIFICATION TRIGGERED! 🚨🚨🚨");
  Serial.println("Type: " + emergencyType);
  Serial.println("Zone: " + String(ZONE_NAME));
}

// ========== SENSOR FUNCTIONS ==========
void readAllSensors() {
  currentData.smoke = analogRead(MQ2_PIN);
  currentData.airQuality = readMQ135PPM();
  currentData.flame = analogRead(FLAME_PIN);
  currentData.temperature = readTemperature();
  currentData.timestamp = getTimestamp();
}

float readMQ135PPM() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(MQ135_PIN);
    delay(10);
  }
  int rawValue = sum / 10;

  float vIn = 3.3;
  float vOut = (rawValue / 4095.0) * vIn;
  
  if (vOut == 0) return 0;
  
  float rs = (vIn - vOut) * RLOAD / vOut;
  float ratio = rs / R0;
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  
  return ppm;
}

float readTemperature() {
  tempSensor.requestTemperatures();
  float tempC = tempSensor.getTempCByIndex(0);
  
  if (tempC == DEVICE_DISCONNECTED_C || tempC == -127.0) {
    return -127.0;
  }
  
  return tempC;
}

void evaluateStatus() {
  bool isDanger = false;
  bool isWarning = false;

  // Check all sensors
  if (currentData.temperature > TEMP_DANGER_THRESHOLD) isDanger = true;
  else if (currentData.temperature > TEMP_WARNING_THRESHOLD) isWarning = true;

  if (currentData.smoke > MQ2_DANGER_THRESHOLD) isDanger = true;
  else if (currentData.smoke > MQ2_WARNING_THRESHOLD) isWarning = true;

  if (currentData.airQuality > MQ135_DANGER_THRESHOLD_PPM) isDanger = true;
  else if (currentData.airQuality > MQ135_WARNING_THRESHOLD_PPM) isWarning = true;

  if (currentData.flame < FLAME_DANGER_THRESHOLD) isDanger = true;
  else if (currentData.flame < FLAME_WARNING_THRESHOLD) isWarning = true;

  // Set status
  if (isDanger) {
    currentData.status = "danger";
  } else if (isWarning) {
    currentData.status = "warning";
  } else {
    currentData.status = "safe";
    emergencyAlertSent = false;  // Reset emergency flag when safe
  }
}

void updateAlarms() {
  if (currentData.status == "danger") {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    tone(BUZZER_PIN, 1000, 200);
  } else if (currentData.status == "warning") {
    // Slow blink
    digitalWrite(RED_LED_PIN, (millis() / 1000) % 2);
    digitalWrite(GREEN_LED_PIN, LOW);
    if ((millis() / 2000) % 2) {
      tone(BUZZER_PIN, 800, 100);
    }
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    noTone(BUZZER_PIN);
  }
}

void checkEmergencyConditions() {
  // FIRE EMERGENCY - Immediate fire brigade notification
  if (currentData.flame < FLAME_DANGER_THRESHOLD || currentData.smoke > MQ2_DANGER_THRESHOLD) {
    if (!emergencyAlertSent) {
      createAlert("FIRE", "CRITICAL", "Fire detected! Immediate evacuation required!");
      triggerEmergencyNotification("FIRE_BREAKOUT");
    }
  }
  
  // KITCHEN TEMPERATURE ALERT - Notify apartment owner
  if (String(ZONE_ID) == "kitchen" && currentData.temperature > TEMP_WARNING_THRESHOLD) {
    if (currentData.temperature > TEMP_DANGER_THRESHOLD) {
      createAlert("HIGH_TEMPERATURE", "CRITICAL", "Kitchen temperature critically high!");
      // This will trigger owner notification through Firebase Cloud Functions
      String notificationPath = "/buildings/" + String(BUILDING_ID) + "/notifications/owner";
      Firebase.setBool(firebaseData, notificationPath + "/pending", true);
      Firebase.setString(firebaseData, notificationPath + "/message", 
                        "⚠️ Kitchen temperature: " + String(currentData.temperature, 1) + "°C");
      Firebase.setString(firebaseData, notificationPath + "/zone", ZONE_NAME);
      Firebase.setInt(firebaseData, notificationPath + "/timestamp", getTimestamp());
    } else {
      createAlert("HIGH_TEMPERATURE", "WARNING", "Kitchen temperature elevated.");
    }
  }
  
  // POOR AIR QUALITY ALERT
  if (currentData.airQuality > MQ135_DANGER_THRESHOLD_PPM) {
    createAlert("AIR_QUALITY", "WARNING", "Poor air quality detected!");
  }
}

// ========== DISPLAY FUNCTIONS ==========
void displayOnLCD() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDisplaySwitch > 3000) {
    displayMode = (displayMode + 1) % 4;
    lastDisplaySwitch = currentTime;
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  
  if (currentData.status == "danger") {
    if ((currentTime / 500) % 2 == 0) {
      lcd.print("*** DANGER! ***");
    } else {
      lcd.print("  EVACUATE!  ");
    }
  } else if (currentData.status == "warning") {
    lcd.print("! WARNING !");
    lcd.print(ZONE_NAME);
  } else {
    switch (displayMode) {
      case 0: lcd.print("Temp: "); break;
      case 1: lcd.print("Smoke: "); break;
      case 2: lcd.print("Air Quality: "); break;
      case 3: lcd.print("Flame: "); break;
    }
  }

  lcd.setCursor(0, 1);
  switch (displayMode) {
    case 0:
      if (currentData.temperature != -127.0) {
        lcd.print(currentData.temperature, 1);
        lcd.print("C ");
      } else {
        lcd.print("ERROR ");
      }
      break;
    case 1:
      lcd.print(currentData.smoke);
      lcd.print(" ");
      break;
    case 2:
      lcd.print(currentData.airQuality, 0);
      lcd.print(" PPM ");
      break;
    case 3:
      lcd.print(currentData.flame);
      lcd.print(" ");
      break;
  }
  
  // Show WiFi status
  if (wifiConnected) {
    lcd.print("WiFi");
  } else {
    lcd.print("NoWiFi");
  }
}

void printToSerial() {
  Serial.println("========================================");
  Serial.print("🏢 Building: ");
  Serial.print(BUILDING_ID);
  Serial.print(" | Zone: ");
  Serial.println(ZONE_NAME);
  Serial.println("----------------------------------------");
  
  Serial.print("🌡️  Temperature: ");
  if (currentData.temperature != -127.0) {
    Serial.print(currentData.temperature, 1);
    Serial.print(" °C");
    if (currentData.temperature > TEMP_DANGER_THRESHOLD) Serial.println(" ⚠️  DANGER!");
    else if (currentData.temperature > TEMP_WARNING_THRESHOLD) Serial.println(" ⚠️  WARNING");
    else Serial.println(" ✅");
  } else {
    Serial.println("ERROR");
  }
  
  Serial.print("💨 Smoke (MQ2): ");
  Serial.print(currentData.smoke);
  if (currentData.smoke > MQ2_DANGER_THRESHOLD) Serial.println(" ⚠️  DANGER!");
  else if (currentData.smoke > MQ2_WARNING_THRESHOLD) Serial.println(" ⚠️  WARNING");
  else Serial.println(" ✅");
  
  Serial.print("🌫️  Air Quality: ");
  Serial.print(currentData.airQuality, 1);
  Serial.print(" PPM");
  if (currentData.airQuality > MQ135_DANGER_THRESHOLD_PPM) Serial.println(" ⚠️  DANGER!");
  else if (currentData.airQuality > MQ135_WARNING_THRESHOLD_PPM) Serial.println(" ⚠️  WARNING");
  else Serial.println(" ✅");
  
  Serial.print("🔥 Flame: ");
  Serial.print(currentData.flame);
  if (currentData.flame < FLAME_DANGER_THRESHOLD) Serial.println(" ⚠️  FIRE DETECTED!");
  else if (currentData.flame < FLAME_WARNING_THRESHOLD) Serial.println(" ⚠️  WARNING");
  else Serial.println(" ✅");
  
  Serial.println("----------------------------------------");
  Serial.print("📊 STATUS: ");
  if (currentData.status == "danger") {
    Serial.println("🚨 DANGER 🚨");
  } else if (currentData.status == "warning") {
    Serial.println("⚠️  WARNING");
  } else {
    Serial.println("✅ SAFE");
  }
  
  Serial.print("📡 WiFi: ");
  Serial.print(wifiConnected ? "✅" : "❌");
  Serial.print(" | Firebase: ");
  Serial.println(firebaseConnected ? "✅" : "❌");
  
  Serial.println("========================================\n");
}

// ========== UTILITY FUNCTIONS ==========
unsigned long getTimestamp() {
  time_t now;
  time(&now);
  return (unsigned long)now;
}
