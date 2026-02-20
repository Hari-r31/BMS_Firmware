#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "config.h"

static hd44780_I2Cexp lcd;
static unsigned long lastUpdate   = 0;
static unsigned long lastRotation = 0;
static uint8_t screenIndex        = 0;

#define LCD_UPDATE_INTERVAL_MS   500
#define LCD_ROTATION_INTERVAL_MS 3000   // switch info screen every 3 s

/* ─────────────────────────────────────────
   Public API
   ───────────────────────────────────────── */

void lcdInit() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("BMS STARTING...");
  lcd.setCursor(0, 1);
  lcd.print("PLEASE WAIT    ");
}

/**
 * lcdUpdate – call every loop.
 *
 * Parameters
 *   v      Pack voltage (V)
 *   i      Current (A, positive = discharge, negative = charge)
 *   t      Temperature (°C)
 *   soc    State of Charge (%)
 *   soh    State of Health (%)
 *   rul    Remaining useful life (months)
 *   fault  True if any fault latched
 *   faultMsg Short fault reason string (e.g. "OV", "OC CHG")
 *   charging True when charger relay is ON
 *   fanOn  True when cooling fan is active
 */
void lcdUpdate(float v, float i, float t,
               float soc, float soh, int rulMonths,
               bool fault, const char* faultMsg,
               bool charging, bool fanOn) {

  unsigned long now = millis();
  if (now - lastUpdate < LCD_UPDATE_INTERVAL_MS) return;
  lastUpdate = now;

  char line1[17];
  char line2[17];

  /* ── FAULT SCREEN overrides everything ── */
  if (fault) {
    snprintf(line1, sizeof(line1), "!! FAULT !!     ");

    // Build a short fault string  (max 16 chars)
    char msg[17];
    if (faultMsg && strlen(faultMsg) > 0) {
      snprintf(msg, sizeof(msg), "%-16s", faultMsg);
    } else {
      snprintf(msg, sizeof(msg), "CHECK SYSTEM    ");
    }
    snprintf(line2, sizeof(line2), "%s", msg);

    lcd.setCursor(0, 0); lcd.print(line1);
    lcd.setCursor(0, 1); lcd.print(line2);
    return;
  }

  /* ── Rotate between screens every 3 s ── */
  if (now - lastRotation > LCD_ROTATION_INTERVAL_MS) {
    screenIndex = (screenIndex + 1) % 3;
    lastRotation = now;
  }

  switch (screenIndex) {

    /* Screen 0: Voltage, Current, Temperature */
    case 0:
      // V:12.6 I: 2.5A
      snprintf(line1, sizeof(line1), "V:%5.1f I:%5.1fA", v, i);
      // T:30C  NORMAL / CHARGING / DISCHARGING
      {
        const char* mode = charging ? "CHARGING" :
                           (i > 0.3f ? "DISCHARG" : "NORMAL  ");
        snprintf(line2, sizeof(line2), "T:%3.0fC  %s", t, mode);
      }
      break;

    /* Screen 1: SOC + SOH */
    case 1:
      snprintf(line1, sizeof(line1), "SOC:%5.1f%%       ", soc);
      snprintf(line2, sizeof(line2), "SOH:%5.1f%%       ", soh);
      break;

    /* Screen 2: RUL + fan status */
    case 2:
      snprintf(line1, sizeof(line1), "RUL:%3d Months  ", rulMonths);
      snprintf(line2, sizeof(line2), "FAN:%s          ", fanOn ? "ON " : "OFF");
      break;
  }

  lcd.setCursor(0, 0); lcd.print(line1);
  lcd.setCursor(0, 1); lcd.print(line2);
}
