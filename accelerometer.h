#pragma once
#include <Arduino.h>

/* ================= Accelerometer Data Structure ================= */

struct AccelData {
  float accelX;            // X-axis acceleration (g)
  float accelY;            // Y-axis acceleration (g)
  float accelZ;            // Z-axis acceleration (g)
  float magnitude;         // Total acceleration magnitude (g)

  bool freeFallDetected;   // Free fall detected
  bool impactDetected;     // Impact event detected
  bool shockDetected;      // Severe shock detected

  uint32_t impactCount;    // Total impact events since boot
  uint32_t shockCount;     // Total shock events since boot
};

/* ================= Function Prototypes ================= */

void initAccelerometer();
AccelData readAccelerometer();

float getAccelMagnitude(float x, float y, float z);

bool checkImpact();
bool checkShock();

uint32_t getImpactCount();
uint32_t getShockCount();

void resetImpactCounters();

bool accelerometerHealthy();
float getTiltAngle(const AccelData& data);
