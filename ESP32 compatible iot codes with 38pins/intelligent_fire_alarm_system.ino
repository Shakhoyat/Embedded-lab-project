/*
 * ========================================
 * INTELLIGENT FIRE ALARM SYSTEM - ESP32
 * ========================================
 * 
 * Sensors Used:
 * - MQ2: Smoke/Gas Detection
 * - MQ135: Air Quality (CO2)
 * - Flame Sensor: Fire Detection
 * - DS18B20: Temperature Monitoring
 * - 16x2 I2C LCD: Display
 * 
 * ESP32 Pin Configuration:
 * - MQ2 Analog: GPIO34 (ADC1_CH6)
 * - MQ135 Analog: GPIO35 (ADC1_CH7)
 * - Flame Sensor Analog: GPIO32 (ADC1_CH4)
 * - DS18B20 Data: GPIO4
 * - Buzzer: GPIO25
 * - Red LED: GPIO26
 * - Green LED: GPIO27
 * - I2C SDA: GPIO21 (default)
 * - I2C SCL: GPIO22 (default)
 * 
 * Author: IoT Lab Project
 * Date: October 2025
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ========== PIN DEFINITIONS ==========
// Analog Sensors (use ADC1 channels, avoid ADC2 when WiFi is used)
#define MQ2_PIN         34    // MQ2 Smoke sensor
#define MQ135_PIN       35    // MQ135 Air quality sensor
#define FLAME_PIN       32    // Flame sensor

// Digital Pins
#define DS18B20_PIN     4     // DS18B20 temperature sensor
#define BUZZER_PIN      25    // Buzzer
#define RED_LED_PIN     26    // Red LED (Alarm)
#define GREEN_LED_PIN   27    // Green LED (Safe)

// ========== SENSOR THRESHOLDS ==========
#define MQ2_THRESHOLD       1500   // Smoke detection threshold (adjust based on testing)
#define MQ135_THRESHOLD_PPM 1000   // Air quality threshold in PPM
#define FLAME_THRESHOLD     2000   // Flame detection threshold (lower = flame detected)
#define TEMP_THRESHOLD      50.0   // Temperature threshold in Celsius

// ========== LCD CONFIGURATION ==========
// Common I2C addresses: 0x27 or 0x3F (try both if one doesn't work)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change to 0x3F if 0x27 doesn't work

// ========== DS18B20 CONFIGURATION ==========
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

// ========== MQ135 CALIBRATION CONSTANTS ==========
float RLOAD = 10.0;      // Load resistance in kΩ
float R0 = 76.63;        // Sensor resistance in clean air (calibrate in fresh air)

// ========== GLOBAL VARIABLES ==========
bool alarmActive = false;
unsigned long lastDisplayUpdate = 0;
int displayMode = 0;     // 0=MQ2, 1=MQ135, 2=Flame, 3=Temp
unsigned long lastModeSwitch = 0;
#define DISPLAY_SWITCH_INTERVAL 3000  // Switch display every 3 seconds

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  Serial.println("\n=================================");
  Serial.println("INTELLIGENT FIRE ALARM SYSTEM");
  Serial.println("ESP32 - IoT Lab Project");
  Serial.println("=================================\n");

  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  // Initialize with safe state
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fire Alarm Sys");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // Initialize DS18B20
  tempSensor.begin();
  
  // Wait for sensors to warm up
  Serial.println("Warming up sensors (20 seconds)...");
  for (int i = 20; i > 0; i--) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nSystem Ready!\n");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  delay(2000);
}

// ========== MAIN LOOP ==========
void loop() {
  // Read all sensors
  int mq2Value = readMQ2();
  float mq135PPM = readMQ135();
  int flameValue = readFlame();
  float temperature = readTemperature();

  // Check for danger conditions
  bool smokeDanger = (mq2Value > MQ2_THRESHOLD);
  bool airQualityDanger = (mq135PPM > MQ135_THRESHOLD_PPM);
  bool flameDanger = (flameValue < FLAME_THRESHOLD);
  bool tempDanger = (temperature > TEMP_THRESHOLD && temperature != -127.0);

  alarmActive = smokeDanger || airQualityDanger || flameDanger || tempDanger;

  // Control alarm
  if (alarmActive) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    tone(BUZZER_PIN, 1000, 200);  // Beep
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    noTone(BUZZER_PIN);
  }

  // Print to Serial Monitor
  printToSerial(mq2Value, mq135PPM, flameValue, temperature, 
                smokeDanger, airQualityDanger, flameDanger, tempDanger);

  // Update LCD display (rotate through different sensors)
  updateLCD(mq2Value, mq135PPM, flameValue, temperature);

  delay(500);  // Update every 500ms
}

// ========== SENSOR READING FUNCTIONS ==========

int readMQ2() {
  // ESP32 ADC resolution is 12-bit (0-4095) by default
  int raw = analogRead(MQ2_PIN);
  return raw;
}

float readMQ135() {
  // Get averaged reading
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(MQ135_PIN);
    delay(10);
  }
  int rawValue = sum / 10;

  // Calculate PPM (approximate CO2 concentration)
  float vIn = 3.3;  // ESP32 operates at 3.3V
  float vOut = (rawValue / 4095.0) * vIn;  // 12-bit ADC
  
  if (vOut == 0) return 0;  // Avoid division by zero
  
  float rs = (vIn - vOut) * RLOAD / vOut;
  float ratio = rs / R0;
  
  // MQ-135 datasheet CO2 curve approximation
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  
  return ppm;
}

int readFlame() {
  int raw = analogRead(FLAME_PIN);
  return raw;
}

float readTemperature() {
  tempSensor.requestTemperatures();
  float tempC = tempSensor.getTempCByIndex(0);
  
  // Check if sensor is connected
  if (tempC == DEVICE_DISCONNECTED_C || tempC == -127.0) {
    return -127.0;  // Error value
  }
  
  return tempC;
}

// ========== DISPLAY FUNCTIONS ==========

void updateLCD(int mq2, float mq135, int flame, float temp) {
  unsigned long currentTime = millis();
  
  // Switch display mode every few seconds
  if (currentTime - lastModeSwitch > DISPLAY_SWITCH_INTERVAL) {
    displayMode = (displayMode + 1) % 4;
    lastModeSwitch = currentTime;
    lcd.clear();
  }

  // First line: Show alarm status or sensor name
  lcd.setCursor(0, 0);
  if (alarmActive) {
    // Flash "ALARM!" when danger detected
    if ((currentTime / 500) % 2 == 0) {
      lcd.print("*** ALARM! ***");
    } else {
      lcd.print("   DANGER!   ");
    }
  } else {
    // Show current sensor being displayed
    switch (displayMode) {
      case 0:
        lcd.print("MQ2 Smoke:");
        break;
      case 1:
        lcd.print("MQ135 AirQual:");
        break;
      case 2:
        lcd.print("Flame Sensor:");
        break;
      case 3:
        lcd.print("Temperature:");
        break;
    }
  }

  // Second line: Show sensor value
  lcd.setCursor(0, 1);
  switch (displayMode) {
    case 0:
      lcd.print(mq2);
      lcd.print(" ");
      if (mq2 > MQ2_THRESHOLD) lcd.print("HIGH!");
      else lcd.print("Safe ");
      break;
      
    case 1:
      lcd.print(mq135, 0);
      lcd.print(" PPM ");
      if (mq135 > MQ135_THRESHOLD_PPM) lcd.print("BAD!");
      else lcd.print("OK ");
      break;
      
    case 2:
      lcd.print(flame);
      lcd.print(" ");
      if (flame < FLAME_THRESHOLD) lcd.print("FIRE!");
      else lcd.print("Safe ");
      break;
      
    case 3:
      if (temp != -127.0) {
        lcd.print(temp, 1);
        lcd.print("C ");
        if (temp > TEMP_THRESHOLD) lcd.print("HOT!");
        else lcd.print("OK ");
      } else {
        lcd.print("Sensor Error");
      }
      break;
  }
}

void printToSerial(int mq2, float mq135, int flame, float temp,
                   bool smokeDanger, bool airDanger, bool flameDanger, bool tempDanger) {
  Serial.println("========================================");
  
  Serial.print("MQ2 Smoke: ");
  Serial.print(mq2);
  if (smokeDanger) Serial.println(" → DANGER! High smoke detected!");
  else Serial.println(" → Safe");
  
  Serial.print("MQ135 Air Quality: ");
  Serial.print(mq135, 1);
  Serial.print(" PPM");
  if (airDanger) Serial.println(" → DANGER! Poor air quality!");
  else Serial.println(" → Good");
  
  Serial.print("Flame Sensor: ");
  Serial.print(flame);
  if (flameDanger) Serial.println(" → DANGER! Fire detected!");
  else Serial.println(" → No flame");
  
  Serial.print("Temperature: ");
  if (temp != -127.0) {
    Serial.print(temp, 1);
    Serial.print(" °C");
    if (tempDanger) Serial.println(" → DANGER! High temperature!");
    else Serial.println(" → Normal");
  } else {
    Serial.println("ERROR - Sensor not connected!");
  }
  
  Serial.print("\n>>> SYSTEM STATUS: ");
  if (alarmActive) {
    Serial.println("⚠️  ALARM ACTIVE! ⚠️");
  } else {
    Serial.println("✅ ALL SAFE ✅");
  }
  Serial.println("========================================\n");
}
