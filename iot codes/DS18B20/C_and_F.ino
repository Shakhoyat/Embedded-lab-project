// #include <OneWire.h>

// #define ONE_WIRE_BUS 8
// OneWire oneWire(ONE_WIRE_BUS);

// void setup() {
//   Serial.begin(9600);
//   Serial.println("Searching for DS18B20...");
// }

// void loop() {
//   byte address[8];

//   if (oneWire.search(address)) {
//     Serial.print("Found device with address: ");
//     for (int i = 0; i < 8; i++) {
//       Serial.print(address[i], HEX);
//       Serial.print(" ");
//     }
//     Serial.println();
//   } else {
//     Serial.println("No DS18B20 detected!");
//     oneWire.reset_search();
//     delay(1000);
//   }
// }




// #include <OneWire.h>
// #include <DallasTemperature.h>

// #define ONE_WIRE_BUS 8

// OneWire oneWire(ONE_WIRE_BUS);

// DallasTemperature sensors(&oneWire);

// float Celsius = 0;
// float Fahrenheit = 0;

// void setup() {
//   sensors.begin();
//   Serial.begin(9600);
// }

// void loop() {
//   sensors.requestTemperatures();

//   Celsius = sensors.getTempCByIndex(0);
//   Fahrenheit = sensors.toFahrenheit(Celsius);

//   Serial.print(Celsius);
//   Serial.print(" C  ");
//   Serial.print(Fahrenheit);
//   Serial.println(" F");

//   delay(1000);
// }


#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 8

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Device address of your DS18B20:
DeviceAddress sensorAddress = {0x28, 0x4B, 0x3B, 0x6B, 0x00, 0x00, 0x00, 0x63};

void setup() {
  Serial.begin(9600);
  sensors.begin();
  Serial.println("DS18B20 Temperature Sensor Ready");
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(sensorAddress);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: DS18B20 not detected!");
  } else {
    float tempF = sensors.toFahrenheit(tempC);
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print(" °C / ");
    Serial.print(tempF);
    Serial.println(" °F");
  }

  delay(1000);
}

