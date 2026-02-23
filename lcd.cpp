#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include "lcd.h"
#include "config.h"
#include <string.h>
#include <math.h>

static hd44780_I2Cexp lcd;

static unsigned long lastUpdate   = 0;
static unsigned long lastRotation = 0;
static uint8_t       screenIndex  = 0;

#define LCD_UPDATE_MS    500
#define LCD_ROTATION_MS  3000

/* SOC thresholds for battery status labels */
#define SOC_LOW_THRESHOLD       20.0f
#define SOC_CRITICAL_THRESHOLD  10.0f

/* Current dead-band – below this magnitude = IDLE */
#define LCD_IDLE_THRESHOLD_A    0.15f

/* ═══════════════════════════════════════════
   FAULT CODE MAP  (full string → short LCD code)
   ═══════════════════════════════════════════ */

static const char* shortFaultCode(const char* msg) {
  if (!msg || !msg[0]) return "FAULT";
  if (strstr(msg, "OVER CURRENT CHARGE"))    return "OC CHG";
  if (strstr(msg, "OVER CURRENT DISCHARGE")) return "OC DIS";
  if (strstr(msg, "THERMAL RUNAWAY"))        return "THRM RUNAWAY";
  if (strstr(msg, "OVER TEMP"))              return "HIGH TEMP";
  if (strstr(msg, "HIGH TEMP"))              return "HIGH TEMP";
  if (strstr(msg, "UNDER TEMP"))             return "LOW TEMP";
  if (strstr(msg, "LOW TEMP"))               return "LOW TEMP";
  if (strstr(msg, "OVER VOLTAGE"))           return "OV";
  if (strstr(msg, "UNDER VOLTAGE"))          return "UV";
  if (strstr(msg, "IMPACT"))                 return "IMPACT";
  if (strstr(msg, "GEOFENCE"))               return "GEOFENCE";
  if (strstr(msg, "IMBALANCE"))              return "CELL IMBAL";
  if (strstr(msg, "AGING"))                  return "AGING";
  return "FAULT";
}

/* ═══════════════════════════════════════════
   STATUS LINE
   Purely based on live current reading (i) and SOC.
   No relay state involved at all.

   Priority (high → low):
     1. CRITICAL BATT  – SOC < 10 %
     2. COOLING FAN ON – fan running, not charging
     3. CHARGING       – current is negative (flowing into battery)
     4. DISCHARGING    – current is positive (flowing out)
     5. LOW BATTERY    – SOC < 20 % but current is idle
     6. NORMAL         – idle, SOC fine
   ═══════════════════════════════════════════ */

static const char* getStatusLine(float i, float soc, bool fanOn) {

  bool isCharging    = (i < -LCD_IDLE_THRESHOLD_A);
  bool isDischarging = (i >  LCD_IDLE_THRESHOLD_A);

  /* Critical battery beats everything except an active fault */
  if (soc <= SOC_CRITICAL_THRESHOLD && !isCharging)
    return "CRITICAL BATT";

  /* Fan active while not charging */
  if (fanOn && !isCharging)
    return "COOLING FAN ON";

  /* Current-flow based states */
  if (isCharging)
    return "CHARGING";

  if (isDischarging) {
    if (soc <= SOC_LOW_THRESHOLD)
      return "LOW BATTERY";    // discharging AND low SOC
    return "DISCHARGING";
  }

  /* Idle */
  if (soc <= SOC_CRITICAL_THRESHOLD)
    return "CRITICAL BATT";

  if (soc <= SOC_LOW_THRESHOLD)
    return "LOW BATTERY";

  return "NORMAL";
}

/* ═══════════════════════════════════════════
   INIT
   ═══════════════════════════════════════════ */

void lcdInit() {
  int rc = lcd.begin(16, 2);
  if (rc) {
    Serial.printf("[LCD] Init failed (rc=%d)\n", rc);
    return;
  }
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("BMS STARTING... ");
  lcd.setCursor(0, 1); lcd.print("PLEASE WAIT...  ");
  Serial.println("[LCD] Initialized");
}

/* ═══════════════════════════════════════════
   UPDATE
   ─────────────────────────────────────────────────────────────
   Screen rotation (no fault):

     Screen 0  – Electrical + Status
       Line 1 : V:XX.X I:X.XX T:XXC
       Line 2 : <status based on current + SOC>

     Screen 1  – Battery Intelligence
       Line 1 : SOC: XX.X %
       Line 2 : SOH: XX.X %

     Screen 2  – RUL + Fan
       Line 1 : RUL: XXXX Months
       Line 2 : FAN: ON / OFF

   Fault override (any fault latched):
       Line 1 : !! FAULT !!
       Line 2 : FAULT: <SHORT CODE>
   ═══════════════════════════════════════════ */

void lcdUpdate(float v, float i, float t,
               float soc, float soh, int rulMonths,
               bool fault, const char* faultMsg,
               bool charging, bool fanOn) {

  (void)charging;   // relay state intentionally ignored for status display

  unsigned long now = millis();
  if (now - lastUpdate < LCD_UPDATE_MS) return;
  lastUpdate = now;

  char line1[17];
  char line2[17];

  /* ── FAULT: overrides all screens ── */
  if (fault) {
    snprintf(line1, sizeof(line1), "!! FAULT !!     ");
    snprintf(line2, sizeof(line2), "FAULT: %-9s", shortFaultCode(faultMsg));
    lcd.setCursor(0, 0); lcd.print(line1);
    lcd.setCursor(0, 1); lcd.print(line2);
    return;
  }

  /* ── Rotate screens every LCD_ROTATION_MS ── */
  if (now - lastRotation > LCD_ROTATION_MS) {
    screenIndex  = (screenIndex + 1) % 3;
    lastRotation = now;
  }

  switch (screenIndex) {

    /* ── Screen 0: Electrical readings + current-based status ── */
    case 0: {
      /* "V:11.4 I: 2.5T:30" – fits 16 chars exactly */
      snprintf(line1, sizeof(line1), "V:%4.1f I:%4.1fT:%2.0f",
               v, fabsf(i) < 0.005f ? 0.0f : fabsf(i), t);

      snprintf(line2, sizeof(line2), "%-16s", getStatusLine(i, soc, fanOn));
      break;
    }

    /* ── Screen 1: SOC / SOH ── */
    case 1:
      snprintf(line1, sizeof(line1), "SOC: %5.1f %%    ", soc);
      snprintf(line2, sizeof(line2), "SOH: %5.1f %%    ", soh);
      break;

    /* ── Screen 2: RUL / Fan ── */
    case 2:
      snprintf(line1, sizeof(line1), "RUL:%4d Months ", rulMonths);
      snprintf(line2, sizeof(line2), "FAN: %-11s", fanOn ? "ON" : "OFF");
      break;
  }

  lcd.setCursor(0, 0); lcd.print(line1);
  lcd.setCursor(0, 1); lcd.print(line2);
}
