// MQ-135 Multi-Gas Air Quality Monitor
// Detects: CO2, CO, NH3, Benzene, Alcohol, Smoke

int redLed = 13;
int greenLed = 9;
int buzzer = 10;
int mq135Pin = A5;

int gasValue = 0;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(mq135Pin, INPUT);
  
  Serial.begin(9600);
  Serial.println("🌫 MQ-135 Multi-Gas Air Quality Monitor");
  delay(2000);
}

void loop() {
  gasValue = analogRead(mq135Pin);
  
  Serial.print("\nAir Quality Reading: ");
  Serial.print(gasValue);
  Serial.println(" (0-1023)");

  // Approximate air quality classification
  if (gasValue < 200) {
    Serial.println("Air Quality: GOOD ✅");
    Serial.println("Detected Gases: Normal air");
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
    digitalWrite(buzzer, LOW);
  } 
  else if (gasValue >= 200 && gasValue < 400) {
    Serial.println("Air Quality: MODERATE ⚠️");
    Serial.println("Possible Gases: Small traces of CO2 or Alcohol vapors");
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
    digitalWrite(buzzer, LOW);
  } 
  else if (gasValue >= 400 && gasValue < 600) {
    Serial.println("Air Quality: POOR 🚫");
    Serial.println("Possible Gases: CO, NH3 (Ammonia), Benzene, Smoke");
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  } 
  else {
    Serial.println("Air Quality: VERY POOR ☠️");
    Serial.println("Possible Gases: High CO2, CO, Alcohol, Smoke, LPG");
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, HIGH);
    digitalWrite(buzzer, HIGH);
  }

  delay(1500);
}

/*
🧠 NOTES:
- MQ-135 detects: CO2, CO, NH3, Benzene, Alcohol, Smoke.
- These classifications are *approximate* (based on analog value range).
- For ESP32, use 3.3V ADC and adjust value ranges (0–4095).
- For accurate PPM readings, calibration and gas charts are required.
*/
