#include <Arduino.h>
#include "voltage.h"
#include "config.h"

/* ================= Calibration ================= */

/*
 * Adjust these two constants to match your hardware:
 *   VOLTAGE_DIVIDER – the actual resistor-divider ratio feeding the ADC pin.
 *                     For a 3S pack (max ~12.6 V) into a 3.3 V ADC, a typical
 *                     divider ratio is 4:1 or 5:1.
 *   VOLTAGE_CORR    – measured trim factor to absorb component tolerances.
 */
static const float VOLTAGE_DIVIDER = 5.0f;
static const float VOLTAGE_CORR    = 1.092f;

static bool initialized = false;

/* ================= ADC helpers ================= */

static float readADCVoltage() {
  uint32_t sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += (uint32_t)analogRead(VOLTAGE_PACK_PIN);
    delayMicroseconds(80);
  }
  float avg = (float)sum / (float)ADC_SAMPLES;
  return (avg / ADC_RESOLUTION) * ADC_VREF;
}

/* ================= Public ================= */

void initVoltage() {
  if (initialized) return;

  pinMode(VOLTAGE_PACK_PIN, INPUT);
  analogSetPinAttenuation(VOLTAGE_PACK_PIN, ADC_11db);   // 0–3.3 V range

  initialized = true;
  Serial.println("[VOLTAGE] Initialized");
}

void calibrateVoltage() {
  Serial.printf("[VOLTAGE] Divider=%.2f  Correction=%.4f\n",
                VOLTAGE_DIVIDER, VOLTAGE_CORR);
}

float readPackVoltage() {
  if (!initialized) initVoltage();
  return readADCVoltage() * VOLTAGE_DIVIDER * VOLTAGE_CORR;
}

float readVoltage() { return readPackVoltage(); }

bool voltageSystemHealthy() {
  float v = readPackVoltage();
  bool  ok = (v >= (CELL_MIN_VOLTAGE * NUM_CELLS * 0.9f) &&
              v <= (CELL_MAX_VOLTAGE * NUM_CELLS * 1.1f));
  if (!ok)
    Serial.printf("[VOLTAGE] Out of range: %.2f V\n", v);
  return ok;
}
