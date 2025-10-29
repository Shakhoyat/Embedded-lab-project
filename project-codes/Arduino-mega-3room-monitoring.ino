/*
 * Smart Building Monitoring System - Arduino Mega Controller
 * 4 Segments: Kitchen, Bedroom, Parking, Central Gas Chamber
 * 
 * Kitchen: MQ135 sensor, DHT11, Flame Sensor, Buzzer, Red+Green LED
 * Bedroom: DS18B20, MQ2, Flame Sensor, Buzzer, Red+Green LED  
 * Parking: MQ2, Flame Sensor, Red+Green LED
 * Central Gas Chamber: MQ2, Buzzer
 * 
 * JSON COMMUNICATION VIA SERIAL1 (115200 baud):
 * - INIT: Initialization complete message
 * - DATA: Complete sensor data in JSON format
 * - HEARTBEAT: Periodic status update
 * - EMERGENCY_ALERT: Emergency condition detected
 * - EMERGENCY_CLEAR: Emergency condition resolved
 * 
 * Serial (115200 baud) used for debugging output
 * Serial1 (115200 baud) used for JSON data transmission to ESP32
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
#define BUZZER_KITCHEN 5    // Kitchen buzzer
#define LED_GREEN_KITCHEN 31  // Kitchen safe indicator
#define LED_RED_KITCHEN 34    // Kitchen danger indicator

// Bedroom Sensors  
#define ONE_WIRE_BUS 23       // DS18B20 temperature sensor
#define MQ2_BEDROOM A0        // Gas/smoke sensor
#define FLAME_BEDROOM 24      // Flame sensor
#define BUZZER_BEDROOM 4    // Bedroom buzzer
#define LED_GREEN_BEDROOM 30  // Bedroom safe indicator
#define LED_RED_BEDROOM 33    // Bedroom danger indicator

// Parking Sensors
#define MQ2_PARKING A2        // Gas/smoke sensor
#define FLAME_PARKING 26      // Flame sensor
#define LED_GREEN_PARKING 32  // Parking safe indicator
#define LED_RED_PARKING 35    // Parking danger indicator

// Central Gas Chamber
#define MQ2_CENTRAL A1        // Central gas monitoring
#define BUZZER_CENTRAL 3     // Central chamber buzzer

// ========== SENSOR THRESHOLDS ==========
// Based on normal room conditions (calibrated values)
#define MQ2_THRESHOLD 350      // Gas/Smoke threshold (analog) - raised from 300
#define MQ135_THRESHOLD 450    // Air quality threshold (analog) - raised from 400
#define TEMP_THRESHOLD_HIGH 40 // High temperature alert (°C) - raised from 35
#define TEMP_THRESHOLD_LOW -5  // Low temperature alert (°C) - lowered to ignore DS18B20 error
#define HUMIDITY_HIGH 75       // High humidity alert (%) - raised from 70
#define HUMIDITY_LOW 25        // Low humidity alert (%) - lowered from 30

// Critical thresholds for immediate emergency
#define MQ2_CRITICAL 550       // Critical gas level - raised from 500
#define MQ135_CRITICAL 650     // Critical air quality - raised from 600
#define TEMP_CRITICAL 50       // Critical temperature - raised from 45

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
unsigned long lastHeartbeat = 0;
int dataSendCount = 0;
bool systemEmergency = false;
unsigned long emergencyStartTime = 0;

const unsigned long SENSOR_INTERVAL = 1000;  // Read sensors every 1 second
const unsigned long SEND_INTERVAL = 3000;    // Send data every 3 seconds
const unsigned long HEARTBEAT_INTERVAL = 10000; // Send heartbeat every 10 seconds
const unsigned long ALERT_DEBOUNCE = 5000;   // 5 second debounce for alerts

// ========== CHUNKED TRANSMISSION CONFIGURATION ==========
const int MAX_CHUNK_SIZE = 200;  // Maximum bytes per chunk
int currentChunkSequence = 0;     // Sequence number for chunks

void setup() {
  Serial.begin(115200);  // For debugging
  Serial1.begin(115200); // For JSON data transmission to ESP32
  Serial.println("Smart Building Monitoring - Arduino Mega");
  Serial.println("Initializing components...");
  Serial1.println("JSON_READY"); // Signal to ESP32 that JSON data will follow
  
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
  Serial.println("Monitoring segments and sending JSON data to ESP32 via Serial1.");
  
  // Send initialization complete message to ESP32
  sendInitializationComplete();
}

// Send initialization complete message as JSON
void sendInitializationComplete() {
  StaticJsonDocument<256> doc;
  doc["messageType"] = "INIT_COMPLETE";
  doc["timestamp"] = millis();
  doc["deviceID"] = "ARDUINO_MEGA";
  doc["status"] = "READY";
  doc["segments"] = 4;
  
  Serial1.print("INIT:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.println("Initialization message sent to ESP32 via Serial1");
}

// Send heartbeat message as JSON to ESP32
void sendHeartbeat() {
  StaticJsonDocument<256> doc;
  doc["messageType"] = "HEARTBEAT";
  doc["timestamp"] = millis();
  doc["deviceID"] = "ARDUINO_MEGA";
  doc["uptime"] = millis() / 1000;
  doc["systemEmergency"] = systemEmergency;
  doc["dataSendCount"] = dataSendCount;
  
  Serial1.print("HEARTBEAT:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.println("Heartbeat sent to ESP32 - Uptime: " + String(millis() / 1000) + "s");
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
  
  // Send heartbeat to ESP32
  if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = currentTime;
    sendHeartbeat();
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
    // Turn on RED LED immediately
    digitalWrite(LED_RED_KITCHEN, HIGH);
    digitalWrite(LED_GREEN_KITCHEN, LOW);
    
    if (kitchen.isEmergency) {
      // FIRE/EMERGENCY: Continuous loud buzzer
      if (kitchen.flameDetected) {
        // FIRE DETECTED: Rapid alternating beeps
        if ((currentTime / 300) % 2 == 0) { // Toggle every 300ms
          tone(BUZZER_KITCHEN, 2500); // Very high pitch for fire
        } else {
          tone(BUZZER_KITCHEN, 2000);
        }
      } else {
        // Other emergency (high temp/gas): Continuous high tone
        tone(BUZZER_KITCHEN, 2000);
      }
    } else {
      // WARNING: Intermittent beeps
      if ((currentTime / 1000) % 3 == 0) { // Beep once every 3 seconds
        tone(BUZZER_KITCHEN, 1000, 200);
      }
    }
  } else {
    // Safe: Green LED on, buzzer off
    digitalWrite(LED_RED_KITCHEN, LOW);
    digitalWrite(LED_GREEN_KITCHEN, HIGH);
    noTone(BUZZER_KITCHEN);
  }
  
  // Bedroom alerts and LEDs
  if (bedroom.isDangerous) {
    // Turn on RED LED immediately
    digitalWrite(LED_RED_BEDROOM, HIGH);
    digitalWrite(LED_GREEN_BEDROOM, LOW);
    
    if (bedroom.isEmergency) {
      // FIRE/EMERGENCY: Continuous loud buzzer
      if (bedroom.flameDetected) {
        // FIRE DETECTED: Rapid alternating beeps
        if ((currentTime / 300) % 2 == 0) { // Toggle every 300ms
          tone(BUZZER_BEDROOM, 2700); // Very high pitch for fire
        } else {
          tone(BUZZER_BEDROOM, 2200);
        }
      } else {
        // Other emergency (high temp/gas): Continuous high tone
        tone(BUZZER_BEDROOM, 2200);
      }
    } else {
      // WARNING: Intermittent beeps
      if ((currentTime / 1000) % 3 == 0) { // Beep once every 3 seconds
        tone(BUZZER_BEDROOM, 1200, 200);
      }
    }
  } else {
    // Safe: Green LED on, buzzer off
    digitalWrite(LED_RED_BEDROOM, LOW);
    digitalWrite(LED_GREEN_BEDROOM, HIGH);
    noTone(BUZZER_BEDROOM);
  }
  
  // Parking LEDs (no buzzer)
  if (parking.isDangerous) {
    // Turn on RED LED immediately
    digitalWrite(LED_RED_PARKING, HIGH);
    digitalWrite(LED_GREEN_PARKING, LOW);
  } else {
    // Safe: Green LED on
    digitalWrite(LED_RED_PARKING, LOW);
    digitalWrite(LED_GREEN_PARKING, HIGH);
  }
  
  // Central Gas Chamber buzzer (no LEDs)
  // Sound alarm for central gas issues OR if fire detected in parking (since parking has no buzzer)
  if (centralGas.isDangerous || parking.flameDetected) {
    if (parking.flameDetected) {
      // PARKING FIRE DETECTED: Rapid alternating beeps (parking has no buzzer, so use central)
      if ((currentTime / 300) % 2 == 0) { // Toggle every 300ms
        tone(BUZZER_CENTRAL, 3000); // Highest pitch for fire
      } else {
        tone(BUZZER_CENTRAL, 2500);
      }
    } else if (centralGas.isEmergency) {
      // GAS EMERGENCY: Continuous very high pitch
      tone(BUZZER_CENTRAL, 2800);
    } else {
      // WARNING: Intermittent beeps
      if ((currentTime / 1000) % 3 == 0) { // Beep once every 3 seconds
        tone(BUZZER_CENTRAL, 1500, 300);
      }
    }
  } else {
    // Safe: Buzzer off
    noTone(BUZZER_CENTRAL);
  }
}

// Send comprehensive JSON data to ESP32 in chunks
void sendDataToESP32() {
  // Send data in 4 separate chunks to avoid buffer overflow
  sendSystemChunk();
  delay(50); // Small delay between chunks
  
  sendKitchenChunk();
  delay(50);
  
  sendBedroomChunk();
  delay(50);
  
  sendParkingAndCentralChunk();
  delay(50);
  
  // Send completion signal
  sendDataCompleteSignal();
  
  dataSendCount++;
}

// Chunk 1: System and Environment data
void sendSystemChunk() {
  StaticJsonDocument<256> doc;
  
  doc["chunkType"] = "SYSTEM";
  doc["sequence"] = currentChunkSequence++;
  doc["timestamp"] = millis();
  doc["systemEmergency"] = systemEmergency;
  doc["emergencyDuration"] = (emergencyStartTime > 0) ? (millis() - emergencyStartTime) : 0;
  doc["globalTemp"] = globalTemperature;
  doc["globalHumidity"] = globalHumidity;
  
  Serial1.print("CHUNK:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.print("Sent SYSTEM chunk: ");
  serializeJson(doc, Serial);
  Serial.println();
}

// Chunk 2: Kitchen data
void sendKitchenChunk() {
  StaticJsonDocument<256> doc;
  
  doc["chunkType"] = "KITCHEN";
  doc["sequence"] = currentChunkSequence++;
  doc["temp"] = kitchen.temperature;
  doc["humidity"] = kitchen.humidity;
  doc["airQuality"] = kitchen.airQuality;
  doc["gasLevel"] = kitchen.gasLevel;
  doc["flame"] = kitchen.flameDetected;
  doc["emergency"] = kitchen.isEmergency;
  doc["dangerous"] = kitchen.isDangerous;
  doc["sensors"] = "DHT11,MQ135,Flame";
  doc["components"] = "Buzzer,LEDs";
  
  Serial1.print("CHUNK:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.print("Sent KITCHEN chunk: ");
  serializeJson(doc, Serial);
  Serial.println();
}

// Chunk 3: Bedroom data
void sendBedroomChunk() {
  StaticJsonDocument<256> doc;
  
  doc["chunkType"] = "BEDROOM";
  doc["sequence"] = currentChunkSequence++;
  doc["temp"] = bedroom.temperature;
  doc["gasLevel"] = bedroom.gasLevel;
  doc["flame"] = bedroom.flameDetected;
  doc["emergency"] = bedroom.isEmergency;
  doc["dangerous"] = bedroom.isDangerous;
  doc["sensors"] = "DS18B20,MQ2,Flame";
  doc["components"] = "Buzzer,LEDs";
  
  Serial1.print("CHUNK:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.print("Sent BEDROOM chunk: ");
  serializeJson(doc, Serial);
  Serial.println();
}

// Chunk 4: Parking and Central Gas data
void sendParkingAndCentralChunk() {
  StaticJsonDocument<256> doc;
  
  doc["chunkType"] = "PARKING_CENTRAL";
  doc["sequence"] = currentChunkSequence++;
  
  // Parking data
  JsonObject parking_data = doc.createNestedObject("parking");
  parking_data["gasLevel"] = parking.gasLevel;
  parking_data["flame"] = parking.flameDetected;
  parking_data["emergency"] = parking.isEmergency;
  parking_data["dangerous"] = parking.isDangerous;
  
  // Central Gas data
  JsonObject central_data = doc.createNestedObject("central");
  central_data["gasLevel"] = centralGas.gasLevel;
  central_data["emergency"] = centralGas.isEmergency;
  central_data["dangerous"] = centralGas.isDangerous;
  
  Serial1.print("CHUNK:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.print("Sent PARKING_CENTRAL chunk: ");
  serializeJson(doc, Serial);
  Serial.println();
}

// Send data completion signal
void sendDataCompleteSignal() {
  StaticJsonDocument<128> doc;
  
  doc["chunkType"] = "COMPLETE";
  doc["sequence"] = currentChunkSequence++;
  doc["totalChunks"] = 4;
  doc["dataPacketId"] = dataSendCount;
  
  Serial1.print("CHUNK:");
  serializeJson(doc, Serial1);
  Serial1.println();
  
  Serial.println("Data transmission complete");
}

// Print detailed system status to Serial (for debugging)
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
      // Send emergency alert via Serial1 as JSON
      Serial1.println("EMERGENCY_ALERT:SYSTEM_EMERGENCY_DETECTED");
    } else {
      Serial.println("System emergency cleared. Returning to normal operation.");
      // Send all-clear message via Serial1 as JSON
      Serial1.println("EMERGENCY_CLEAR:SYSTEM_NORMAL");
    }
    lastEmergencyState = systemEmergency;
  }
}