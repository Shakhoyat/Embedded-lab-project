#ifndef MQ135_H
#define MQ135_H

#include <Arduino.h>

class MQ135 {
public:
    MQ135(int pin);
    void begin();
    float readAirQuality();
    bool isGasDetected(float threshold);

private:
    int _pin;
    float _calibrationFactor;
};

#endif // MQ135_H