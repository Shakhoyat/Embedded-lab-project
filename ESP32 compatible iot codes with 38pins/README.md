# Intelligent Fire Alarm System - ESP32
## Complete Setup Guide

---

## 📋 Components Required

### Main Components:
1. **ESP32 Development Board** (38-pin version)
2. **MQ2 Gas/Smoke Sensor**
3. **MQ135 Air Quality Sensor**
4. **Flame Sensor Module**
5. **DS18B20 Temperature Sensor**
6. **16x2 I2C LCD Display**
7. **Active Buzzer Module** (5V)
8. **Red LED** + 220Ω Resistor
9. **Green LED** + 220Ω Resistor
10. **4.7kΩ Resistor** (for DS18B20 pull-up)
11. **Breadboard & Jumper Wires**
12. **5V Power Supply** (for sensors)

---

## 🔌 Wiring Connections

### ESP32 Pinout Reference:
```
ESP32 38-PIN Layout:
                     ┌─────────┐
                     │   USB   │
                     └─────────┘
      EN │1       38│ GND
   VP(36)│2       37│ VIN(5V)
   VN(39)│3       36│ GPIO23
   GPIO34│4   E   35│ GPIO22 (SCL)
   GPIO35│5   S   34│ GPIO1 (TX)
   GPIO32│6   P   33│ GPIO3 (RX)
   GPIO33│7   3   32│ GPIO21 (SDA)
   GPIO25│8   2   31│ GND
   GPIO26│9       30│ GPIO19
   GPIO27│10      29│ GPIO18
   GPIO14│11      28│ GPIO5
   GPIO12│12      27│ GPIO17
      GND│13      26│ GPIO16
      3V3│14      25│ GPIO4
   GPIO13│15      24│ GPIO2
      SD2│16      23│ GPIO15
      SD3│17      22│ SD0
      CMD│18      21│ CLK
     5V  │19      20│ GND
                     └─────────┘
```

### 📍 Detailed Pin Connections:

#### **1. MQ2 Smoke Sensor**
```
MQ2 Module        →    ESP32
─────────────────────────────
VCC               →    5V (VIN)
GND               →    GND
A0 (Analog Out)   →    GPIO34
```

#### **2. MQ135 Air Quality Sensor**
```
MQ135 Module      →    ESP32
─────────────────────────────
VCC               →    5V (VIN)
GND               →    GND
A0 (Analog Out)   →    GPIO35
```

#### **3. Flame Sensor**
```
Flame Module      →    ESP32
─────────────────────────────
VCC               →    5V (VIN)
GND               →    GND
A0 (Analog Out)   →    GPIO32
```

#### **4. DS18B20 Temperature Sensor**
```
DS18B20           →    ESP32
─────────────────────────────
VCC (Red)         →    3.3V
GND (Black)       →    GND
DATA (Yellow)     →    GPIO4
                       + 4.7kΩ pull-up resistor
                       between DATA and VCC
```

#### **5. 16x2 I2C LCD Display**
```
LCD Module        →    ESP32
─────────────────────────────
VCC               →    5V (VIN)
GND               →    GND
SDA               →    GPIO21 (SDA)
SCL               →    GPIO22 (SCL)
```

#### **6. Buzzer (Active)**
```
Buzzer            →    ESP32
─────────────────────────────
VCC/Signal        →    GPIO25
GND               →    GND
```

#### **7. Red LED (Alarm Indicator)**
```
Red LED           →    ESP32
─────────────────────────────
Anode (+)         →    GPIO26 → 220Ω → LED
Cathode (-)       →    GND
```

#### **8. Green LED (Safe Indicator)**
```
Green LED         →    ESP32
─────────────────────────────
Anode (+)         →    GPIO27 → 220Ω → LED
Cathode (-)       →    GND
```

---

## 📚 Required Libraries

Install these libraries via Arduino IDE Library Manager:

1. **LiquidCrystal I2C** by Frank de Brabander
   - `Sketch → Include Library → Manage Libraries`
   - Search: "LiquidCrystal I2C"
   - Install version 1.1.2 or later

2. **OneWire** by Jim Studt, Tom Pollard, etc.
   - Search: "OneWire"
   - Install latest version

3. **DallasTemperature** by Miles Burton
   - Search: "DallasTemperature"
   - Install latest version

---

## ⚙️ Arduino IDE Setup for ESP32

### 1. Install ESP32 Board Support:
```
1. Open Arduino IDE
2. Go to File → Preferences
3. In "Additional Board Manager URLs" add:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
4. Go to Tools → Board → Boards Manager
5. Search "ESP32"
6. Install "esp32 by Espressif Systems"
```

### 2. Select Board & Port:
```
Tools → Board → ESP32 Arduino → ESP32 Dev Module
Tools → Port → (Select your COM port)
Tools → Upload Speed → 115200
```

---

## 🔧 Configuration & Calibration

### 1. **I2C LCD Address**
The code defaults to `0x27`. If your LCD doesn't display anything:
```cpp
// Try changing line 47 in the code:
LiquidCrystal_I2C lcd(0x27, 16, 2);  // to
LiquidCrystal_I2C lcd(0x3F, 16, 2);
```

**Find your LCD address:**
```cpp
// Use I2C Scanner sketch to find the address
// Upload this code to ESP32:

#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("I2C Scanner");
}
void loop() {
  for(byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if(Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(i, HEX);
    }
  }
  delay(5000);
}
```

### 2. **Sensor Thresholds**
Adjust these values in the code based on your environment:

```cpp
#define MQ2_THRESHOLD       1500   // Adjust after testing
#define MQ135_THRESHOLD_PPM 1000   // Adjust after testing
#define FLAME_THRESHOLD     2000   // Lower value = more sensitive
#define TEMP_THRESHOLD      50.0   // Temperature in Celsius
```

**Calibration Steps:**
1. Upload the code
2. Open Serial Monitor (115200 baud)
3. Observe normal baseline values
4. Test with smoke/heat/gas
5. Adjust thresholds accordingly

### 3. **MQ135 R0 Calibration**
For accurate air quality readings:
```
1. Place sensor in fresh outdoor air
2. Let it warm up for 24 hours (yes, really!)
3. Read the Rs value from Serial Monitor
4. Update R0 in code (line 51):
   float R0 = 76.63;  // Replace with your Rs value
```

### 4. **DS18B20 Address** (Optional)
If you have multiple DS18B20 sensors or connection issues:
```cpp
// The code uses index-based reading (works for single sensor)
// For specific addressing, find your sensor's unique address using:

#include <OneWire.h>
OneWire oneWire(4);
void setup() {
  Serial.begin(115200);
}
void loop() {
  byte addr[8];
  if(oneWire.search(addr)) {
    Serial.print("Address: ");
    for(int i=0; i<8; i++) {
      Serial.print(addr[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  delay(1000);
}
```

---

## 🚀 How to Use

### 1. **Upload the Code:**
   - Connect ESP32 via USB
   - Open `intelligent_fire_alarm_system.ino`
   - Click Upload (➔)
   - Wait for "Done uploading"

### 2. **Power On:**
   - The system initializes for 20 seconds (sensor warm-up)
   - LCD shows "System Ready!"
   - Green LED lights up (safe state)

### 3. **Operation:**
   - **Normal Mode:** LCD rotates through sensor readings every 3 seconds
   - **Alarm Mode:** Red LED + Buzzer activate, LCD shows "ALARM!"
   - **Serial Monitor:** Shows detailed readings at 115200 baud

---

## 📊 Display Modes

The LCD automatically cycles through:
1. **MQ2 Smoke Level** (0-4095 scale)
2. **MQ135 Air Quality** (PPM)
3. **Flame Sensor** (0-4095, lower = flame)
4. **Temperature** (°C)

---

## 🔍 Troubleshooting

### Problem: LCD shows nothing
- Check I2C address (try 0x27 or 0x3F)
- Check wiring (SDA→21, SCL→22)
- Check contrast potentiometer on LCD back

### Problem: Temperature shows -127°C
- Check DS18B20 wiring
- Ensure 4.7kΩ pull-up resistor is connected
- Verify 3.3V power supply

### Problem: False alarms
- Adjust sensor thresholds
- Allow sensors to warm up fully
- Check for loose connections

### Problem: Sensors always show 0 or 4095
- Check 5V power supply for sensor modules
- Verify analog pin connections
- Test each sensor individually

### Problem: Code won't upload
- Press and hold BOOT button during upload
- Check correct COM port selected
- Try different USB cable
- Reduce upload speed to 115200

---

## 💡 Enhancement Ideas

1. **WiFi Integration:**
   ```cpp
   #include <WiFi.h>
   // Send alerts to smartphone
   ```

2. **Blynk IoT Dashboard:**
   - Monitor remotely
   - Historical data logging

3. **SD Card Data Logger:**
   - Log sensor readings to SD card

4. **Additional Sensors:**
   - CO sensor (MQ7)
   - Humidity sensor (DHT22)

5. **Relay Control:**
   - Automatically activate fire suppression
   - Cut power to equipment

---

## ⚠️ Important Notes

1. **Power Supply:** MQ sensors require 5V. ESP32 operates at 3.3V logic but has 5V tolerant ADC pins.

2. **ADC Pins:** Use ADC1 channels (GPIO32-39) to avoid WiFi conflicts.

3. **Sensor Warm-up:** MQ sensors need 24-48 hours for accurate readings initially.

4. **Safety:** This is a student project. For real fire safety, use certified equipment.

5. **Calibration:** Must be calibrated for your specific environment.

---

## 📝 Code Features

✅ Multi-sensor integration (4 sensors)  
✅ Real-time LCD display (rotating views)  
✅ Visual alarms (Red/Green LEDs)  
✅ Audio alarm (Buzzer)  
✅ Serial monitoring with detailed status  
✅ Adjustable thresholds  
✅ ESP32 optimized (3.3V logic, 12-bit ADC)  
✅ Non-blocking code  
✅ Well-commented for learning  

---

## 📧 Support

If you encounter issues:
1. Check Serial Monitor output (115200 baud)
2. Verify all wiring connections
3. Test sensors individually
4. Adjust thresholds for your environment

**Good luck with your IoT Lab Project! 🔥🚨**
