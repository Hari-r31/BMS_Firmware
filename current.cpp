#include <Arduino.h>
#include "current.h"
#include "config.h"

/* ================= Private ================= */

static bool initialized = false;
static float zeroVoltage = CURRENT_SENSOR_VREF;
static float peakCurrent = 0.0;
static unsigned long overcurrentStartTime = 0;
static bool overcurrentActive = false;

static const float SENSITIVITY = CURRENT_SENSOR_MV_PER_A / 1000.0;
static const float DEAD_BAND = 0.15;

/* ================= ADC ================= */

static float readADCVoltage() {
  uint32_t sum = 0;

  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(CURRENT_PIN);
    delayMicroseconds(80);
  }

  float avg = (float)sum / ADC_SAMPLES;
  return (avg / ADC_RESOLUTION) * ADC_VREF;
}

static float voltageToCurrent(float voltage) {
  return fabs((voltage - zeroVoltage) / SENSITIVITY);
}

/* ================= Public ================= */

void initCurrent() {
  if (initialized) return;

  pinMode(CURRENT_PIN, INPUT);
  analogSetAttenuation(ADC_11db);

  Serial.println("[CURRENT] Initializing...");
  calibrateCurrent();

  initialized = true;
}

void calibrateCurrent() {
  Serial.println("[CURRENT] Calibrating (NO LOAD)...");

  uint32_t sum = 0;
  const int samples = 1000;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(CURRENT_PIN);
    delayMicroseconds(200);
  }

  float avg = (float)sum / samples;
  zeroVoltage = (avg / ADC_RESOLUTION) * ADC_VREF;

  if (zeroVoltage < 2.0 || zeroVoltage > 3.0) {
    Serial.println("[CURRENT] Calibration out of range, using default 2.5V");
    zeroVoltage = CURRENT_SENSOR_VREF;
  }

  Serial.print("[CURRENT] Zero voltage = ");
  Serial.print(zeroVoltage, 4);
  Serial.println(" V");
}

float readCurrent() {
  if (!initialized) initCurrent();

  float voltage = readADCVoltage();
  float current = voltageToCurrent(voltage);

  if (current < DEAD_BAND) current = 0.0;
  return current;
}

CurrentData readCurrentData() {
  CurrentData data = {};

  data.current = readCurrent();
  data.direction = CURRENT_IDLE;   // MUST be set by main logic
  data.powerWatts = 0.0;

  // Instant overcurrent (direction-aware)
  data.overCurrent = checkOvercurrent(data.current, data.direction);

  // Reserved flags (not used yet)
  data.overcurrentWarning = false;
  data.overcurrentFault = false;

  if (data.current > peakCurrent) {
    peakCurrent = data.current;
  }

  return data;
}


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
    return current > MAX_CHARGE_CURRENT;
  }
  if (direction == CURRENT_DISCHARGING) {
    return current > MAX_DISCHARGE_CURRENT;
  }
  return false;
}

bool currentSensorHealthy() {
  float current = readCurrent();

  if (current > 40.0) {
    Serial.println("[CURRENT] Sensor out of range");
    return false;
  }

  if (zeroVoltage < 2.0 || zeroVoltage > 3.0) {
    Serial.println("[CURRENT] Zero drift detected");
    return false;
  }

  return true;
}
