# Smart Building Monitoring System

## Overview
The Smart Building Monitoring System is designed to provide real-time monitoring and control of various environmental parameters in a building with three distinct rooms: a bedroom, a kitchen, and a parking lot. This project utilizes an Arduino Mega for sensor data collection and an ESP32 for data transmission to Firebase, enabling remote monitoring and alerts.

## Project Structure
The project is organized into several components, each responsible for specific functionalities:

- **Arduino Mega**: Handles sensor data collection and controls components like buzzers and LEDs.
  - **Sensors**: Interfaces with various sensors including DHT11, DS18B20, flame sensors, MQ2, and MQ135.
  - **Components**: Manages buzzers, LEDs, and optional LCD display for local monitoring.
  
- **ESP32**: Connects to Wi-Fi and sends data to Firebase for real-time monitoring.
  - **Wi-Fi Configuration**: Contains settings for connecting to the internet.
  - **Firebase Handler**: Manages data transmission to and from Firebase.

- **Libraries**: Includes necessary libraries for sensor and component management.

- **Configuration**: Contains pin definitions for all connected components.

## Features
1. **Modular Design**: The code is structured into separate files for sensors, components, and communication, making it easy to manage and expand.
2. **Real-time Monitoring**: Data is sent to Firebase, allowing users to monitor conditions through a web app.
3. **Alerts and Notifications**: The system sends alerts for anomalies such as high temperatures or gas leaks.
4. **User Interface**: A simple web interface displays the status of each room and allows control of LEDs.
5. **Data Logging**: Historical data is stored in Firebase for trend analysis.
6. **Safety Features**: Automatic buzzers and LED indicators guide residents in emergencies.
7. **Expandable System**: The design allows for easy addition of new sensors or features.

## Setup Instructions
1. **Hardware Requirements**:
   - Arduino Mega
   - ESP32
   - DHT11 Sensor
   - DS18B20 Sensor
   - Flame Sensor
   - MQ2 Gas Sensor
   - MQ135 Gas Sensor
   - Buzzers and LEDs
   - Jumper wires and breadboard

2. **Wiring Diagram**: Refer to the `config/pin_definitions.h` file for pin assignments and wiring instructions.

3. **Software Requirements**:
   - Arduino IDE
   - Required libraries (LiquidCrystal_I2C, DHT, OneWire, DallasTemperature)

4. **Firebase Setup**: Create a Firebase project and configure the database to store sensor data.

5. **Upload Code**: Upload the respective code to the Arduino Mega and ESP32 using the Arduino IDE.

## Conclusion
This Smart Building Monitoring System provides an efficient and user-friendly solution for monitoring environmental conditions in a building. With its modular design and real-time capabilities, it serves as a foundation for future enhancements and expansions.