/*
 * ESP32 - Individual Sensor Test Suite
 * Use this to test each sensor one by one before running the main system
 * 
 * Uncomment the sensor you want to test
 */

// ============================================
// UNCOMMENT ONE TEST AT A TIME
// ============================================

// #define TEST_MQ2
// #define TEST_MQ135
// #define TEST_FLAME
// #define TEST_DS18B20
// #define TEST_LCD
// #define TEST_BUZZER_LEDS
#define TEST_ALL_SENSORS  // Test all sensors together

// ============================================
// LIBRARIES
// ============================================
#ifdef TEST_LCD
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27, 16, 2);  // Change to 0x3F if needed
#endif

#ifdef TEST_DS18B20
  #include <OneWire.h>
  #include <DallasTemperature.h>
  OneWire oneWire(4);
  DallasTemperature tempSensor(&oneWire);
#endif

#ifdef TEST_ALL_SENSORS
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  #include <OneWire.h>
  #include <DallasTemperature.h>
  LiquidCrystal_I2C lcd(0x27, 16, 2);
  OneWire oneWire(4);
  DallasTemperature tempSensor(&oneWire);
#endif

// ============================================
// PIN DEFINITIONS
// ============================================
#define MQ2_PIN         34
#define MQ135_PIN       35
#define FLAME_PIN       32
#define DS18B20_PIN     4
#define BUZZER_PIN      25
#define RED_LED_PIN     26
#define GREEN_LED_PIN   27

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("ESP32 SENSOR TEST SUITE");
  Serial.println("========================================\n");

  #ifdef TEST_MQ2
    Serial.println("Testing: MQ2 Smoke Sensor");
    pinMode(MQ2_PIN, INPUT);
  #endif

  #ifdef TEST_MQ135
    Serial.println("Testing: MQ135 Air Quality Sensor");
    pinMode(MQ135_PIN, INPUT);
  #endif

  #ifdef TEST_FLAME
    Serial.println("Testing: Flame Sensor");
    pinMode(FLAME_PIN, INPUT);
  #endif

  #ifdef TEST_DS18B20
    Serial.println("Testing: DS18B20 Temperature Sensor");
    tempSensor.begin();
    Serial.print("Found ");
    Serial.print(tempSensor.getDeviceCount());
    Serial.println(" DS18B20 device(s)");
  #endif

  #ifdef TEST_LCD
    Serial.println("Testing: 16x2 I2C LCD");
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LCD Test OK!");
    lcd.setCursor(0, 1);
    lcd.print("ESP32 Ready");
  #endif

  #ifdef TEST_BUZZER_LEDS
    Serial.println("Testing: Buzzer and LEDs");
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    
    // Test sequence
    Serial.println("Red LED ON");
    digitalWrite(RED_LED_PIN, HIGH);
    delay(1000);
    digitalWrite(RED_LED_PIN, LOW);
    
    Serial.println("Green LED ON");
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(1000);
    digitalWrite(GREEN_LED_PIN, LOW);
    
    Serial.println("Buzzer ON");
    tone(BUZZER_PIN, 1000);
    delay(500);
    noTone(BUZZER_PIN);
    
    Serial.println("All tests complete!");
  #endif

  #ifdef TEST_ALL_SENSORS
    Serial.println("Testing: ALL SENSORS");
    tempSensor.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("All Sensor Test");
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    digitalWrite(GREEN_LED_PIN, HIGH);
  #endif

  Serial.println("\nStarting readings...\n");
}

// ============================================
// LOOP
// ============================================
void loop() {
  
  #ifdef TEST_MQ2
    int mq2 = analogRead(MQ2_PIN);
    Serial.print("MQ2 Raw Value: ");
    Serial.println(mq2);
    delay(1000);
  #endif

  #ifdef TEST_MQ135
    int mq135 = analogRead(MQ135_PIN);
    Serial.print("MQ135 Raw Value: ");
    Serial.println(mq135);
    delay(1000);
  #endif

  #ifdef TEST_FLAME
    int flame = analogRead(FLAME_PIN);
    Serial.print("Flame Sensor Raw Value: ");
    Serial.print(flame);
    if (flame < 2000) {
      Serial.println(" → Flame Detected!");
    } else {
      Serial.println(" → No Flame");
    }
    delay(1000);
  #endif

  #ifdef TEST_DS18B20
    tempSensor.requestTemperatures();
    float temp = tempSensor.getTempCByIndex(0);
    Serial.print("Temperature: ");
    if (temp == -127.0) {
      Serial.println("ERROR - Sensor not detected!");
    } else {
      Serial.print(temp);
      Serial.println(" °C");
    }
    delay(2000);
  #endif

  #ifdef TEST_LCD
    static int counter = 0;
    lcd.setCursor(0, 1);
    lcd.print("Count: ");
    lcd.print(counter++);
    Serial.print("LCD Counter: ");
    Serial.println(counter);
    delay(1000);
  #endif

  #ifdef TEST_BUZZER_LEDS
    // No loop needed, test was in setup
    delay(5000);
  #endif

  #ifdef TEST_ALL_SENSORS
    Serial.println("========================================");
    
    // Read all sensors
    int mq2 = analogRead(MQ2_PIN);
    int mq135 = analogRead(MQ135_PIN);
    int flame = analogRead(FLAME_PIN);
    tempSensor.requestTemperatures();
    float temp = tempSensor.getTempCByIndex(0);
    
    // Print to serial
    Serial.print("MQ2 (Smoke): ");
    Serial.println(mq2);
    
    Serial.print("MQ135 (Air): ");
    Serial.println(mq135);
    
    Serial.print("Flame: ");
    Serial.print(flame);
    if (flame < 2000) Serial.println(" [DETECTED]");
    else Serial.println(" [None]");
    
    Serial.print("Temperature: ");
    if (temp != -127.0) {
      Serial.print(temp);
      Serial.println(" °C");
    } else {
      Serial.println("ERROR");
    }
    
    // Display on LCD (rotate every 2 seconds)
    static int displayMode = 0;
    static unsigned long lastSwitch = 0;
    
    if (millis() - lastSwitch > 2000) {
      displayMode = (displayMode + 1) % 4;
      lastSwitch = millis();
      lcd.clear();
    }
    
    lcd.setCursor(0, 0);
    switch(displayMode) {
      case 0:
        lcd.print("MQ2 Smoke:");
        lcd.setCursor(0, 1);
        lcd.print(mq2);
        break;
      case 1:
        lcd.print("MQ135 Air:");
        lcd.setCursor(0, 1);
        lcd.print(mq135);
        break;
      case 2:
        lcd.print("Flame Sensor:");
        lcd.setCursor(0, 1);
        lcd.print(flame);
        break;
      case 3:
        lcd.print("Temperature:");
        lcd.setCursor(0, 1);
        if (temp != -127.0) {
          lcd.print(temp, 1);
          lcd.print(" C");
        } else {
          lcd.print("Error!");
        }
        break;
    }
    
    Serial.println("========================================\n");
    delay(500);
  #endif
}

/*
 * ============================================
 * HOW TO USE THIS TEST SUITE:
 * ============================================
 * 
 * 1. Uncomment ONE test at a time (lines 9-15)
 * 2. Upload to ESP32
 * 3. Open Serial Monitor at 115200 baud
 * 4. Observe the readings
 * 5. Once verified, move to next sensor
 * 6. Finally test all sensors together
 * 
 * TROUBLESHOOTING:
 * - If MQ2/MQ135/Flame shows 0 or 4095: Check wiring and power
 * - If DS18B20 shows -127: Check pull-up resistor and connections
 * - If LCD blank: Try changing address to 0x3F (line 21)
 * - If Buzzer/LEDs don't work: Check GPIO pins and ground
 */
