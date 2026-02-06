#pragma once

#include <Arduino.h>

/* ================= Accelerometer Data Structure ================= */

struct AccelData {
  float accelX;         // X-axis acceleration (g)
  float accelY;         // Y-axis acceleration (g)
  float accelZ;         // Z-axis acceleration (g)
  float magnitude;      // Total acceleration magnitude (g)
  float vibrationRMS;   // RMS vibration level
  bool impactDetected;  // Impact event detected
  bool shockDetected;   // Severe shock detected
  uint32_t impactCount; // Total impact events since boot
  uint32_t shockCount;  // Total shock events since boot
};

/* ================= Function Prototypes ================= */

/**
 * Initialize accelerometer
 */
void initAccelerometer();

/**
 * Read current accelerometer data
 * @return Accelerometer data structure
 */
AccelData readAccelerometer();

/**
 * Get acceleration magnitude
 * @param x X-axis acceleration
 * @param y Y-axis acceleration
 * @param z Z-axis acceleration
 * @return Total magnitude in g
 */
float getAccelMagnitude(float x, float y, float z);

/**
 * Check if impact detected
 * @return True if impact threshold exceeded
 */
bool checkImpact();

/**
 * Check if severe shock detected
 * @return True if shock threshold exceeded
 */
bool checkShock();

/**
 * Get impact count since boot
 * @return Number of impacts detected
 */
uint32_t getImpactCount();

/**
 * Get shock count since boot
 * @return Number of shocks detected
 */
uint32_t getShockCount();

/**
 * Reset impact and shock counters
 */
void resetImpactCounters();

/**
 * Calculate vibration level (RMS)
 * @return RMS vibration level
 */
float calculateVibration();

/**
 * Check accelerometer health
 * @return True if sensor working
 */
bool accelerometerHealthy();

/**
 * Get orientation (tilt angle)
 * @param data Accelerometer data
 * @return Tilt angle in degrees
 */
float getTiltAngle(const AccelData& data);
