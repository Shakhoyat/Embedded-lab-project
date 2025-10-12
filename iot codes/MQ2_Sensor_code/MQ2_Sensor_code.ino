int redLed = 13;
int greenLed = 9;
int buzzer = 10;
int smokeA0 = A5;
// Your threshold value
int sensorThres = 350;

void setup() {
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(smokeA0, INPUT);
  Serial.begin(9600);
}

void loop() {
  int analogSensor = analogRead(smokeA0);

  // Print sensor value along with status
  Serial.print("Sensor Value: ");
  Serial.print(analogSensor);
  if (analogSensor > sensorThres) {
    Serial.println(" → ABOVE THRESHOLD! Alarm ON");
    digitalWrite(redLed, HIGH);
    digitalWrite(greenLed, LOW);
    digitalWrite(buzzer, HIGH);  // Active buzzer
  } else {
    Serial.println(" → BELOW THRESHOLD. Alarm OFF");
    digitalWrite(redLed, LOW);
    digitalWrite(greenLed, HIGH);
    digitalWrite(buzzer, LOW);
  }

  delay(500); // slower prints for easier reading
}
