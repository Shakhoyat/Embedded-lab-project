#define SENSOR_PIN 4   // Fire sensor pin

void setup() 
{
  pinMode(SENSOR_PIN, INPUT);
  Serial.begin(9600);
  Serial.println("🔥 Fire Detection System Initialized");
  Serial.println("----------------------------------");
}

void loop() 
{
  int sensorValue = digitalRead(SENSOR_PIN);

  // Print sensor reading
  if (sensorValue == LOW) {
    Serial.println("⚠️ FIRE DETECTED!");
  } 
  else {
    Serial.println("✅ No fire detected.");
  }

  delay(1000); // Delay for readability
}
