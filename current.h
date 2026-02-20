#pragma once
#include <Arduino.h>

enum CurrentDirection {
  CURRENT_IDLE = 0,
  CURRENT_CHARGING,
  CURRENT_DISCHARGING
};

struct CurrentData {
  float current;               
  CurrentDirection direction;  
  float powerWatts;

  bool overCurrent;
  bool overcurrentWarning;
  bool overcurrentFault;
};

/* ================= API ================= */

void initCurrent();
CurrentData readCurrentData();

float readCurrent();
float calculatePower(float current, float voltage);

float getPeakCurrent();
void resetPeakCurrent();

bool checkOvercurrent(float current, CurrentDirection direction);
bool currentSensorHealthy();