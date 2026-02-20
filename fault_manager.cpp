#include "fault_manager.h"
#include "config.h"
#include "gsm_sms.h"
#include "nvs_logger.h"
#include <string.h>
#include <math.h>

/* ================= Relay Config ================= */

#define LOAD_RELAY_PIN 33

// ⚠️ Change if your relay is active LOW
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

/* ================= State ================= */

static FaultData currentFault;
static EdgeAnalytics analytics;
static bool initialized = false;

static uint32_t faultBitmap = 0;

/* ================= Edge Windows ================= */

#define WINDOW_SIZE 10
static float voltageWindow[WINDOW_SIZE];
static float currentWindow[WINDOW_SIZE];
static float tempWindow[WINDOW_SIZE];
static uint8_t windowIndex = 0;
static uint8_t samplesCollected = 0;

/* ================= Helpers ================= */

static void setFaultBit(FaultType type) {
  faultBitmap |= (1UL << type);
}

static bool isBitSet(FaultType type) {
  return (faultBitmap & (1UL << type)) != 0;
}

static float avgWindow(float *w) {
  if (samplesCollected == 0) return 0;
  float sum = 0;
  for (uint8_t i = 0; i < samplesCollected; i++) sum += w[i];
  return sum / samplesCollected;
}

static uint8_t max8(uint8_t a, uint8_t b) {
  return (a > b) ? a : b;
}

static void cutMotor() {
  digitalWrite(LOAD_RELAY_PIN, RELAY_OFF);
}

static void allowMotor() {
  digitalWrite(LOAD_RELAY_PIN, RELAY_ON);
}

/* ================= Init ================= */

void initFaultManager() {
  if (initialized) return;

  memset(&currentFault, 0, sizeof(currentFault));
  strcpy(currentFault.faultMessage, "NONE");
  currentFault.primaryFault = FAULT_NONE;

  pinMode(LOAD_RELAY_PIN, OUTPUT);
  allowMotor();  // motor allowed at startup

  initialized = true;
  Serial.println("[FAULT] Manager initialized");
}

/* ================= Fault Evaluation ================= */

void evaluateSystemFaults(
  float packVoltage,
  float cellMin,
  float cellMax,
  float cellImbalance,
  float current,
  bool overcurrent,
  float tempMax,
  float tempMin
) {
  if (!initialized) initFaultManager();

  bool newFault = false;

  /* ---- Voltage ---- */
  if (cellMax >= CELL_MAX_VOLTAGE || packVoltage >= MAX_VOLTAGE) {
    if (!isBitSet(FAULT_OVER_VOLTAGE)) currentFault.faultCount++;
    setFaultBit(FAULT_OVER_VOLTAGE);
    currentFault.primaryFault = FAULT_OVER_VOLTAGE;
    strcpy(currentFault.faultMessage, "OVER VOLTAGE");
    currentFault.severity = max8(currentFault.severity, 4);
    newFault = true;
  }

  if (cellMin <= CELL_MIN_VOLTAGE || packVoltage <= MIN_VOLTAGE) {
    if (!isBitSet(FAULT_UNDER_VOLTAGE)) currentFault.faultCount++;
    setFaultBit(FAULT_UNDER_VOLTAGE);
    currentFault.primaryFault = FAULT_UNDER_VOLTAGE;
    strcpy(currentFault.faultMessage, "UNDER VOLTAGE");
    currentFault.severity = max8(currentFault.severity, 4);
    newFault = true;
  }

  /* ---- Imbalance ---- */
  if (cellImbalance > MAX_CELL_IMBALANCE) {
    if (!isBitSet(FAULT_CELL_IMBALANCE)) currentFault.faultCount++;
    setFaultBit(FAULT_CELL_IMBALANCE);
    currentFault.primaryFault = FAULT_CELL_IMBALANCE;
    strcpy(currentFault.faultMessage, "CELL IMBALANCE");
    currentFault.severity = max8(currentFault.severity, 3);
    newFault = true;
  }

  /* ---- Current ---- */
  if (overcurrent) {
    if (current < 0) {
      if (!isBitSet(FAULT_OVER_CURRENT_CHARGE)) currentFault.faultCount++;
      setFaultBit(FAULT_OVER_CURRENT_CHARGE);
      strcpy(currentFault.faultMessage, "OVER CURRENT CHARGE");
      currentFault.primaryFault = FAULT_OVER_CURRENT_CHARGE;
    } else {
      if (!isBitSet(FAULT_OVER_CURRENT_DISCHARGE)) currentFault.faultCount++;
      setFaultBit(FAULT_OVER_CURRENT_DISCHARGE);
      strcpy(currentFault.faultMessage, "OVER CURRENT DISCHARGE");
      currentFault.primaryFault = FAULT_OVER_CURRENT_DISCHARGE;
    }
    currentFault.severity = 4;
    newFault = true;
  }

  /* ---- Temperature ---- */
  if (tempMax >= MAX_CELL_TEMP) {
    if (!isBitSet(FAULT_OVER_TEMPERATURE)) currentFault.faultCount++;
    setFaultBit(FAULT_OVER_TEMPERATURE);
    currentFault.primaryFault = FAULT_OVER_TEMPERATURE;
    strcpy(currentFault.faultMessage, "OVER TEMPERATURE");
    currentFault.severity = 4;
    newFault = true;
  }

  if (tempMin <= MIN_CELL_TEMP) {
    if (!isBitSet(FAULT_UNDER_TEMPERATURE)) currentFault.faultCount++;
    setFaultBit(FAULT_UNDER_TEMPERATURE);
    currentFault.primaryFault = FAULT_UNDER_TEMPERATURE;
    strcpy(currentFault.faultMessage, "UNDER TEMPERATURE");
    currentFault.severity = max8(currentFault.severity, 3);
    newFault = true;
  }

  if (tempMax >= 60.0) {
    if (!isBitSet(FAULT_THERMAL_RUNAWAY)) currentFault.faultCount++;
    setFaultBit(FAULT_THERMAL_RUNAWAY);
    currentFault.primaryFault = FAULT_THERMAL_RUNAWAY;
    strcpy(currentFault.faultMessage, "THERMAL RUNAWAY");
    currentFault.severity = 4;
    newFault = true;
  }

  /* ---- Finalize ---- */
  if (newFault && !currentFault.latched) {
    cutMotor();   // 🔴 HARD CUT

    currentFault.active = true;
    currentFault.latched = true;
    currentFault.faultTimestamp = millis();

    incrementFaultCount();
    gsmSendSMS("BMS FAULT DETECTED");
  }
}

/* ================= Public Access ================= */

bool isFaulted() {
  return currentFault.latched;
}

bool isFaultActive(FaultType type) {
  return isBitSet(type);
}

const char* faultReason() {
  return currentFault.faultMessage;
}

FaultData getFaultData() {
  return currentFault;
}

uint8_t getFaultSeverity() {
  return currentFault.severity;
}

void clearFaults() {
  memset(&currentFault, 0, sizeof(currentFault));
  strcpy(currentFault.faultMessage, "NONE");
  currentFault.primaryFault = FAULT_NONE;
  faultBitmap = 0;

  allowMotor();   // restore motor only after manual clear
}

/* ================= Motor Permission ================= */

bool shouldAllowMotor() {
  return !currentFault.latched;
}

/* ================= Edge Analytics ================= */

EdgeAnalytics performEdgeAnalytics(float v, float i, float t) {
  voltageWindow[windowIndex] = v;
  currentWindow[windowIndex] = i;
  tempWindow[windowIndex] = t;

  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  if (samplesCollected < WINDOW_SIZE) samplesCollected++;

  analytics.voltageMovingAvg = avgWindow(voltageWindow);
  analytics.currentMovingAvg = avgWindow(currentWindow);
  analytics.temperatureMovingAvg = avgWindow(tempWindow);

  analytics.anomalyScore = 0;
  if (fabs(v - analytics.voltageMovingAvg) > 0.5) analytics.anomalyScore += 30;
  if (fabs(t - analytics.temperatureMovingAvg) > 5.0) analytics.anomalyScore += 30;
  if (fabs(i) > MAX_DISCHARGE_CURRENT * 0.8) analytics.anomalyScore += 40;

  analytics.anomalyDetected = analytics.anomalyScore >= 60;
  analytics.trendWarning = analytics.anomalyScore >= 40;

  return analytics;
}

EdgeAnalytics getEdgeAnalytics() {
  return analytics;
}

/* ================= External Fault ================= */

void triggerExternalFault(FaultType type, const char* message) {
  setFaultBit(type);

  currentFault.active = true;
  currentFault.latched = true;
  currentFault.primaryFault = type;
  strncpy(currentFault.faultMessage, message, sizeof(currentFault.faultMessage) - 1);
  currentFault.severity = 3;

  cutMotor();  // 🔴 external faults also cut motor
  incrementFaultCount();
}