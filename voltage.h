#pragma once
#include <Arduino.h>

/* ================= Pack Voltage Monitoring ================= */

struct VoltageData {
  float packVoltage;   // Pack voltage in volts
};

/* ================= API ================= */

/**
 * Initialize voltage monitoring
 */
void initVoltage();

/**
 * Read pack voltage (scaled, real volts)
 */
float readPackVoltage();

/**
 * Legacy alias
 */
float readVoltage();

/**
 * Sensor calibration (optional)
 */
void calibrateVoltage();

/**
 * Health check
 */
bool voltageSystemHealthy();
