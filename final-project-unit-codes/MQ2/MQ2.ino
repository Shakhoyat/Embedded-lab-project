// Single MQ2 Gas Sensor Test
// Connect MQ2 to analog pin A0

#define MQ2_PIN A2

void setup() {
  Serial.begin(9600);
  Serial.println("MQ2 Gas Sensor Test");
  Serial.println("===================");
  Serial.println("Waiting for sensor to warm up...");
  
  // Warm up the sensor (MQ sensors need 1-2 minutes to warm up)
  for(int i = 20; i > 0; i--) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.println("Sensor ready!");
  Serial.println();
}

void loop() {
  // Read the analog value
  int sensorValue = analogRead(MQ2_PIN);
  
  // Display the reading
  Serial.print("MQ2 Raw Value: ");
  Serial.print(sensorValue);
  
  // Interpret the reading
  Serial.print(" | Status: ");
  if (sensorValue < 200) {
    Serial.println("Clean Air");
  } else if (sensorValue < 400) {
    Serial.println("Low Gas");
  } else if (sensorValue < 600) {
    Serial.println("Medium Gas");
  } else {
    Serial.println("HIGH GAS - DANGER!");
  }
  
  delay(1000); // Wait 1 second between readings
}