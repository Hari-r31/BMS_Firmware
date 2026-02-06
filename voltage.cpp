#include <Arduino.h>
#include "voltage.h"
#include "config.h"

/* ================= Private ================= */

static bool initialized = false;

/* ================= ADC ================= */

static float readADCVoltage() {
  uint32_t sum = 0;

  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(VOLTAGE_PACK_PIN);
    delayMicroseconds(80);
  }

  float avg = (float)sum / ADC_SAMPLES;
  return (avg / ADC_RESOLUTION) * ADC_VREF;
}

/* ================= Public ================= */

void initVoltage() {
  if (initialized) return;

  pinMode(VOLTAGE_PACK_PIN, INPUT);
  analogSetAttenuation(ADC_11db);

  calibrateVoltage();
  initialized = true;

  Serial.println("[VOLTAGE] Pack voltage monitoring initialized");
}

void calibrateVoltage() {
  // For direct voltage sensor modules,
  // calibration is usually a single scale factor.
  // Adjust VOLTAGE_SENSOR_MAX_VOLTAGE in config.h if needed.
  Serial.println("[VOLTAGE] Calibration using module scale factor");
}

float readPackVoltage() {
  if (!initialized) initVoltage();

  float adcVoltage = readADCVoltage();

  // Direct sensor scaling
  float packVoltage =
    (adcVoltage / ADC_VREF) * VOLTAGE_SENSOR_MAX_VOLTAGE;

  return packVoltage;
}

float readVoltage() {
  return readPackVoltage();
}

bool voltageSystemHealthy() {
  float v = readPackVoltage();

  if (v < 1.0 || v > VOLTAGE_SENSOR_MAX_VOLTAGE * 1.1) {
    Serial.println("[VOLTAGE] Error: Voltage out of expected range");
    return false;
  }

  return true;
}
