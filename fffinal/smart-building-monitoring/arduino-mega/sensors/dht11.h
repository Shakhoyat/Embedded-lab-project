#ifndef DHT11_H
#define DHT11_H

#include <DHT.h>

#define DHTPIN 2          // Pin where the DHT11 is connected
#define DHTTYPE DHT11     // DHT 11

class DHT11Sensor {
  public:
    DHT11Sensor() : dht(DHTPIN, DHTTYPE) {}

    void begin() {
      dht.begin();
    }

    float readTemperature() {
      return dht.readTemperature();
    }

    float readHumidity() {
      return dht.readHumidity();
    }

    bool isValid() {
      return !isnan(readTemperature()) && !isnan(readHumidity());
    }

  private:
    DHT dht;
};

#endif // DHT11_H