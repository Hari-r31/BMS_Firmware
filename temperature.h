#pragma once
#include <Arduino.h>

/* ================= Pack Temperature ================= */

struct TemperatureData {
  float packTemp;          // Pack temperature in °C
  bool overTempWarning;    // Above safe threshold
};

/* ================= API ================= */

/**
 * Initialize temperature sensor
 */
void initTemperature();

/**
 * Read pack temperature
 */
float readPackTemperature();

/**
 * Legacy alias
 */
float readTemperature();

/**
 * Read structured temperature data
 */
TemperatureData readTemperatureData();

/**
 * Health check
 */
bool temperatureSystemHealthy();
