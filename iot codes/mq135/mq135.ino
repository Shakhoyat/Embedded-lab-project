/*
  MQ-135 Air Quality Sensor with Arduino
  Measures Air Quality (CO2 approximation) in PPM
*/

int MQ135_Pin = A0;   // Analog pin connected to MQ-135
float RLOAD = 10.0;   // Load resistance on the board (kΩ)
float R0 = 76.63;     // Sensor resistance in clean air (calibration required!)

void setup() {
  Serial.begin(9600);
  Serial.println("MQ-135 Air Quality Sensor");
  delay(2000);  // Give sensor some time to warm up
}

// Function to get an averaged ADC reading
int getAverage(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(10);
  }
  return sum / 10;
}

// Function to calculate Rs (sensor resistance)
float getResistance(int raw_adc) {
  float v_in = 5.0;
  float v_out = (raw_adc / 1023.0) * v_in;
  float rs = (v_in - v_out) * RLOAD / v_out;
  return rs;
}

// Function to estimate CO2 ppm (very approximate)
float getPPM(int raw_adc) {
  float rs = getResistance(raw_adc);
  float ratio = rs / R0;

  // MQ-135 datasheet CO2 curve (log-log approximation)
  // ppm = 116.6020682 * (ratio ^ -2.769034857)
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  return ppm;
}

void loop() {
  int rawValue = getAverage(MQ135_Pin);
  float ppmValue = getPPM(rawValue);

  Serial.print("Raw Value: ");
  Serial.print(rawValue);
  Serial.print("   CO2 PPM: ");
  Serial.println(ppmValue);

  delay(1000);
}
