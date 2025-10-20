/*
 * Smart Building Monitoring System - Arduino Mega Controller
 * 4 Segments: Kitchen, Bedroom, Parking, Central Gas Chamber
 * 
 * Kitchen: MQ135 sensor, DHT11, Flame Sensor, Buzzer, Red+Green LED
 * Bedroom: DS18B20, MQ2, Flame Sensor, Buzzer, Red+Green LED  
 * Parking: MQ2, Flame Sensor, Red+Green LED
 * Central Gas Chamber: MQ2, Buzzer
 * 
 * Sends sensor data to ESP32 for Firebase integration and emergency alerts
 */

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// ========== PIN DEFINITIONS ==========
// Kitchen Sensors
#define DHT_PIN 22            // DHT11 for temperature/humidity
#define DHT_TYPE DHT11
#define MQ135_PIN A3          // Air quality sensor
#define FLAME_KITCHEN 25      // Flame sensor
#define BUZZER_KITCHEN 28     // Kitchen buzzer
#define LED_GREEN_KITCHEN 31  // Kitchen safe indicator
#define LED_RED_KITCHEN 34    // Kitchen danger indicator

// Bedroom Sensors  
#define ONE_WIRE_BUS 23       // DS18B20 temperature sensor
#define MQ2_BEDROOM A0        // Gas/smoke sensor
#define FLAME_BEDROOM 24      // Flame sensor
#define BUZZER_BEDROOM 27     // Bedroom buzzer
#define LED_GREEN_BEDROOM 30  // Bedroom safe indicator
#define LED_RED_BEDROOM 33    // Bedroom danger indicator

// Parking Sensors
#define MQ2_PARKING A2        // Gas/smoke sensor
#define FLAME_PARKING 26      // Flame sensor
#define LED_GREEN_PARKING 32  // Parking safe indicator
#define LED_RED_PARKING 35    // Parking danger indicator

// Central Gas Chamber
#define MQ2_CENTRAL A1        // Central gas monitoring
#define BUZZER_CENTRAL 29     // Central chamber buzzer

// ========== SENSOR THRESHOLDS ==========
#define MQ2_THRESHOLD 300      // Gas/Smoke threshold (analog)
#define MQ135_THRESHOLD 400    // Air quality threshold (analog)
#define TEMP_THRESHOLD_HIGH 35 // High temperature alert (°C)
#define TEMP_THRESHOLD_LOW 10  // Low temperature alert (°C)
#define HUMIDITY_HIGH 70       // High humidity alert (%)
#define HUMIDITY_LOW 30        // Low humidity alert (%)

// Critical thresholds for immediate emergency
#define MQ2_CRITICAL 500       // Critical gas level
#define MQ135_CRITICAL 600     // Critical air quality
#define TEMP_CRITICAL 45       // Critical temperature

// ========== SENSOR OBJECTS ==========
DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// ========== SEGMENT STATUS STRUCTURE ==========
struct SegmentData {
  const char* segmentName;
  bool flameDetected;
  int gasLevel;
  float temperature;        // Individual temperature readings
  float humidity;          // For kitchen only (DHT11)
  int airQuality;          // For kitchen only (MQ135)
  bool isEmergency;        // Fire or critical conditions
  bool isDangerous;        // Warning conditions
  bool hasBuzzer;          // Does this segment have buzzer
  bool hasLEDs;            // Does this segment have LED indicators
  unsigned long lastAlert; // Last alert time for debouncing
};

SegmentData kitchen = {"Kitchen", false, 0, 0, 0, 0, false, false, true, true, 0};
SegmentData bedroom = {"Bedroom", false, 0, 0, 0, 0, false, false, true, true, 0};
SegmentData parking = {"Parking", false, 0, 0, 0, 0, false, false, false, true, 0};
SegmentData centralGas = {"Central_Gas", false, 0, 0, 0, 0, false, false, true, false, 0};

// ========== GLOBAL VARIABLES ==========
float globalTemperature = 0;   // Average/general temperature
float globalHumidity = 0;      // Kitchen humidity (DHT11)
unsigned long lastSensorRead = 0;
unsigned long lastDataSend = 0;
int dataSendCount = 0;
bool systemEmergency = false;
unsigned long emergencyStartTime = 0;

const unsigned long SENSOR_INTERVAL = 1000;  // Read sensors every 1 second
const unsigned long SEND_INTERVAL = 3000;    // Send data every 3 seconds
const unsigned long ALERT_DEBOUNCE = 5000;   // 5 second debounce for alerts

void setup() {
  Serial.begin(115200);
  Serial.println("Smart Building Monitoring - Arduino Mega");
  Serial.println("Initializing components...");
  
  // Initialize sensors
  dht.begin();
  ds18b20.begin();
  
  // Initialize flame sensor pins
  pinMode(FLAME_KITCHEN, INPUT);
  pinMode(FLAME_BEDROOM, INPUT);
  pinMode(FLAME_PARKING, INPUT);
  
  // Initialize buzzer pins
  pinMode(BUZZER_KITCHEN, OUTPUT);
  pinMode(BUZZER_BEDROOM, OUTPUT);
  pinMode(BUZZER_CENTRAL, OUTPUT);
  
  // Initialize LED pins
  pinMode(LED_GREEN_KITCHEN, OUTPUT);
  pinMode(LED_GREEN_BEDROOM, OUTPUT);
  pinMode(LED_GREEN_PARKING, OUTPUT);
  pinMode(LED_RED_KITCHEN, OUTPUT);
  pinMode(LED_RED_BEDROOM, OUTPUT);
  pinMode(LED_RED_PARKING, OUTPUT);
  
  // Set initial safe state - all green LEDs on, red LEDs off
  digitalWrite(LED_GREEN_KITCHEN, HIGH);
  digitalWrite(LED_GREEN_BEDROOM, HIGH);
  digitalWrite(LED_GREEN_PARKING, HIGH);
  digitalWrite(LED_RED_KITCHEN, LOW);
  digitalWrite(LED_RED_BEDROOM, LOW);
  digitalWrite(LED_RED_PARKING, LOW);
  
  // Turn off all buzzers initially
  digitalWrite(BUZZER_KITCHEN, LOW);
  digitalWrite(BUZZER_BEDROOM, LOW);
  digitalWrite(BUZZER_CENTRAL, LOW);
  
  delay(2000);
  Serial.println("System initialized.");
  Serial.println("Monitoring segments and sending data to ESP32.");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read all sensors at regular intervals
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentTime;
    readAllSensors();
    analyzeSegmentSafety();
    controlAlertsAndLEDs();
  }
  
  // Send comprehensive data to ESP32 (includes LCD display data)
  if (currentTime - lastDataSend >= SEND_INTERVAL) {
    lastDataSend = currentTime;
    sendDataToESP32();
    printSystemStatus();
  }
  
  // Handle system-wide emergency state
  handleSystemEmergency();
}

// Read all sensors from all segments
void readAllSensors() {
  // Kitchen: DHT11 (Temperature and Humidity) + MQ135 (Air Quality)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) {
    kitchen.humidity = h;
    globalHumidity = h; // Kitchen is our reference for humidity
  }
  if (!isnan(t)) {
    kitchen.temperature = t;
    globalTemperature = t; // Kitchen temperature as global reference
  }
  kitchen.airQuality = analogRead(MQ135_PIN);
  kitchen.flameDetected = (digitalRead(FLAME_KITCHEN) == LOW);
  // Kitchen doesn't have MQ2, using MQ135 as gas level reference
  kitchen.gasLevel = kitchen.airQuality;
  
  // Bedroom: DS18B20 (Precise Temperature) + MQ2 (Gas)
  ds18b20.requestTemperatures();
  float dsTemp = ds18b20.getTempCByIndex(0);
  if (dsTemp != DEVICE_DISCONNECTED_C) {
    bedroom.temperature = dsTemp;
  }
  bedroom.gasLevel = analogRead(MQ2_BEDROOM);
  bedroom.flameDetected = (digitalRead(FLAME_BEDROOM) == LOW);
  // No humidity/air quality sensors in bedroom
  bedroom.humidity = 0;
  bedroom.airQuality = 0;
  
  // Parking: MQ2 (Gas) + Flame
  parking.gasLevel = analogRead(MQ2_PARKING);
  parking.flameDetected = (digitalRead(FLAME_PARKING) == LOW);
  // Use global temperature as reference for parking
  parking.temperature = globalTemperature;
  parking.humidity = 0;
  parking.airQuality = 0;
  
  // Central Gas Chamber: MQ2 (Gas monitoring)
  centralGas.gasLevel = analogRead(MQ2_CENTRAL);
  centralGas.flameDetected = false; // No flame sensor in central chamber
  centralGas.temperature = globalTemperature; // Reference temperature
  centralGas.humidity = 0;
  centralGas.airQuality = 0;
}

// Analyze safety conditions for each segment
void analyzeSegmentSafety() {
  unsigned long currentTime = millis();
  
  // Kitchen Analysis
  kitchen.isEmergency = kitchen.flameDetected || 
                       (kitchen.temperature > TEMP_CRITICAL) ||
                       (kitchen.airQuality > MQ135_CRITICAL);
                       
  kitchen.isDangerous = kitchen.isEmergency ||
                       (kitchen.temperature > TEMP_THRESHOLD_HIGH) ||
                       (kitchen.humidity > HUMIDITY_HIGH) ||
                       (kitchen.humidity < HUMIDITY_LOW) ||
                       (kitchen.airQuality > MQ135_THRESHOLD);
  
  // Bedroom Analysis  
  bedroom.isEmergency = bedroom.flameDetected ||
                       (bedroom.temperature > TEMP_CRITICAL) ||
                       (bedroom.gasLevel > MQ2_CRITICAL);
                       
  bedroom.isDangerous = bedroom.isEmergency ||
                       (bedroom.temperature > TEMP_THRESHOLD_HIGH) ||
                       (bedroom.temperature < TEMP_THRESHOLD_LOW) ||
                       (bedroom.gasLevel > MQ2_THRESHOLD);
  
  // Parking Analysis
  parking.isEmergency = parking.flameDetected ||
                       (parking.gasLevel > MQ2_CRITICAL);
                       
  parking.isDangerous = parking.isEmergency ||
                       (parking.gasLevel > MQ2_THRESHOLD);
  
  // Central Gas Chamber Analysis (Most critical for gas monitoring)
  centralGas.isEmergency = (centralGas.gasLevel > MQ2_CRITICAL);
  centralGas.isDangerous = centralGas.isEmergency ||
                          (centralGas.gasLevel > MQ2_THRESHOLD);
  
  // Update system emergency state
  systemEmergency = kitchen.isEmergency || bedroom.isEmergency || 
                   parking.isEmergency || centralGas.isEmergency;
  
  if (systemEmergency && emergencyStartTime == 0) {
    emergencyStartTime = currentTime;
  } else if (!systemEmergency) {
    emergencyStartTime = 0;
  }
}

// Control buzzers and LEDs for all segments
void controlAlertsAndLEDs() {
  unsigned long currentTime = millis();
  
  // Kitchen alerts and LEDs
  if (kitchen.isDangerous) {
    digitalWrite(LED_RED_KITCHEN, HIGH);
    digitalWrite(LED_GREEN_KITCHEN, LOW);
    
    if (kitchen.isEmergency && (currentTime - kitchen.lastAlert > ALERT_DEBOUNCE)) {
      tone(BUZZER_KITCHEN, 2000, 500); // High pitch for emergency
      kitchen.lastAlert = currentTime;
    } else if (!kitchen.isEmergency && (currentTime - kitchen.lastAlert > ALERT_DEBOUNCE * 2)) {
      tone(BUZZER_KITCHEN, 1000, 200); // Lower pitch for warning
      kitchen.lastAlert = currentTime;
    }
  } else {
    digitalWrite(LED_RED_KITCHEN, LOW);
    digitalWrite(LED_GREEN_KITCHEN, HIGH);
    noTone(BUZZER_KITCHEN);
  }
  
  // Bedroom alerts and LEDs
  if (bedroom.isDangerous) {
    digitalWrite(LED_RED_BEDROOM, HIGH);
    digitalWrite(LED_GREEN_BEDROOM, LOW);
    
    if (bedroom.isEmergency && (currentTime - bedroom.lastAlert > ALERT_DEBOUNCE)) {
      tone(BUZZER_BEDROOM, 2200, 500); // Distinct pitch from kitchen
      bedroom.lastAlert = currentTime;
    } else if (!bedroom.isEmergency && (currentTime - bedroom.lastAlert > ALERT_DEBOUNCE * 2)) {
      tone(BUZZER_BEDROOM, 1200, 200);
      bedroom.lastAlert = currentTime;
    }
  } else {
    digitalWrite(LED_RED_BEDROOM, LOW);
    digitalWrite(LED_GREEN_BEDROOM, HIGH);
    noTone(BUZZER_BEDROOM);
  }
  
  // Parking LEDs (no buzzer)
  if (parking.isDangerous) {
    digitalWrite(LED_RED_PARKING, HIGH);
    digitalWrite(LED_GREEN_PARKING, LOW);
  } else {
    digitalWrite(LED_RED_PARKING, LOW);
    digitalWrite(LED_GREEN_PARKING, HIGH);
  }
  
  // Central Gas Chamber buzzer (no LEDs)
  if (centralGas.isDangerous) {
    if (centralGas.isEmergency && (currentTime - centralGas.lastAlert > ALERT_DEBOUNCE)) {
      tone(BUZZER_CENTRAL, 2500, 700); // Highest pitch for gas emergency
      centralGas.lastAlert = currentTime;
    } else if (!centralGas.isEmergency && (currentTime - centralGas.lastAlert > ALERT_DEBOUNCE * 2)) {
      tone(BUZZER_CENTRAL, 1500, 300);
      centralGas.lastAlert = currentTime;
    }
  } else {
    noTone(BUZZER_CENTRAL);
  }
}

// Send comprehensive JSON data to ESP32
void sendDataToESP32() {
  StaticJsonDocument<1024> doc;
  
  // System-wide information
  doc["systemEmergency"] = systemEmergency;
  doc["timestamp"] = millis();
  doc["emergencyDuration"] = (emergencyStartTime > 0) ? (millis() - emergencyStartTime) : 0;
  
  // Global environmental data
  JsonObject env = doc.createNestedObject("environment");
  env["globalTemperature"] = globalTemperature;
  env["globalHumidity"] = globalHumidity;
  
  // Kitchen data (DHT11 + MQ135 + Flame + Buzzer + LEDs)
  JsonObject kit = doc.createNestedObject("kitchen");
  kit["temperature"] = kitchen.temperature;
  kit["humidity"] = kitchen.humidity;
  kit["airQuality"] = kitchen.airQuality;
  kit["flameDetected"] = kitchen.flameDetected;
  kit["gasLevel"] = kitchen.gasLevel; // Using airQuality as gas reference
  kit["isEmergency"] = kitchen.isEmergency;
  kit["isDangerous"] = kitchen.isDangerous;
  kit["sensorTypes"] = "DHT11,MQ135,Flame";
  kit["hasComponents"] = "Buzzer,LEDs";
  
  // Bedroom data (DS18B20 + MQ2 + Flame + Buzzer + LEDs)
  JsonObject bed = doc.createNestedObject("bedroom");
  bed["temperature"] = bedroom.temperature;
  bed["gasLevel"] = bedroom.gasLevel;
  bed["flameDetected"] = bedroom.flameDetected;
  bed["isEmergency"] = bedroom.isEmergency;
  bed["isDangerous"] = bedroom.isDangerous;
  bed["sensorTypes"] = "DS18B20,MQ2,Flame";
  bed["hasComponents"] = "Buzzer,LEDs";
  
  // Parking data (MQ2 + Flame + LEDs)
  JsonObject park = doc.createNestedObject("parking");
  park["gasLevel"] = parking.gasLevel;
  park["flameDetected"] = parking.flameDetected;
  park["isEmergency"] = parking.isEmergency;
  park["isDangerous"] = parking.isDangerous;
  park["sensorTypes"] = "MQ2,Flame";
  park["hasComponents"] = "LEDs";
  
  // Central Gas Chamber data (MQ2 + Buzzer)
  JsonObject central = doc.createNestedObject("centralGas");
  central["gasLevel"] = centralGas.gasLevel;
  central["isEmergency"] = centralGas.isEmergency;
  central["isDangerous"] = centralGas.isDangerous;
  central["sensorTypes"] = "MQ2";
  central["hasComponents"] = "Buzzer";
  
  // Critical thresholds for ESP32 reference
  JsonObject thresholds = doc.createNestedObject("thresholds");
  thresholds["mq2_warning"] = MQ2_THRESHOLD;
  thresholds["mq2_critical"] = MQ2_CRITICAL;
  thresholds["mq135_warning"] = MQ135_THRESHOLD;
  thresholds["mq135_critical"] = MQ135_CRITICAL;
  thresholds["temp_high"] = TEMP_THRESHOLD_HIGH;
  thresholds["temp_critical"] = TEMP_CRITICAL;
  
  // Serialize and send with DATA: prefix for ESP32 parsing
  Serial.print("DATA:");
  serializeJson(doc, Serial);
  Serial.println(); // newline as message delimiter
  dataSendCount++;
}

// Print detailed system status
void printSystemStatus() {
  Serial.println("\n--- System Status Report ---");
  Serial.print("Overall Emergency: "); Serial.println(systemEmergency ? "ACTIVE" : "Normal");
  
  // Kitchen
  Serial.print("Kitchen: ");
  if (kitchen.isEmergency) Serial.print("EMERGENCY");
  else if (kitchen.isDangerous) Serial.print("WARNING");
  else Serial.print("Safe");
  Serial.print(" | Temp: "); Serial.print(kitchen.temperature, 1);
  Serial.print("C | Humidity: "); Serial.print(kitchen.humidity, 1);
  Serial.print("% | Air Quality: "); Serial.print(kitchen.airQuality);
  Serial.print(" | Flame: "); Serial.println(kitchen.flameDetected ? "Yes" : "No");

  // Bedroom
  Serial.print("Bedroom: ");
  if (bedroom.isEmergency) Serial.print("EMERGENCY");
  else if (bedroom.isDangerous) Serial.print("WARNING");
  else Serial.print("Safe");
  Serial.print(" | Temp: "); Serial.print(bedroom.temperature, 1);
  Serial.print("C | Gas: "); Serial.print(bedroom.gasLevel);
  Serial.print(" | Flame: "); Serial.println(bedroom.flameDetected ? "Yes" : "No");

  // Parking
  Serial.print("Parking: ");
  if (parking.isEmergency) Serial.print("EMERGENCY");
  else if (parking.isDangerous) Serial.print("WARNING");
  else Serial.print("Safe");
  Serial.print(" | Gas: "); Serial.print(parking.gasLevel);
  Serial.print(" | Flame: "); Serial.println(parking.flameDetected ? "Yes" : "No");

  // Central Gas
  Serial.print("Central Gas: ");
  if (centralGas.isEmergency) Serial.print("EMERGENCY");
  else if (centralGas.isDangerous) Serial.print("WARNING");
  else Serial.print("Safe");
  Serial.print(" | Gas: "); Serial.println(centralGas.gasLevel);
  
  Serial.print("Data Packets Sent: "); Serial.println(dataSendCount);
  Serial.println("--- End of Report ---\n");
}

// Handle system-wide emergency coordination
void handleSystemEmergency() {
  static bool lastEmergencyState = false;
  
  if (systemEmergency != lastEmergencyState) {
    if (systemEmergency) {
      Serial.println("SYSTEM-WIDE EMERGENCY: At least one segment is critical.");
      Serial.println("ESP32 notified for advanced alerting.");
    } else {
      Serial.println("System emergency cleared. Returning to normal operation.");
    }
    lastEmergencyState = systemEmergency;
  }
}