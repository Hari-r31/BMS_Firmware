#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include "current.h"
#include "config.h"

/* ================= Private ================= */

static Adafruit_INA219 ina219;
static bool initialized = false;
static float peakCurrent = 0.0;

/* ================= Init ================= */

void initCurrent() {
  if (initialized) return;

  if (!ina219.begin()) {
    Serial.println("[INA219] Not detected!");
    while (1);
  }

  Serial.println("[INA219] Initialized");
  initialized = true;
}

/* ================= Read ================= */

float readCurrent() {
  if (!initialized) initCurrent();

  float current_mA = ina219.getCurrent_mA();
  float current_A = current_mA / 1000.0;

  if (fabs(current_A) > peakCurrent) {
    peakCurrent = fabs(current_A);
  }

  return current_A;
}

CurrentData readCurrentData() {
  CurrentData data = {};

  data.current = readCurrent();
  if (data.current > 0.2)
    data.direction = CURRENT_DISCHARGING;
  else if (data.current < -0.2)
    data.direction = CURRENT_CHARGING;
  else
    data.direction = CURRENT_IDLE;

  float power_mW = ina219.getPower_mW();
  data.powerWatts = power_mW / 1000.0;

  data.overCurrent = checkOvercurrent(data.current, data.direction);
  data.overcurrentWarning = false;
  data.overcurrentFault = false;

  return data;
}

/* ================= Utilities ================= */

float calculatePower(float current, float voltage) {
  return current * voltage;
}

float getPeakCurrent() {
  return peakCurrent;
}

void resetPeakCurrent() {
  peakCurrent = 0.0;
}

bool checkOvercurrent(float current, CurrentDirection direction) {
  if (direction == CURRENT_CHARGING) {
    return fabs(current) > MAX_CHARGE_CURRENT;
  }
  if (direction == CURRENT_DISCHARGING) {
    return fabs(current) > MAX_DISCHARGE_CURRENT;
  }
  return false;
}

bool currentSensorHealthy() {
  float current = readCurrent();

  if (fabs(current) > 50.0) {  // sanity limit
    Serial.println("[INA219] Sensor out of range");
    return false;
  }

  return true;
}