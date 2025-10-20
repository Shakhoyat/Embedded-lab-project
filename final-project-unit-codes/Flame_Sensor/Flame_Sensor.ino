/*
 * Smart Building Monitoring System - Arduino Mega Code
 * 3 Rooms: Bedroom, Kitchen, Parking Lot
 * 
 * Sensors:
 * - DHT11 x1 (General temperature/humidity)
 * - DS18B20 x1 (Precision temperature monitoring)
 * - Flame Sensor x3 (One per room)
 * - MQ2 x3 (Smoke/Gas detection per room)
 * - MQ135 x1 (Air quality monitoring)
 * 
 * Components:
 * - Buzzer x3 (One per room for alerts)
 * - LEDs x6 (3 Green + 3 Red for room status)
 * - LCD Display (System status)
 */

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ArduinoJson.h>

// ========== PIN DEFINITIONS ==========
// DHT11 Sensor
#define DHT_PIN 22
#define DHT_TYPE DHT11

// DS18B20 Temperature Sensor
#define ONE_WIRE_BUS 23

// Flame Sensors (Digital pins)
#define FLAME_BEDROOM 24
#define FLAME_KITCHEN 25
#define FLAME_PARKING 26

// MQ2 Gas Sensors (Analog pins)
#define MQ2_BEDROOM A0
#define MQ2_KITCHEN A1
#define MQ2_PARKING A2

// MQ135 Air Quality Sensor
#define MQ135_PIN A3

// Buzzers
#define BUZZER_BEDROOM 27
#define BUZZER_KITCHEN 28
#define BUZZER_PARKING 3

// LEDs - Green (Safe)
#define LED_GREEN_BEDROOM 30
#define LED_GREEN_KITCHEN 31
#define LED_GREEN_PARKING 32

// LEDs - Red (Danger)
#define LED_RED_BEDROOM 33
#define LED_RED_KITCHEN 34
#define LED_RED_PARKING 35

// LCD Display (I2C)
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// ========== SENSOR THRESHOLDS ==========
#define MQ2_THRESHOLD 300      // Gas/Smoke threshold (analog)
#define MQ135_THRESHOLD 400    // Air quality threshold (analog)
#define TEMP_THRESHOLD_HIGH 35 // High temperature alert (°C)
#define TEMP_THRESHOLD_LOW 10  // Low temperature alert (°C)
#define HUMIDITY_HIGH 70       // High humidity alert (%)

// ========== SENSOR OBJECTS ==========
DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// ========== ROOM STATUS STRUCTURE ==========
struct RoomData {
  const char* roomName;
  bool flameDetected;
  int gasLevel;
  bool isEmergency;
  bool isDangerous;
};

RoomData bedroom = {"Bedroom", false, 0, false, false};
RoomData kitchen = {"Kitchen", false, 0, false, false};
RoomData parking = {"Parking", false, 0, false, false};

// ========== GLOBAL VARIABLES ==========
float temperature = 0;
float humidity = 0;
float preciseTemp = 0;
int airQuality = 0;
unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastDataSend = 0;
int currentRoom = 0; // For LCD display cycling
int dataSendCount = 0;

const unsigned long SENSOR_INTERVAL = 2000;
const unsigned long DISPLAY_INTERVAL = 3000;
const unsigned long SEND_INTERVAL = 5000;

void setup() {
  Serial.begin(115200);
  Serial.println("=== SMART BUILDING MONITORING SYSTEM ===");
  Serial.println("Initializing sensors and components...");
  
  // Initialize sensors
  dht.begin();
  ds18b20.begin();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Building");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Initialize flame sensor pins
  pinMode(FLAME_BEDROOM, INPUT);
  pinMode(FLAME_KITCHEN, INPUT);
  pinMode(FLAME_PARKING, INPUT);
  
  // Initialize buzzer pins
  pinMode(BUZZER_BEDROOM, OUTPUT);
  pinMode(BUZZER_KITCHEN, OUTPUT);
  pinMode(BUZZER_PARKING, OUTPUT);
  
  // Initialize LED pins
  pinMode(LED_GREEN_BEDROOM, OUTPUT);
  pinMode(LED_GREEN_KITCHEN, OUTPUT);
  pinMode(LED_GREEN_PARKING, OUTPUT);
  pinMode(LED_RED_BEDROOM, OUTPUT);
  pinMode(LED_RED_KITCHEN, OUTPUT);
  pinMode(LED_RED_PARKING, OUTPUT);
  
  // Turn on all green LEDs initially (safe state)
  digitalWrite(LED_GREEN_BEDROOM, HIGH);
  digitalWrite(LED_GREEN_KITCHEN, HIGH);
  digitalWrite(LED_GREEN_PARKING, HIGH);
  
  // Turn off all red LEDs and buzzers
  digitalWrite(LED_RED_BEDROOM, LOW);
  digitalWrite(LED_RED_KITCHEN, LOW);
  digitalWrite(LED_RED_PARKING, LOW);
  digitalWrite(BUZZER_BEDROOM, LOW);
  digitalWrite(BUZZER_KITCHEN, LOW);
  digitalWrite(BUZZER_PARKING, LOW);
  
  delay(1500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  lcd.setCursor(0,1);
  lcd.print("Monitoring...");
  delay(1000);
  
  Serial.println("System initialized successfully!");
  Serial.println("Monitoring 3 rooms: Bedroom, Kitchen, Parking Lot");
  Serial.println("=====================================");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentTime;
    readAllSensors();
    analyzeRoomSafety();
    controlAlertsAndLEDs();
  }
  
  if (currentTime - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = currentTime;
    updateLCDDisplay();
    currentRoom = (currentRoom + 1) % 4; // 0: env, 1: bedroom, 2: kitchen, 3: parking
  }
  
  if (currentTime - lastDataSend >= SEND_INTERVAL) {
    lastDataSend = currentTime;
    sendDataToESP32();
    printSystemStatus();
  }
}

// ====== Read sensors ======
void readAllSensors() {
  // Read DHT11 (Temperature and Humidity)
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) humidity = h;
  if (!isnan(t)) temperature = t;
  
  // Read DS18B20 (Precise Temperature)
  ds18b20.requestTemperatures();
  float dsTemp = ds18b20.getTempCByIndex(0);
  if (dsTemp != DEVICE_DISCONNECTED_C) preciseTemp = dsTemp;
  
  // Read Flame Sensors (many modules output LOW when flame detected)
  bedroom.flameDetected = (digitalRead(FLAME_BEDROOM) == LOW);
  kitchen.flameDetected = (digitalRead(FLAME_KITCHEN) == LOW);
  parking.flameDetected = (digitalRead(FLAME_PARKING) == LOW);
  
  // Read MQ2 Gas Sensors
  bedroom.gasLevel = analogRead(MQ2_BEDROOM);
  kitchen.gasLevel = analogRead(MQ2_KITCHEN);
  parking.gasLevel = analogRead(MQ2_PARKING);
  
  // Read MQ135 Air Quality
  airQuality = analogRead(MQ135_PIN);
}

// ====== Analyze safety for each room ======
void analyzeRoomSafety() {
  // Bedroom
  bedroom.isEmergency = bedroom.flameDetected;
  bedroom.isDangerous = bedroom.isEmergency ||
                         (bedroom.gasLevel > MQ2_THRESHOLD) ||
                         (temperature > TEMP_THRESHOLD_HIGH) ||
                         (airQuality > MQ135_THRESHOLD);
  // Kitchen
  kitchen.isEmergency = kitchen.flameDetected;
  kitchen.isDangerous = kitchen.isEmergency ||
                         (kitchen.gasLevel > MQ2_THRESHOLD) ||
                         (temperature > TEMP_THRESHOLD_HIGH) ||
                         (airQuality > MQ135_THRESHOLD);
  // Parking
  parking.isEmergency = parking.flameDetected;
  parking.isDangerous = parking.isEmergency ||
                         (parking.gasLevel > MQ2_THRESHOLD) ||
                         (temperature > TEMP_THRESHOLD_HIGH) ||
                         (airQuality > MQ135_THRESHOLD);
}

// ====== Control buzzers and LEDs ======
void controlAlertsAndLEDs() {
  // Bedroom LEDs / buzzer
  if (bedroom.isDangerous) {
    digitalWrite(LED_RED_BEDROOM, HIGH);
    digitalWrite(LED_GREEN_BEDROOM, LOW);
  } else {
    digitalWrite(LED_RED_BEDROOM, LOW);
    digitalWrite(LED_GREEN_BEDROOM, HIGH);
  }
  if (bedroom.isEmergency) {
    tone(BUZZER_BEDROOM, 2000, 500);
  } else {
    noTone(BUZZER_BEDROOM);
  }

  // Kitchen LEDs / buzzer
  if (kitchen.isDangerous) {
    digitalWrite(LED_RED_KITCHEN, HIGH);
    digitalWrite(LED_GREEN_KITCHEN, LOW);
  } else {
    digitalWrite(LED_RED_KITCHEN, LOW);
    digitalWrite(LED_GREEN_KITCHEN, HIGH);
  }
  if (kitchen.isEmergency) {
    tone(BUZZER_KITCHEN, 2000, 500);
  } else {
    noTone(BUZZER_KITCHEN);
  }

  // Parking LEDs / buzzer
  if (parking.isDangerous) {
    digitalWrite(LED_RED_PARKING, HIGH);
    digitalWrite(LED_GREEN_PARKING, LOW);
  } else {
    digitalWrite(LED_RED_PARKING, LOW);
    digitalWrite(LED_GREEN_PARKING, HIGH);
  }
  if (parking.isEmergency) {
    digitalWrite(BUZZER_PARKING, HIGH);
  } else {
    digitalWrite(BUZZER_PARKING, LOW);
  }
}

// ====== LCD display cycling ======
void updateLCDDisplay() {
  lcd.clear();
  switch (currentRoom) {
    case 0: // Environmental summary
      lcd.setCursor(0,0);
      lcd.print("T:");
      lcd.print(temperature,1);
      lcd.print("C H:");
      lcd.print(humidity,0);
      lcd.setCursor(0,1);
      lcd.print("AQ:");
      lcd.print(map(airQuality,0,1023,0,500)); // rough scale
      lcd.print(" DS:");
      lcd.print(preciseTemp,1);
      break;
    case 1: // Bedroom
      lcd.setCursor(0,0);
      lcd.print("Bedroom:");
      lcd.print(bedroom.flameDetected ? "FLAME" : (bedroom.isDangerous ? "DANGER" : "SAFE"));
      lcd.setCursor(0,1);
      lcd.print("G:");
      lcd.print(bedroom.gasLevel);
      break;
    case 2: // Kitchen
      lcd.setCursor(0,0);
      lcd.print("Kitchen:");
      lcd.print(kitchen.flameDetected ? "FLAME" : (kitchen.isDangerous ? "DANGER" : "SAFE"));
      lcd.setCursor(0,1);
      lcd.print("G:");
      lcd.print(kitchen.gasLevel);
      break;
    case 3: // Parking
      lcd.setCursor(0,0);
      lcd.print("Parking:");
      lcd.print(parking.flameDetected ? "FLAME" : (parking.isDangerous ? "DANGER" : "SAFE"));
      lcd.setCursor(0,1);
      lcd.print("G:");
      lcd.print(parking.gasLevel);
      break;
  }
}

// ====== Send JSON to ESP32 via Serial ======
void sendDataToESP32() {
  StaticJsonDocument<512> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["preciseTemp"] = preciseTemp;
  doc["airQuality"] = airQuality;
  doc["timestamp"] = millis();

  JsonObject bed = doc.createNestedObject("bedroom");
  bed["flame"] = bedroom.flameDetected;
  bed["gas"] = bedroom.gasLevel;
  bed["emergency"] = bedroom.isEmergency;
  bed["dangerous"] = bedroom.isDangerous;

  JsonObject kit = doc.createNestedObject("kitchen");
  kit["flame"] = kitchen.flameDetected;
  kit["gas"] = kitchen.gasLevel;
  kit["emergency"] = kitchen.isEmergency;
  kit["dangerous"] = kitchen.isDangerous;

  JsonObject park = doc.createNestedObject("parking");
  park["flame"] = parking.flameDetected;
  park["gas"] = parking.gasLevel;
  park["emergency"] = parking.isEmergency;
  park["dangerous"] = parking.isDangerous;

  // Serialize and send
  serializeJson(doc, Serial);
  Serial.println(); // newline as message delimiter
  dataSendCount++;
}

// ====== Print status to Serial (human readable) ======
void printSystemStatus() {
  Serial.println("--- Status ---");
  Serial.print("T: "); Serial.print(temperature,1); Serial.print("C  H: "); Serial.print(humidity,0); Serial.println("%");
  Serial.print("DS18B20: "); Serial.print(preciseTemp,1); Serial.println("C");
  Serial.print("AQ(MQ135): "); Serial.println(airQuality);
  Serial.print("Bedroom - Flame: "); Serial.print(bedroom.flameDetected); Serial.print(" Gas: "); Serial.print(bedroom.gasLevel); Serial.print(" Danger: "); Serial.println(bedroom.isDangerous);
  Serial.print("Kitchen  - Flame: "); Serial.print(kitchen.flameDetected); Serial.print(" Gas: "); Serial.print(kitchen.gasLevel); Serial.print(" Danger: "); Serial.println(kitchen.isDangerous);
  Serial.print("Parking  - Flame: "); Serial.print(parking.flameDetected); Serial.print(" Gas: "); Serial.print(parking.gasLevel); Serial.print(" Danger: "); Serial.println(parking.isDangerous);
  Serial.print("JSON messages sent: "); Serial.println(dataSendCount);
  Serial.println("---------------");
}