#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
public:
    Buzzer(int pin);
    void begin();
    void activate();
    void deactivate();
    void alert(int duration);

private:
    int buzzerPin;
};

#endif // BUZZER_H