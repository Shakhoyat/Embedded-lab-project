#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class LCDDisplay {
public:
    LCDDisplay(uint8_t address, uint8_t cols, uint8_t rows) : lcd(address, cols, rows) {
        lcd.init();
        lcd.backlight();
    }

    void displayWelcomeMessage() {
        lcd.setCursor(0, 0);
        lcd.print("Smart Building");
        lcd.setCursor(0, 1);
        lcd.print("Monitoring Sys");
    }

    void updateSensorData(float temperature, float humidity, float gasLevel) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature);
        lcd.print(" C");
        
        lcd.setCursor(0, 1);
        lcd.print("Humidity: ");
        lcd.print(humidity);
        lcd.print(" %");
        
        lcd.setCursor(0, 2);
        lcd.print("Gas Level: ");
        lcd.print(gasLevel);
        lcd.print(" ppm");
    }

    void displayAlert(const char* message) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ALERT!");
        lcd.setCursor(0, 1);
        lcd.print(message);
    }

private:
    LiquidCrystal_I2C lcd;
};

#endif // LCD_DISPLAY_H