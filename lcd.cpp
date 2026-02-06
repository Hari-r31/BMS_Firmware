#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "config.h"

static hd44780_I2Cexp lcd;
static unsigned long lastUpdate = 0;

#define LCD_UPDATE_INTERVAL_MS 500

void lcdInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);   // important for MPU6050 clones


  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("BMS STARTING...");
  lcd.setCursor(0, 1);
  lcd.print("PLEASE WAIT   ");
}

void lcdUpdate(float v, float i, float t, bool fault) {
  if (millis() - lastUpdate < LCD_UPDATE_INTERVAL_MS) return;
  lastUpdate = millis();

  char line1[17];
  char line2[17];

  if (fault) {
    // Line 1: FAULT + Voltage
    snprintf(line1, sizeof(line1), "FAULT V:%5.1f", v);

    // Line 2: Temperature
    snprintf(line2, sizeof(line2), "T:%4.0fC CHECK ", t);
  } else {
    // Normal operation
    snprintf(line1, sizeof(line1), "V:%5.1f I:%4.1f", v, i);
    snprintf(line2, sizeof(line2), "T:%4.0fC NORMAL", t);
  }

  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
