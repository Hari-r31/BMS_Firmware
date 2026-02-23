#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "current.h"
#include "config.h"

/* ================= Private ================= */

static Adafruit_INA219 ina219;
static bool  initialized = false;
static float peakCurrent = 0.0f;

/* Idle dead-band: currents below this are treated as IDLE */
#define IDLE_THRESHOLD_A  0.15f

/* ================= Init ================= */

void initCurrent() {
  if (initialized) return;

  if (!ina219.begin()) {
    Serial.println("[INA219] Sensor not detected – halting");
    while (1) { delay(1000); }
  }

  Serial.println("[INA219] Initialized");
  initialized = true;
}

/* ================= Read ================= */

/*
 * Sign convention (system-wide):
 *   Positive  = discharging (current flows OUT of battery to load)
 *   Negative  = charging    (current flows INTO battery from charger)
 *
 * INA219 wired with IN+ toward battery, IN- toward load/charger:
 *   - Load drawing current  → INA219 reads POSITIVE → discharging ✓
 *   - Charger pushing in    → INA219 reads NEGATIVE → charging    ✓
 *
 * Raw value is used directly – no negation needed.
 * If your hardware is wired the other way (IN+ toward load),
 * uncomment the negation line below.
 */
float readCurrent() {
  if (!initialized) initCurrent();

  float current_A = ina219.getCurrent_mA() / 1000.0f;
  // If charging and discharging are still swapped, uncomment:
  current_A = -current_A;

  float absA = fabsf(current_A);
  if (absA > peakCurrent) peakCurrent = absA;

  return current_A;
}

CurrentData readCurrentData() {
  CurrentData data = {};

  data.current = readCurrent();

  if (data.current > IDLE_THRESHOLD_A)
    data.direction = CURRENT_DISCHARGING;
  else if (data.current < -IDLE_THRESHOLD_A)
    data.direction = CURRENT_CHARGING;
  else
    data.direction = CURRENT_IDLE;

  /* INA219 power register: always positive, in mW */
  data.powerWatts = ina219.getPower_mW() / 1000.0f;

  data.overCurrent       = checkOvercurrent(data.current, data.direction);
  data.overcurrentWarning = (fabsf(data.current) > MAX_DISCHARGE_CURRENT * 0.8f);
  data.overcurrentFault   = data.overCurrent;

  return data;
}

/* ================= Utilities ================= */

float calculatePower(float current, float voltage) {
  return current * voltage;
}

float getPeakCurrent()    { return peakCurrent; }
void  resetPeakCurrent()  { peakCurrent = 0.0f; }

bool checkOvercurrent(float current, CurrentDirection direction) {
  /* Spec: overcurrent must be sustained for OVERCURRENT_DURATION_MS before
     the fault is declared (prevents single-sample spikes from tripping). */
  static unsigned long ocStartMs = 0;

  bool rawOC = false;
  if (direction == CURRENT_CHARGING)
    rawOC = fabsf(current) > MAX_CHARGE_CURRENT;
  else if (direction == CURRENT_DISCHARGING)
    rawOC = fabsf(current) > MAX_DISCHARGE_CURRENT;

  if (rawOC) {
    if (ocStartMs == 0) ocStartMs = millis();
    return (millis() - ocStartMs) >= OVERCURRENT_DURATION_MS;
  } else {
    ocStartMs = 0;   // reset timer when current returns to safe range
    return false;
  }
}

bool currentSensorHealthy() {
  float c = readCurrent();
  if (fabsf(c) > 100.0f) {
    Serial.println("[INA219] Reading out of range");
    return false;
  }
  return true;
}
