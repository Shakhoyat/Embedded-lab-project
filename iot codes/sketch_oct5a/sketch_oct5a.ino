//MQ2


int Input = A0;
int SensorVal = 0;

void setup() {
  Serial.begin(9600);
  pinMode(Input, INPUT);
  Serial.println("Interfacing of Smoke Sensor with Arduino");
  Serial. println("Design by www.TheEngineeringProjects.com");
  Serial.println();
}

void loop() {

  SensorVal = analogRead(Input);
  Serial.println(SensorVal);
  delay(500);
}