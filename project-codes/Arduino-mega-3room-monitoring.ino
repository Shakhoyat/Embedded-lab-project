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
#define BUZZER_PARKING 29

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
#define MQ2_THRESHOLD 300      // Gas/Smoke threshold
#define MQ135_THRESHOLD 400    // Air quality threshold
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
  String roomName;
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

void setup() {
  Serial.begin(9600);
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
  
  delay(2000);
  Serial.println("System initialized successfully!");
  Serial.println("Monitoring 3 rooms: Bedroom, Kitchen, Parking Lot");
  Serial.println("=====================================");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensors every 2 seconds
  if (currentTime - lastSensorRead >= 2000) {
    readAllSensors();
    analyzeRoomSafety();
    controlAlertsAndLEDs();
    lastSensorRead = currentTime;
  }
  
  // Update LCD display every 3 seconds (cycle through rooms)
  if (currentTime - lastDisplayUpdate >= 3000) {
    updateLCDDisplay();
    lastDisplayUpdate = currentTime;
  }
  
  // Send data to ESP32 every 5 seconds
  if (currentTime - lastDataSend >= 5000) {
    sendDataToESP32();
    lastDataSend = currentTime;
  }
  
  // Print status to Serial every 5 seconds
  if (currentTime - lastDataSend >= 5000) {
    printSystemStatus();
  }
}

void readAllSensors() {
  // Read DHT11 (Temperature and Humidity)
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  // Read DS18B20 (Precise Temperature)
  ds18b20.requestTemperatures();
  preciseTemp = ds18b20.getTempCByIndex(0);
  
  // Read Flame Sensors
  bedroom.flameDetected = (digitalRead(FLAME_BEDROOM) == LOW);
  kitchen.flameDetected = (digitalRead(FLAME_KITCHEN) == LOW);
  parking.flameDetected = (digitalRead(FLAME_PARKING) == LOW);
  
  // Read MQ2 Gas Sensors
  bedroom.gasLevel = analogRead(MQ2_BEDROOM);
  kitchen.gasLevel = analogRead(MQ2_KITCHEN);
  parking.gasLevel = analogRead(MQ2_PARKING);
  
  // Read MQ135 Air Quality
  airQuality = analogRead(MQ135_PIN);
  
  // Handle sensor reading errors
  if (isnan(humidity) || isnan(temperature)) {
    humidity = -1;
    temperature = -1;
  }
  if (preciseTemp == DEVICE_DISCONNECTED_C) {
    preciseTemp = -127;
  }
}

void analyzeRoomSafety() {
  // Analyze Bedroom
  bedroom.isEmergency = bedroom.flameDetected;
  bedroom.isDangerous = (bedroom.gasLevel > MQ2_THRESHOLD) || 
                       (temperature > TEMP_THRESHOLD_HIGH) || 
                       (airQuality > MQ135_THRESHOLD);
  
  // Analyze Kitchen
  kitchen.isEmergency = kitchen.flameDetected;
  kitchen.isDangerous = (kitchen.gasLevel > MQ2_THRESHOLD) || 
                       (temperature > TEMP_THRESHOLD_HIGH) || 
                       (airQuality > MQ135_THRESHOLD);
  
  // Analyze Parking Lot
  parking.isEmergency = parking.flameDetected;
  parking.isDangerous = (parking.gasLevel > MQ2_THRESHOLD) || 
                       (temperature > TEMP_THRESHOLD_HIGH) || 
                       (airQuality > MQ135_THRESHOLD);
}

void controlAlertsAndLEDs() {
  // Control Bedroom alerts
  if (bedroom.isEmergency) {
    digitalWrite(BUZZER_BEDROOM, HIGH);
    digitalWrite(LED_RED_BEDROOM, HIGH);
    digitalWrite(LED_GREEN_BEDROOM, LOW);
  } else if (bedroom.isDangerous) {
    // Blinking red LED for warning
    digitalWrite(LED_RED_BEDROOM, !digitalRead(LED_RED_BEDROOM));
    digitalWrite(LED_GREEN_BEDROOM, LOW);
    digitalWrite(BUZZER_BEDROOM, LOW);
  } else {
    digitalWrite(BUZZER_BEDROOM, LOW);
    digitalWrite(LED_RED_BEDROOM, LOW);
    digitalWrite(LED_GREEN_BEDROOM, HIGH);
  }
  
  // Control Kitchen alerts
  if (kitchen.isEmergency) {
    digitalWrite(BUZZER_KITCHEN, HIGH);
    digitalWrite(LED_RED_KITCHEN, HIGH);
    digitalWrite(LED_GREEN_KITCHEN, LOW);
  } else if (kitchen.isDangerous) {
    digitalWrite(LED_RED_KITCHEN, !digitalRead(LED_RED_KITCHEN));
    digitalWrite(LED_GREEN_KITCHEN, LOW);
    digitalWrite(BUZZER_KITCHEN, LOW);
  } else {
    digitalWrite(BUZZER_KITCHEN, LOW);
    digitalWrite(LED_RED_KITCHEN, LOW);
    digitalWrite(LED_GREEN_KITCHEN, HIGH);
  }
  
  // Control Parking Lot alerts
  if (parking.isEmergency) {
    digitalWrite(BUZZER_PARKING, HIGH);
    digitalWrite(LED_RED_PARKING, HIGH);
    digitalWrite(LED_GREEN_PARKING, LOW);
  } else if (parking.isDangerous) {
    digitalWrite(LED_RED_PARKING, !digitalRead(LED_RED_PARKING));
    digitalWrite(LED_GREEN_PARKING, LOW);
    digitalWrite(BUZZER_PARKING, LOW);
  } else {
    digitalWrite(BUZZER_PARKING, LOW);
    digitalWrite(LED_RED_PARKING, LOW);
    digitalWrite(LED_GREEN_PARKING, HIGH);
  }
}

void updateLCDDisplay() {
  lcd.clear();
  
  switch (currentRoom) {
    case 0: // Environmental data
      lcd.setCursor(0, 0);
      lcd.print("Temp:" + String(temperature, 1) + "C H:" + String(humidity, 0) + "%");
      lcd.setCursor(0, 1);
      lcd.print("Air:" + String(airQuality) + " PT:" + String(preciseTemp, 1) + "C");
      break;
      
    case 1: // Bedroom status
      lcd.setCursor(0, 0);
      lcd.print("BEDROOM");
      lcd.setCursor(0, 1);
      if (bedroom.isEmergency) {
        lcd.print("FIRE! EVACUATE!");
      } else if (bedroom.isDangerous) {
        lcd.print("WARNING! Gas:" + String(bedroom.gasLevel));
      } else {
        lcd.print("SAFE Gas:" + String(bedroom.gasLevel));
      }
      break;
      
    case 2: // Kitchen status
      lcd.setCursor(0, 0);
      lcd.print("KITCHEN");
      lcd.setCursor(0, 1);
      if (kitchen.isEmergency) {
        lcd.print("FIRE! EVACUATE!");
      } else if (kitchen.isDangerous) {
        lcd.print("WARNING! Gas:" + String(kitchen.gasLevel));
      } else {
        lcd.print("SAFE Gas:" + String(kitchen.gasLevel));
      }
      break;
      
    case 3: // Parking Lot status
      lcd.setCursor(0, 0);
      lcd.print("PARKING LOT");
      lcd.setCursor(0, 1);
      if (parking.isEmergency) {
        lcd.print("FIRE! EVACUATE!");
      } else if (parking.isDangerous) {
        lcd.print("WARNING! Gas:" + String(parking.gasLevel));
      } else {
        lcd.print("SAFE Gas:" + String(parking.gasLevel));
      }
      break;
  }
  
  currentRoom = (currentRoom + 1) % 4;
}

void sendDataToESP32() {
  // Send JSON-formatted data to ESP32 via Serial
  Serial.print("DATA:{");
  Serial.print("\"timestamp\":" + String(millis()) + ",");
  Serial.print("\"temperature\":" + String(temperature) + ",");
  Serial.print("\"humidity\":" + String(humidity) + ",");
  Serial.print("\"preciseTemp\":" + String(preciseTemp) + ",");
  Serial.print("\"airQuality\":" + String(airQuality) + ",");
  
  // Bedroom data
  Serial.print("\"bedroom\":{");
  Serial.print("\"flame\":" + String(bedroom.flameDetected ? "true" : "false") + ",");
  Serial.print("\"gas\":" + String(bedroom.gasLevel) + ",");
  Serial.print("\"emergency\":" + String(bedroom.isEmergency ? "true" : "false") + ",");
  Serial.print("\"dangerous\":" + String(bedroom.isDangerous ? "true" : "false"));
  Serial.print("},");
  
  // Kitchen data
  Serial.print("\"kitchen\":{");
  Serial.print("\"flame\":" + String(kitchen.flameDetected ? "true" : "false") + ",");
  Serial.print("\"gas\":" + String(kitchen.gasLevel) + ",");
  Serial.print("\"emergency\":" + String(kitchen.isEmergency ? "true" : "false") + ",");
  Serial.print("\"dangerous\":" + String(kitchen.isDangerous ? "true" : "false"));
  Serial.print("},");
  
  // Parking data
  Serial.print("\"parking\":{");
  Serial.print("\"flame\":" + String(parking.flameDetected ? "true" : "false") + ",");
  Serial.print("\"gas\":" + String(parking.gasLevel) + ",");
  Serial.print("\"emergency\":" + String(parking.isEmergency ? "true" : "false") + ",");
  Serial.print("\"dangerous\":" + String(parking.isDangerous ? "true" : "false"));
  Serial.print("}");
  
  Serial.println("}");
}

void printSystemStatus() {
  Serial.println("\n=== SYSTEM STATUS ===");
  Serial.println("Environment: Temp=" + String(temperature) + "°C, Humidity=" + String(humidity) + "%, AirQ=" + String(airQuality));
  Serial.println("Precise Temp: " + String(preciseTemp) + "°C");
  
  Serial.print("Bedroom: ");
  if (bedroom.isEmergency) Serial.print("🔥 FIRE! ");
  else if (bedroom.isDangerous) Serial.print("⚠️ WARNING ");
  else Serial.print("✅ SAFE ");
  Serial.println("(Gas: " + String(bedroom.gasLevel) + ")");
  
  Serial.print("Kitchen: ");
  if (kitchen.isEmergency) Serial.print("🔥 FIRE! ");
  else if (kitchen.isDangerous) Serial.print("⚠️ WARNING ");
  else Serial.print("✅ SAFE ");
  Serial.println("(Gas: " + String(kitchen.gasLevel) + ")");
  
  Serial.print("Parking: ");
  if (parking.isEmergency) Serial.print("🔥 FIRE! ");
  else if (parking.isDangerous) Serial.print("⚠️ WARNING ");
  else Serial.print("✅ SAFE ");
  Serial.println("(Gas: " + String(parking.gasLevel) + ")");
  
  Serial.println("====================\n");
}