# Data Communication Logic - Arduino Mega to ESP32

## Overview
This document explains how sensor data flows from Arduino Mega to ESP32 and then to Firebase.

---

## 🔄 Communication Architecture

```
┌─────────────────┐      Serial      ┌─────────────┐      WiFi/HTTPS      ┌──────────────┐
│  Arduino Mega   │ ──────────────>  │    ESP32    │  ────────────────>   │   Firebase   │
│  (Sensor Hub)   │   JSON over      │  (Gateway)  │   REST API           │  (Cloud DB)  │
└─────────────────┘   115200 baud    └─────────────┘                       └──────────────┘
```

---

## 📡 Step-by-Step Data Flow

### 1️⃣ **Arduino Mega: Data Collection & Transmission**

#### Sensors Read (Every 2 seconds)
```cpp
void readAllSensors() {
  // DHT11: Temperature & Humidity
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // DS18B20: Precise Temperature
  preciseTemp = ds18b20.getTempCByIndex(0);
  
  // Flame Sensors (3 rooms)
  bedroom.flameDetected = (digitalRead(FLAME_BEDROOM) == LOW);
  kitchen.flameDetected = (digitalRead(FLAME_KITCHEN) == LOW);
  parking.flameDetected = (digitalRead(FLAME_PARKING) == LOW);
  
  // MQ2 Gas Sensors (3 rooms)
  bedroom.gasLevel = analogRead(MQ2_BEDROOM);
  kitchen.gasLevel = analogRead(MQ2_KITCHEN);
  parking.gasLevel = analogRead(MQ2_PARKING);
  
  // MQ135 Air Quality
  airQuality = analogRead(MQ135_PIN);
}
```

#### JSON Packet Creation (Every 5 seconds)
```cpp
void sendDataToESP32() {
  // Create JSON structure
  StaticJsonDocument<512> doc;
  
  // Environmental data
  doc["temperature"] = temperature;      // DHT11 temp
  doc["humidity"] = humidity;            // DHT11 humidity
  doc["preciseTemp"] = preciseTemp;      // DS18B20 temp
  doc["airQuality"] = airQuality;        // MQ135 value
  doc["timestamp"] = millis();           // Arduino uptime
  
  // Bedroom data
  JsonObject bed = doc.createNestedObject("bedroom");
  bed["flame"] = bedroom.flameDetected;   // true/false
  bed["gas"] = bedroom.gasLevel;          // 0-1023
  bed["emergency"] = bedroom.isEmergency; // true/false
  bed["dangerous"] = bedroom.isDangerous; // true/false
  
  // Kitchen data
  JsonObject kit = doc.createNestedObject("kitchen");
  kit["flame"] = kitchen.flameDetected;
  kit["gas"] = kitchen.gasLevel;
  kit["emergency"] = kitchen.isEmergency;
  kit["dangerous"] = kitchen.isDangerous;
  
  // Parking data
  JsonObject park = doc.createNestedObject("parking");
  park["flame"] = parking.flameDetected;
  park["gas"] = parking.gasLevel;
  park["emergency"] = parking.isEmergency;
  park["dangerous"] = parking.isDangerous;
  
  // Send via Serial (115200 baud)
  serializeJson(doc, Serial);
  Serial.println(); // Newline as message delimiter
}
```

#### Example JSON Output
```json
{
  "temperature": 28.5,
  "humidity": 65.0,
  "preciseTemp": 28.3,
  "airQuality": 250,
  "timestamp": 15000,
  "bedroom": {
    "flame": false,
    "gas": 120,
    "emergency": false,
    "dangerous": false
  },
  "kitchen": {
    "flame": false,
    "gas": 180,
    "emergency": false,
    "dangerous": false
  },
  "parking": {
    "flame": false,
    "gas": 95,
    "emergency": false,
    "dangerous": false
  }
}
```

---

### 2️⃣ **ESP32: Data Reception & Processing**

#### Serial Listening (Continuous)
```cpp
void loop() {
  // Read from Serial port (connected to Arduino Mega TX)
  if (Serial.available()) {
    String incomingData = Serial.readString();
    incomingData.trim();
    
    // Check if valid JSON data
    if (incomingData.startsWith("DATA:")) {
      receivedData = incomingData.substring(5); // Remove prefix
      processArduinoData();
    } else if (incomingData.length() > 0) {
      // Handle debug messages
      Serial.println("Arduino Debug: " + incomingData);
    }
  }
}
```

#### JSON Parsing
```cpp
void processArduinoData() {
  // Parse JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, receivedData);
  
  if (error) {
    Serial.println("JSON parsing error!");
    return;
  }
  
  // Extract environmental data
  float temperature = doc["temperature"];
  float humidity = doc["humidity"];
  float preciseTemp = doc["preciseTemp"];
  int airQuality = doc["airQuality"];
  
  // Send to Firebase
  sendEnvironmentalDataToFirebase(...);
  
  // Process each room
  processRoomData("bedroom", doc["bedroom"]);
  processRoomData("kitchen", doc["kitchen"]);
  processRoomData("parking", doc["parking"]);
}
```

---

### 3️⃣ **ESP32 to Firebase: Data Upload**

#### Environmental Data Upload
```cpp
String envPath = "/smartBuilding/environmental";

Firebase.setFloat(firebaseData, envPath + "/temperature", temperature);
Firebase.setFloat(firebaseData, envPath + "/humidity", humidity);
Firebase.setFloat(firebaseData, envPath + "/preciseTemperature", preciseTemp);
Firebase.setInt(firebaseData, envPath + "/airQuality", airQuality);
Firebase.setString(firebaseData, envPath + "/lastUpdated", timeString);
```

#### Room Data Upload
```cpp
void processRoomData(String roomName, JsonObject roomData) {
  String roomPath = "/smartBuilding/rooms/" + roomName;
  
  Firebase.setBool(firebaseData, roomPath + "/flameDetected", flame);
  Firebase.setInt(firebaseData, roomPath + "/gasLevel", gas);
  Firebase.setBool(firebaseData, roomPath + "/emergency", emergency);
  Firebase.setBool(firebaseData, roomPath + "/dangerous", dangerous);
  Firebase.setString(firebaseData, roomPath + "/status", status);
  Firebase.setString(firebaseData, roomPath + "/lastUpdated", timeString);
}
```

#### Firebase Database Structure
```
smartBuilding/
├── environmental/
│   ├── temperature: 28.5
│   ├── humidity: 65.0
│   ├── preciseTemperature: 28.3
│   ├── airQuality: 250
│   └── lastUpdated: "2025-10-14 10:30:45"
│
├── rooms/
│   ├── bedroom/
│   │   ├── flameDetected: false
│   │   ├── gasLevel: 120
│   │   ├── emergency: false
│   │   ├── dangerous: false
│   │   ├── status: "SAFE"
│   │   └── lastUpdated: "2025-10-14 10:30:45"
│   │
│   ├── kitchen/
│   │   ├── flameDetected: false
│   │   ├── gasLevel: 180
│   │   ├── emergency: false
│   │   ├── dangerous: false
│   │   ├── status: "SAFE"
│   │   └── lastUpdated: "2025-10-14 10:30:45"
│   │
│   └── parking/
│       ├── flameDetected: false
│       ├── gasLevel: 95
│       ├── emergency: false
│       ├── dangerous: false
│       ├── status: "SAFE"
│       └── lastUpdated: "2025-10-14 10:30:45"
│
├── alerts/
│   └── [timestamp]/
│       ├── type: "FIRE_EMERGENCY"
│       ├── room: "kitchen"
│       ├── timestamp: "2025-10-14 10:35:12"
│       ├── message: "Fire detected in kitchen!"
│       └── acknowledged: false
│
└── system/
    ├── esp32Status: "online"
    ├── uptime: 3600
    ├── dataReceiveCount: 720
    ├── buildingStatus: "NORMAL"
    └── esp32Heartbeat: "2025-10-14 10:30:00"
```

---

## ⚡ Key Features of This Logic

### 1. **Serial Communication Protocol**
- **Baud Rate**: 115200 (fast, reliable)
- **Format**: JSON strings
- **Delimiter**: Newline character (`\n`)
- **Connection**: Arduino TX → ESP32 RX

### 2. **Data Packaging**
- **Library**: ArduinoJson (efficient serialization)
- **Size**: ~512 bytes per packet
- **Frequency**: Every 5 seconds
- **Structure**: Hierarchical JSON with nested objects

### 3. **Error Handling**
```cpp
// Arduino side
if (!isnan(temperature)) {
  // Only send valid data
}

// ESP32 side
if (error) {
  Serial.println("JSON parsing error!");
  return; // Skip invalid data
}

// Firebase side
if (Firebase.setFloat(...)) {
  // Success
} else {
  // Retry logic
  firebaseConnected = false;
}
```

### 4. **Real-time Status Detection**
```cpp
// Arduino analyzes and flags dangerous conditions
bedroom.isEmergency = bedroom.flameDetected;
bedroom.isDangerous = bedroom.isEmergency || 
                      (bedroom.gasLevel > MQ2_THRESHOLD) ||
                      (temperature > TEMP_THRESHOLD_HIGH);
```

### 5. **Emergency Alert System**
```cpp
// ESP32 monitors emergency states
if (emergency) {
  // Log to Firebase alerts
  Firebase.setString(alertPath + "/type", "FIRE_EMERGENCY");
  Firebase.setString(alertPath + "/room", roomName);
  Firebase.setString(alertPath + "/message", "Fire detected!");
  
  // Activate LED
  digitalWrite(EMERGENCY_LED, HIGH);
  
  // Send notification
  sendEmergencyNotification();
}
```

---

## 🔌 Physical Wiring

```
Arduino Mega          ESP32
─────────────────────────────
TX1 (Pin 18)    →    RX (GPIO 3)
GND             →    GND
5V              →    VIN (or use separate power)
```

**Note**: Arduino Mega operates at 5V logic, ESP32 at 3.3V. Use a level shifter or voltage divider if necessary, though direct connection often works for RX.

---

## ⏱️ Timing Diagram

```
Time (seconds)    Arduino Mega                ESP32                    Firebase
─────────────────────────────────────────────────────────────────────────────────
0                 Read sensors                 Idle                    -
2                 Read sensors                 Idle                    -
4                 Read sensors                 Idle                    -
5                 Send JSON packet   →         Receive & Parse   →     Upload data
                                                Process rooms            Update DB
                                                
7                 Read sensors                 Idle                    -
9                 Read sensors                 Idle                    -
10                Send JSON packet   →         Receive & Parse   →     Upload data
                                                Process rooms            Update DB

30                -                            Send heartbeat    →     Update system
```

---

## 📊 Data Flow Summary

1. **Arduino Mega** collects sensor data every 2 seconds
2. **Arduino Mega** packages data as JSON every 5 seconds
3. **Arduino Mega** transmits JSON via Serial at 115200 baud
4. **ESP32** receives and parses JSON immediately
5. **ESP32** uploads to Firebase via WiFi/HTTPS
6. **ESP32** sends heartbeat every 30 seconds
7. **Firebase** stores data in real-time database
8. **Web Dashboard** reads from Firebase and displays

---

## 🛠️ Why This Design?

### ✅ Advantages
- **Separation of Concerns**: Arduino handles sensors, ESP32 handles connectivity
- **Reliability**: Arduino Mega has more pins and stable analog readings
- **Scalability**: Easy to add more sensors to Arduino
- **Cost-Effective**: ESP32 only needed once, Arduino handles all sensors
- **Real-time**: Fast serial communication (115200 baud)
- **Structured Data**: JSON makes parsing easy and human-readable
- **Error Recovery**: Both sides have error handling

### 💡 Alternative Approaches (Not Used)
- **I2C/SPI**: More complex, overkill for this use case
- **Direct Firebase from Arduino**: Would need WiFi shield, more expensive
- **HTTP POST**: Serial is faster and simpler for local communication
- **Binary Protocol**: More efficient but harder to debug

---

## 🧪 Testing the Communication

### Test 1: Serial Monitor (Arduino)
```
Open Serial Monitor at 115200 baud
Should see:
- "System initialized successfully!"
- JSON packets every 5 seconds
- "JSON messages sent: X"
```

### Test 2: Serial Monitor (ESP32)
```
Open Serial Monitor at 115200 baud
Should see:
- "WiFi connected successfully!"
- "Firebase initialized successfully!"
- "📨 Received data from Arduino Mega"
- "✅ Environmental data sent to Firebase"
```

### Test 3: Firebase Console
```
Navigate to: https://console.firebase.google.com
Check Realtime Database:
- /smartBuilding/environmental (updating every 5s)
- /smartBuilding/rooms/bedroom (updating every 5s)
- /smartBuilding/system (heartbeat every 30s)
```

---

## 🐛 Troubleshooting

### Problem: ESP32 not receiving data
- Check wiring: Arduino TX → ESP32 RX
- Check GND connection
- Verify baud rate: 115200 on both sides
- Check if Arduino is sending: Open Serial Monitor

### Problem: Firebase not updating
- Check WiFi credentials
- Verify Firebase credentials (API key, database secret)
- Check database rules (should allow read/write)
- Look for error messages in ESP32 Serial Monitor

### Problem: Invalid JSON errors
- Check ArduinoJson library version (6.x)
- Verify JSON structure with online validator
- Increase JSON buffer size if needed

---

## 📝 Notes

- **Current Implementation**: Sends data every 5 seconds (good balance)
- **Can be adjusted**: Change `SEND_INTERVAL` for different frequencies
- **Buffer size**: 512 bytes sufficient for current data structure
- **Scalable**: Can add more rooms/sensors by expanding JSON

---

**Created**: October 14, 2025  
**Version**: 1.0  
**Author**: Smart Building Monitoring System
