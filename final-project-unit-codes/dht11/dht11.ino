#include <DHT.h>
#include <Adafruit_Sensor.h>

// Define DHT11 pin and type
#define DHTPIN 5      // Digital pin connected to DHT11
#define DHTTYPE DHT11 // DHT 11

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.println("DHT11 Sensor Test");
  Serial.println("=================");
  
  dht.begin();
  
  // Wait for sensor to initialize
  delay(2000);
}

void loop() {
  // Wait a few seconds between measurements (DHT11 needs 2 seconds minimum)
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius
  float fahrenheit = dht.readTemperature(true); // Fahrenheit

  // Check if any reads failed and exit early (to try again)
  if (isnan(humidity) || isnan(temperature) || isnan(fahrenheit)) {
    Serial.println("ERROR: Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index (the felt temperature)
  float heatIndexC = dht.computeHeatIndex(temperature, humidity, false);
  float heatIndexF = dht.computeHeatIndex(fahrenheit, humidity, true);

  // Display results in serial monitor
  Serial.println("=== DHT11 Sensor Readings ===");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C | ");
  Serial.print(fahrenheit);
  Serial.println(" °F");
  
  Serial.print("Heat index: ");
  Serial.print(heatIndexC);
  Serial.print(" °C | ");
  Serial.print(heatIndexF);
  Serial.println(" °F");
  
  Serial.println("=============================");
  Serial.println();
}