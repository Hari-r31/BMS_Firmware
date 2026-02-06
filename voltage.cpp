#include <Arduino.h>
#include "voltage.h"
#include "config.h"

/* ================= Private ================= */

static bool initialized = false;

/* 
 * These MUST match your real-world calibration
 * DIVIDER = effective hardware scaling
 * CORR    = ADC + tolerance correction
 */
static const float VOLTAGE_DIVIDER = 5.0f;   // effective divider ratio
static const float VOLTAGE_CORR    = 1.092f; // measured calibration factor

/* ================= ADC ================= */

static float readADCVoltage() {
  uint32_t sum = 0;

  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(VOLTAGE_PACK_PIN);
    delayMicroseconds(80);
  }

  float avg = (float)sum / ADC_SAMPLES;

  // Convert ADC counts → volts at ESP32 pin
  return (avg / ADC_RESOLUTION) * ADC_VREF;
}

/* ================= Public ================= */

void initVoltage() {
  if (initialized) return;

  pinMode(VOLTAGE_PACK_PIN, INPUT);

  // CRITICAL for ESP32 ADC accuracy
  analogSetPinAttenuation(VOLTAGE_PACK_PIN, ADC_11db);

  initialized = true;
  Serial.println("[VOLTAGE] Pack voltage monitoring initialized");
}

void calibrateVoltage() {
  // Calibration is handled by VOLTAGE_CORR
  Serial.println("[VOLTAGE] Using calibrated divider + correction");
}

float readPackVoltage() {
  if (!initialized) initVoltage();

  float v_adc = readADCVoltage();

  // Correct, real-world scaling
  float packVoltage = v_adc * VOLTAGE_DIVIDER * VOLTAGE_CORR;

  return packVoltage;
}

float readVoltage() {
  return readPackVoltage();
}

bool voltageSystemHealthy() {
  float v = readPackVoltage();

  // 3S sanity check (very wide on purpose)
  if (v < 5.0 || v > 15.0) {
    Serial.println("[VOLTAGE] Error: Voltage out of expected range");
    return false;
  }

  return true;
}
