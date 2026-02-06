#pragma once

#include <Arduino.h>

/* ================= Fault Types ================= */

enum FaultType {
  FAULT_NONE = 0,
  FAULT_OVER_VOLTAGE = 1,
  FAULT_UNDER_VOLTAGE = 2,
  FAULT_OVER_CURRENT_CHARGE = 3,
  FAULT_OVER_CURRENT_DISCHARGE = 4,
  FAULT_OVER_TEMPERATURE = 5,
  FAULT_UNDER_TEMPERATURE = 6,
  FAULT_CELL_IMBALANCE = 7,
  FAULT_SENSOR_FAILURE = 8,
  FAULT_COMMUNICATION_LOSS = 9,
  FAULT_GEOFENCE_VIOLATION = 10,
  FAULT_IMPACT_DETECTED = 11,
  FAULT_THERMAL_RUNAWAY = 12,
  FAULT_BATTERY_AGING = 13
};

/* ================= Fault Data ================= */

struct FaultData {
  bool active;
  FaultType primaryFault;
  uint32_t faultCount;
  bool latched;
  char faultMessage[64];
  unsigned long faultTimestamp;
  uint8_t severity;   // 0–4
};

/* ================= Edge Analytics ================= */

struct EdgeAnalytics {
  float voltageMovingAvg;
  float currentMovingAvg;
  float temperatureMovingAvg;
  uint8_t anomalyScore;
  bool anomalyDetected;
  bool trendWarning;
};

/* ================= API ================= */

void initFaultManager();

void evaluateSystemFaults(
  float packVoltage,
  float cellMin,
  float cellMax,
  float cellImbalance,
  float current,
  bool overcurrent,
  float tempMax,
  float tempMin
);

bool isFaulted();
bool isFaultActive(FaultType type);
const char* faultReason();
FaultData getFaultData();
uint8_t getFaultSeverity();
void clearFaults();

EdgeAnalytics performEdgeAnalytics(float voltage, float current, float temp);
EdgeAnalytics getEdgeAnalytics();

void triggerExternalFault(FaultType type, const char* message);
