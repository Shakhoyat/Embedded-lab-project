int flameSensor = A0;
int redLed = 13;
int buzzer = 10;

int sensorValue = 0;
int threshold = 400;  // Adjust based on your testing

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(flameSensor, INPUT);
  
  Serial.begin(9600);
  Serial.println("🔥 Flame Sensor Test Initialized...");
  delay(2000);
}

void loop() {
  sensorValue = analogRead(flameSensor);
  
  Serial.print("Flame Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue < threshold) { // Flame detected
    Serial.println("🔥 Fire Detected! Alarm ON!");
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  } else {                     // No flame
    Serial.println("✅ No Flame Detected.");
    digitalWrite(redLed, LOW);
    digitalWrite(buzzer, LOW);
  }

  delay(500);
}
