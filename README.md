# 🏢 Smart Building Monitoring System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Arduino](https://img.shields.io/badge/Arduino-Mega-00979D?logo=arduino)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/ESP32-Gateway-E7352C?logo=espressif)](https://www.espressif.com/)
[![React](https://img.shields.io/badge/React-18.x-61DAFB?logo=react)](https://reactjs.org/)
[![Firebase](https://img.shields.io/badge/Firebase-Realtime%20DB-FFCA28?logo=firebase)](https://firebase.google.com/)

A comprehensive IoT-based fire and gas monitoring system for smart buildings with real-time alerts, web dashboard, and automated emergency response capabilities.

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [System Architecture](#-system-architecture)
- [Hardware Components](#-hardware-components)
- [Software Stack](#-software-stack)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [API Documentation](#-api-documentation)
- [Safety Features](#-safety-features)
- [Contributing](#-contributing)
- [License](#-license)

## 🎯 Overview

The Smart Building Monitoring System is an advanced IoT solution designed to detect fire, gas leaks, and environmental hazards across multiple zones in a building. The system provides real-time monitoring, instant alerts, and a responsive web dashboard for facility managers.

### Key Highlights

- **Multi-Zone Monitoring**: 4 distinct segments (Kitchen, Bedroom, Parking, Central Gas Chamber)
- **Real-Time Alerts**: Instant notifications via web dashboard and audio alarms
- **Cloud Integration**: Firebase Realtime Database for remote monitoring
- **Emergency Response**: Automated alert system with fire department integration
- **Low Latency**: Chunked data transmission for reliable communication

## ✨ Features

### Monitoring Capabilities

- 🔥 **Fire Detection**: Flame sensors in Kitchen, Bedroom, and Parking
- 💨 **Gas Detection**: MQ2 and MQ135 sensors for smoke and hazardous gases
- 🌡️ **Temperature Monitoring**: DHT11 and DS18B20 sensors
- 💧 **Humidity Tracking**: Environmental condition monitoring
- 🎯 **Air Quality**: MQ135 sensor for air quality index

### Alert System

- 🚨 **Visual Alerts**: Red/Green LED indicators per zone
- 🔊 **Audio Alerts**: Active buzzers with distinct alarm patterns
- 📱 **Web Notifications**: Real-time dashboard updates
- 📞 **Emergency Dispatch**: Automated fire department notification system

### Dashboard Features

- 📊 Real-time sensor data visualization
- 🗺️ Multi-segment status overview
- ⚠️ Emergency overlay with priority alerts
- 📈 Historical data logging
- 🔐 Manager authentication system
- 🎨 Modern, responsive UI with TailwindCSS

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Frontend (React + Vite)                  │
│                  Real-time Dashboard & Alerts               │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      │ Firebase Realtime Database
                      │
┌─────────────────────┴───────────────────────────────────────┐
│                  ESP32 Gateway (WiFi)                       │
│            • Firebase Integration                           │
│            • Serial Communication                           │
│            • LCD Display (I2C)                              │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      │ Serial (TX1/RX1 - 115200 baud)
                      │ Chunked Data Protocol
                      │
┌─────────────────────┴───────────────────────────────────────┐
│               Arduino Mega 2560 Controller                  │
│                   Sensor Hub & Logic                        │
└─────────────────────────────────────────────────────────────┘
         │              │              │              │
    ┌────┴────┐    ┌────┴────┐   ┌────┴────┐   ┌────┴────┐
    │ Kitchen │    │ Bedroom │   │ Parking │   │Central  │
    │         │    │         │   │         │   │   Gas   │
    │ DHT11   │    │ DS18B20 │   │   MQ2   │   │   MQ2   │
    │ MQ135   │    │   MQ2   │   │  Flame  │   │ Buzzer  │
    │ Flame   │    │  Flame  │   │  LEDs   │   │         │
    │ Buzzer  │    │ Buzzer  │   │         │   │         │
    │ LEDs    │    │  LEDs   │   │         │   │         │
    └─────────┘    └─────────┘   └─────────┘   └─────────┘
```

## 🔧 Hardware Components

### Microcontrollers

| Component | Quantity | Purpose |
|-----------|----------|---------|
| Arduino Mega 2560 | 1 | Main sensor controller and logic processor |
| ESP32 DevKit | 1 | WiFi gateway and Firebase integration |

### Sensors

| Sensor | Quantity | Location | Measurement |
|--------|----------|----------|-------------|
| DHT11 | 1 | Kitchen | Temperature & Humidity |
| DS18B20 | 1 | Bedroom | Precision Temperature |
| MQ135 | 1 | Kitchen | Air Quality & Gas |
| MQ2 | 3 | Bedroom, Parking, Central | Smoke & Gas Detection |
| Flame Sensor | 3 | Kitchen, Bedroom, Parking | Fire Detection |

### Output Devices

| Device | Quantity | Location |
|--------|----------|----------|
| Active Buzzer | 3 | Kitchen, Bedroom, Central Gas |
| Red LED | 3 | Kitchen, Bedroom, Parking |
| Green LED | 3 | Kitchen, Bedroom, Parking |
| I2C LCD 16x2 | 1 | ESP32 Gateway |

### Pin Configuration

#### Arduino Mega 2560

```cpp
// Kitchen
DHT11:          Pin 22
MQ135:          A3
Flame Sensor:   Pin 25
Buzzer:         Pin 5
Green LED:      Pin 31
Red LED:        Pin 34

// Bedroom
DS18B20:        Pin 23
MQ2:            A0
Flame Sensor:   Pin 24
Buzzer:         Pin 4
Green LED:      Pin 30
Red LED:        Pin 33

// Parking
MQ2:            A2
Flame Sensor:   Pin 26
Green LED:      Pin 32
Red LED:        Pin 35

// Central Gas Chamber
MQ2:            A1
Buzzer:         Pin 3

// Communication
Serial1 (TX1/RX1): ESP32 Communication @ 115200 baud
```

#### ESP32 Gateway

```cpp
// Communication
Serial2 (RX: 16, TX: 17): Arduino Mega @ 115200 baud

// Display
I2C LCD: SDA (21), SCL (22)
```

## 💻 Software Stack

### Embedded Systems

- **Arduino IDE**: 2.x
- **ESP32 Board Support**: 2.0.x
- **Libraries**:
  - `ArduinoJson` (7.x) - JSON serialization
  - `DHT sensor library` - Temperature/humidity
  - `DallasTemperature` - DS18B20 sensor
  - `OneWire` - 1-Wire protocol
  - `Firebase ESP32 Client` - Firebase integration
  - `LiquidCrystal_I2C` - LCD display

### Frontend

- **Framework**: React 18.x with Vite
- **Styling**: TailwindCSS
- **State Management**: React Hooks
- **Database**: Firebase Realtime Database
- **Build Tool**: Vite 5.x

### Cloud Services

- **Firebase Realtime Database**: Real-time data synchronization
- **Firebase Hosting**: Web dashboard deployment

## 🚀 Installation

### Prerequisites

- Arduino IDE 2.x or higher
- Node.js 18.x or higher
- Firebase account
- USB cables for Arduino Mega and ESP32

### Hardware Setup

1. **Connect sensors to Arduino Mega** according to pin configuration
2. **Wire Arduino Mega to ESP32**:
   - Arduino TX1 (Pin 18) → ESP32 RX2 (Pin 16)
   - Arduino RX1 (Pin 19) → ESP32 TX2 (Pin 17)
   - GND → GND
3. **Connect I2C LCD to ESP32**:
   - SDA → Pin 21
   - SCL → Pin 22
   - VCC → 5V
   - GND → GND

### Arduino Mega Firmware

```bash
# 1. Open Arduino IDE
# 2. Install required libraries via Library Manager:
#    - ArduinoJson
#    - DHT sensor library
#    - DallasTemperature
#    - OneWire

# 3. Open project-codes/Arduino-mega-3room-monitoring.ino
# 4. Select Board: Arduino Mega 2560
# 5. Select correct COM port
# 6. Upload
```

### ESP32 Firmware

```bash
# 1. Install ESP32 board support in Arduino IDE
# 2. Install required libraries:
#    - Firebase ESP32 Client
#    - LiquidCrystal_I2C
#    - ArduinoJson

# 3. Configure WiFi credentials in ESP32-3room-gateway.ino:
#    - WIFI_SSID
#    - WIFI_PASSWORD

# 4. Configure Firebase credentials (see Configuration section)
# 5. Select Board: ESP32 Dev Module
# 6. Upload
```

### Frontend Setup

```bash
# Navigate to frontend directory
cd frontend

# Install dependencies
npm install

# Configure Firebase (copy your config to src/firebase/config.js)

# Development server
npm run dev

# Production build
npm run build

# Preview production build
npm run preview
```

## ⚙️ Configuration

### Firebase Configuration

1. Create a Firebase project at [Firebase Console](https://console.firebase.google.com/)
2. Enable Realtime Database
3. Set database rules for security:

```json
{
  "rules": {
    "smartBuilding": {
      ".read": true,
      ".write": true
    }
  }
}
```

4. Update Firebase config in:
   - `frontend/src/firebase/config.js`
   - ESP32 code (Firebase credentials)

### Threshold Calibration

Adjust sensor thresholds in `Arduino-mega-3room-monitoring.ino`:

```cpp
// Gas/Smoke Detection
#define MQ2_THRESHOLD 350      // Warning level
#define MQ2_CRITICAL 550       // Emergency level
#define MQ135_THRESHOLD 450    // Air quality warning
#define MQ135_CRITICAL 650     // Air quality critical

// Temperature
#define TEMP_THRESHOLD_HIGH 40 // Warning (°C)
#define TEMP_CRITICAL 50       // Emergency (°C)

// Humidity
#define HUMIDITY_HIGH 75       // High humidity (%)
#define HUMIDITY_LOW 25        // Low humidity (%)
```

## 📖 Usage

### System Startup

1. **Power on Arduino Mega** - All green LEDs should illuminate
2. **Power on ESP32** - LCD displays "Waiting for Arduino data..."
3. **Check Serial Monitor** - Verify sensor readings and communication
4. **Access Dashboard** - Navigate to `http://localhost:5173` (dev) or deployed URL

### Dashboard Interface

#### Main View
- **System Overview**: Global temperature, humidity, emergency status
- **Segment Cards**: Real-time status for each zone
- **Status Indicators**:
  - 🟢 Green: Safe
  - 🟡 Yellow: Warning
  - 🔴 Red: Emergency

#### Emergency Overlay
- Automatically appears on critical fire detection
- Displays affected segment and severity
- Options to:
  - Acknowledge alert
  - Dismiss false alarm
  - Dispatch emergency services

#### Manager Mode
- Default credentials: `manager` / `emergency123`
- Advanced notifications
- Emergency dispatch control

### Alert Behaviors

#### Fire Detection
```
Kitchen Fire Detected:
├─ Red LED: ON
├─ Green LED: OFF
├─ Buzzer: Rapid alternating beeps (2500↔2000 Hz)
└─ Dashboard: Emergency overlay + notification

Bedroom Fire Detected:
├─ Red LED: ON
├─ Green LED: OFF
├─ Buzzer: Rapid alternating beeps (2700↔2200 Hz)
└─ Dashboard: Emergency overlay + notification

Parking Fire Detected:
├─ Red LED: ON
├─ Green LED: OFF
├─ Central Gas Buzzer: Rapid beeps (3000↔2500 Hz)
└─ Dashboard: Emergency overlay + notification
```

#### Gas Detection
```
Warning Level (MQ2 > 350):
├─ Red LED: ON
├─ Buzzer: Intermittent beeps (once per 3 seconds)
└─ Dashboard: Warning status

Critical Level (MQ2 > 550):
├─ Red LED: ON
├─ Buzzer: Continuous high-pitch tone
└─ Dashboard: Emergency overlay
```

## 📡 API Documentation

### Data Transmission Protocol

The system uses a chunked data transmission protocol to handle large payloads efficiently.

#### Message Format

```
CHUNK:[JSON_DATA]
```

#### Chunk Types

1. **SYSTEM Chunk**
```json
{
  "chunkType": "SYSTEM",
  "sequence": 0,
  "timestamp": 12345,
  "systemEmergency": false,
  "emergencyDuration": 0,
  "globalTemp": 26.7,
  "globalHumidity": 63
}
```

2. **KITCHEN Chunk**
```json
{
  "chunkType": "KITCHEN",
  "sequence": 1,
  "temp": 26.7,
  "humidity": 63,
  "airQuality": 328,
  "gasLevel": 328,
  "flame": false,
  "emergency": false,
  "dangerous": false,
  "sensors": "DHT11,MQ135,Flame",
  "components": "Buzzer,LEDs"
}
```

3. **BEDROOM Chunk**
```json
{
  "chunkType": "BEDROOM",
  "sequence": 2,
  "temp": 25.5,
  "gasLevel": 269,
  "flame": false,
  "emergency": false,
  "dangerous": false,
  "sensors": "DS18B20,MQ2,Flame",
  "components": "Buzzer,LEDs"
}
```

4. **PARKING_CENTRAL Chunk**
```json
{
  "chunkType": "PARKING_CENTRAL",
  "sequence": 3,
  "parking": {
    "gasLevel": 127,
    "flame": false,
    "emergency": false,
    "dangerous": false
  },
  "central": {
    "gasLevel": 228,
    "emergency": false,
    "dangerous": false
  }
}
```

5. **COMPLETE Signal**
```json
{
  "chunkType": "COMPLETE",
  "sequence": 4,
  "totalChunks": 4,
  "dataPacketId": 156
}
```

### Firebase Database Structure

```
smartBuilding/
├── system/
│   ├── emergency: boolean
│   ├── globalTemperature: float
│   ├── globalHumidity: float
│   ├── emergencyDuration: int
│   └── lastUpdate: string
│
├── segments/
│   ├── Kitchen/
│   │   ├── status: "SAFE" | "WARNING" | "EMERGENCY"
│   │   ├── temperature: float
│   │   ├── humidity: float
│   │   ├── airQuality: int
│   │   ├── gasLevel: int
│   │   ├── flameDetected: boolean
│   │   ├── emergency: boolean
│   │   ├── dangerous: boolean
│   │   ├── sensors: string
│   │   ├── components: string
│   │   └── lastUpdate: string
│   │
│   ├── Bedroom/
│   ├── Parking/
│   └── Central_Gas/
│
└── alerts/
    └── [alertId]/
        ├── type: string
        ├── segment: string
        ├── cause: string
        ├── timestamp: string
        ├── severity: string
        └── acknowledged: boolean
```

## 🛡️ Safety Features

### Redundancy
- **Dual alert system**: Local buzzers + cloud notifications
- **Building-wide alarm**: Central gas buzzer for parking fire (no local buzzer)
- **Visual + audio alerts**: Multi-sensory warning system

### Reliability
- **Chunked transmission**: Prevents data loss from buffer overflow
- **Heartbeat monitoring**: Detects communication failures
- **Automatic reconnection**: ESP32 auto-reconnects to WiFi/Firebase

### Emergency Response
- **Instant alerts**: < 1 second from detection to alarm
- **Emergency dispatch**: Automated fire department notification
- **Manager authentication**: Controlled access to critical functions
- **False alarm handling**: Manager can dismiss non-emergencies

## 🎨 Customization

### Adding New Segments

1. **Update Arduino Code**:
```cpp
// Add sensor pins
#define NEW_SENSOR_PIN A4

// Add segment structure
SegmentData newRoom = {"NewRoom", false, 0, 0, 0, 0, false, false, true, true, 0};

// Add to readAllSensors() and analyzeSegmentSafety()
```

2. **Update ESP32 Code**:
```cpp
// Add to chunk processing
// Update Firebase paths
// Add to LCD display cycle
```

3. **Update Frontend**:
```jsx
// Add segment card in SmartBuildingMonitor.jsx
// Update segment state management
```

### Customizing Alert Thresholds

Edit values in Arduino code based on your environment:
- Calibrate with clean air baseline
- Test with smoke/gas sources
- Adjust for local climate conditions

## 🐛 Troubleshooting

### Common Issues

**Arduino not sending data**
- Check Serial1 connections (TX1/RX1)
- Verify baud rate (115200)
- Monitor Serial output for errors

**ESP32 not connecting to WiFi**
- Check SSID/password
- Verify WiFi signal strength
- Check Serial Monitor for connection status

**Sensors reading incorrect values**
- Verify pin connections
- Check sensor power supply
- Calibrate thresholds

**Buzzers not sounding**
- Confirm passive buzzers (not active)
- Check pin connections
- Test with simple digitalWrite()

**Firebase not updating**
- Verify Firebase credentials
- Check internet connection
- Review Firebase console for errors

### Debug Mode

Enable verbose logging:
```cpp
// Arduino
#define DEBUG_MODE 1

// ESP32
#define DEBUG_FIREBASE 1
```

## 📊 Performance Metrics

- **Sensor Reading Rate**: 1 Hz (every 1 second)
- **Data Transmission**: Every 3 seconds
- **Firebase Sync**: Real-time (< 1s latency)
- **Alert Response Time**: < 1 second
- **Power Consumption**: ~500mA @ 5V (Arduino + ESP32)

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👥 Authors

- **Shakhoyat** - *Initial work* - [GitHub Profile](https://github.com/Shakhoyat)

## 🙏 Acknowledgments

- Arduino and ESP32 communities
- Firebase team for excellent documentation
- Open-source library contributors
- React and Vite teams

## 📞 Support

For issues, questions, or suggestions:
- Open an issue on [GitHub](https://github.com/Shakhoyat/Embedded-lab-project/issues)
- Contact: [Your Email]

## 🗺️ Roadmap

- [ ] Mobile app (iOS/Android)
- [ ] Machine learning for predictive alerts
- [ ] Integration with smart home systems
- [ ] Historical data analytics dashboard
- [ ] Multi-building support
- [ ] SMS/Email notifications
- [ ] Voice control integration

---

**Built with ❤️ for safer smart buildings**
