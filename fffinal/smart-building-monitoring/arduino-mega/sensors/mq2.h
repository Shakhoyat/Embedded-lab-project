// mq2.h
#ifndef MQ2_H
#define MQ2_H

#include <Arduino.h>

class MQ2 {
public:
    MQ2(int pin);
    void begin();
    float readGasConcentration();
    bool isGasDetected(float threshold);

private:
    int _pin;
};

#endif // MQ2_H