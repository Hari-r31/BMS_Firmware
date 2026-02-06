#pragma once
#include <Arduino.h>

/* ================= Current Monitoring ================= */

enum CurrentDirection {
  CURRENT_IDLE = 0,
  CURRENT_CHARGING,
  CURRENT_DISCHARGING
};

struct CurrentData {
  float current;               // Absolute current in Amps
  CurrentDirection direction;  // Set by MAIN logic (relay-based)
  float powerWatts;

  bool overCurrent;            // Instant overcurrent (USED by fault manager)
  bool overcurrentWarning;     // Reserved (future use)
  bool overcurrentFault;       // Reserved (future use)
};

/* ================= API ================= */

void initCurrent();
void calibrateCurrent();

float readCurrent();
CurrentData readCurrentData();

float calculatePower(float current, float voltage);

float getPeakCurrent();
void resetPeakCurrent();

bool checkOvercurrent(float current, CurrentDirection direction);
bool currentSensorHealthy();
