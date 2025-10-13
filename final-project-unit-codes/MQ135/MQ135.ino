// MQ135 Exact Gas Concentrations - Serial Output
#include <math.h>

#define MQ135_PIN A3
#define RL 20.0  // Load resistance in kilo-ohms

float R0 = 28.65;  // Calibrate this value in clean air

void setup() {
  Serial.begin(9600);
  Serial.println("==========================================");
  Serial.println("      MQ135 EXACT GAS CONCENTRATIONS");
  Serial.println("==========================================");
  delay(2000);
}

float readRS() {
  int sensorValue = analogRead(MQ135_PIN);
  float VRL = sensorValue * (5.0 / 1023.0);
  float RS = (5.0 - VRL) / VRL * RL;
  return RS;
}

float calculatePPM(float ratio, float a, float b) {
  return pow(ratio / a, 1.0 / b);
}

void loop() {
  float RS = readRS();
  float ratio = RS / R0;
  
  // Calculate all gas concentrations
  float CO2 = calculatePPM(ratio, 110.47, -2.862);
  float CO = calculatePPM(ratio, 605.18, -3.937);
  float Alcohol = calculatePPM(ratio, 77.255, -3.18);
  float NH3 = calculatePPM(ratio, 102.2, -2.473);
  float Toluene = calculatePPM(ratio, 44.947, -3.445);
  float Acetone = calculatePPM(ratio, 34.668, -3.369);
  
  // SERIAL OUTPUT
  Serial.println("------------------------------------------");
  Serial.print("RAW VALUE: ");
  Serial.print(analogRead(MQ135_PIN));
  Serial.print(" | RS/R0: ");
  Serial.println(ratio, 2);
  Serial.println("------------------------------------------");
  Serial.print("CO2:       ");
  Serial.print(CO2, 1);
  Serial.println(" ppm");
  
  Serial.print("CO:        ");
  Serial.print(CO, 1);
  Serial.println(" ppm");
  
  Serial.print("Alcohol:   ");
  Serial.print(Alcohol, 1);
  Serial.println(" ppm");
  
  Serial.print("Ammonia:   ");
  Serial.print(NH3, 1);
  Serial.println(" ppm");
  
  Serial.print("Toluene:   ");
  Serial.print(Toluene, 1);
  Serial.println(" ppm");
  
  Serial.print("Acetone:   ");
  Serial.print(Acetone, 1);
  Serial.println(" ppm");
  Serial.println("==========================================");
  Serial.println();
  
  delay(3000);
}