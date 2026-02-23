#include "fault_manager.h"
#include "config.h"
#include "gsm_sms.h"
#include "telegram.h"
#include "nvs_logger.h"
#include <string.h>
#include <math.h>

/* ================= State ================= */

static FaultData     currentFault;
static EdgeAnalytics analytics;
static bool          initialized = false;
static uint32_t      faultBitmap = 0;

/* ================= Edge-analytics window ================= */

#define WINDOW_SIZE 10
static float   voltageWindow[WINDOW_SIZE];
static float   currentWindow[WINDOW_SIZE];
static float   tempWindow[WINDOW_SIZE];
static uint8_t windowIndex      = 0;
static uint8_t samplesCollected = 0;

/* ================= Helpers ================= */

static void setFaultBit(FaultType t)  { faultBitmap |=  (1UL << t); }
static bool isBitSet(FaultType t)     { return (faultBitmap & (1UL << t)) != 0; }

static float avgWindow(const float* w) {
  if (samplesCollected == 0) return 0.0f;
  float s = 0;
  for (uint8_t i = 0; i < samplesCollected; i++) s += w[i];
  return s / samplesCollected;
}

static uint8_t max8(uint8_t a, uint8_t b) { return (a > b) ? a : b; }

/* Relay helpers – use config pin names */
static void cutMotor()   { digitalWrite(LOAD_MOTOR_RELAY_PIN, LOW);  }
static void allowMotor() { digitalWrite(LOAD_MOTOR_RELAY_PIN, HIGH); }

static void latchFault(const char* msg, FaultType type, uint8_t sev) {
  bool isNew = !isBitSet(type);
  setFaultBit(type);

  if (isNew) currentFault.faultCount++;
  currentFault.primaryFault = type;
  strncpy(currentFault.faultMessage, msg, sizeof(currentFault.faultMessage) - 1);
  currentFault.severity = max8(currentFault.severity, sev);

  if (!currentFault.latched) {
    currentFault.active         = true;
    currentFault.latched        = true;
    currentFault.faultTimestamp = millis();

    cutMotor();
    incrementFaultCount();

    /* Build alert with device ID prefix so both SMS and Telegram are clear */
    char alert[128];
    snprintf(alert, sizeof(alert), "BMS ALERT [%s]\nFAULT: %s", DEVICE_ID, msg);

    gsmSendSMS(alert);
    sendTelegramForced(String(alert));   // fault latch – must never be skipped

    Serial.printf("[FAULT] Latched: %s (sev=%u)\n", msg, sev);
  }
}

/* ================= Init ================= */

void initFaultManager() {
  if (initialized) return;

  memset(&currentFault, 0, sizeof(currentFault));
  strncpy(currentFault.faultMessage, "NONE", sizeof(currentFault.faultMessage));
  currentFault.primaryFault = FAULT_NONE;

  pinMode(LOAD_MOTOR_RELAY_PIN, OUTPUT);
  digitalWrite(LOAD_MOTOR_RELAY_PIN, LOW);  // keep OFF during init – enabled after all systems ready

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
  bool  overcurrent,
  float tempMax,
  float tempMin
) {
  if (!initialized) initFaultManager();

  /* ── Over Voltage ── */
  if (cellMax >= CELL_MAX_VOLTAGE || packVoltage >= MAX_VOLTAGE)
    latchFault("OVER VOLTAGE", FAULT_OVER_VOLTAGE, 4);

  /* ── Under Voltage ── */
  if (cellMin <= CELL_MIN_VOLTAGE || packVoltage <= MIN_VOLTAGE)
    latchFault("UNDER VOLTAGE", FAULT_UNDER_VOLTAGE, 4);

  /* ── Cell Imbalance ── */
  if (cellImbalance > MAX_CELL_IMBALANCE)
    latchFault("CELL IMBALANCE", FAULT_CELL_IMBALANCE, 3);

  /* ── Over Current ── */
  if (overcurrent) {
    if (current < 0.0f)
      latchFault("OVER CURRENT CHARGE",    FAULT_OVER_CURRENT_CHARGE,    4);
    else
      latchFault("OVER CURRENT DISCHARGE", FAULT_OVER_CURRENT_DISCHARGE, 4);
  }

  /* ── Over Temperature ── */
  if (tempMax >= MAX_CELL_TEMP)
    latchFault("OVER TEMPERATURE", FAULT_OVER_TEMPERATURE, 4);

  /* ── Under Temperature ── */
  if (tempMin <= MIN_CELL_TEMP)
    latchFault("UNDER TEMPERATURE", FAULT_UNDER_TEMPERATURE, 3);

  /* ── Thermal Runaway (temp exceeds absolute danger limit) ── */
  if (tempMax >= MAX_PACK_TEMP)
    latchFault("THERMAL RUNAWAY", FAULT_THERMAL_RUNAWAY, 4);
}

/* ================= Public Accessors ================= */

bool        isFaulted()                   { return currentFault.latched; }
bool        isFaultActive(FaultType type) { return isBitSet(type); }
const char* faultReason()                 { return currentFault.faultMessage; }
FaultData   getFaultData()                { return currentFault; }
uint8_t     getFaultSeverity()            { return currentFault.severity; }

/* ================= Auto Fault Recovery ================= */

/*
 * Recoverable faults: cleared automatically when condition resolves.
 * Non-recoverable faults: require manual clearFaults() call.
 *
 * Recovery hysteresis margins prevent chattering:
 *   Voltage: must recover 0.1 V inside the safe band
 *   Current: overcurrent flag must drop
 *   Temp:    must drop 2 °C below the trigger threshold
 */
void autoCheckFaultRecovery(
  float packVoltage,
  float current,
  bool  overcurrent,
  float temperature
) {
  if (!currentFault.latched) return;   // nothing to check

  bool changed = false;

  /* ── Over Voltage recovery ── */
  if (isBitSet(FAULT_OVER_VOLTAGE) && packVoltage < (MAX_VOLTAGE - 0.1f)) {
    faultBitmap &= ~(1UL << FAULT_OVER_VOLTAGE);
    Serial.println("[FAULT] OV cleared");
    changed = true;
  }

  /* ── Under Voltage recovery ── */
  if (isBitSet(FAULT_UNDER_VOLTAGE) && packVoltage > (MIN_VOLTAGE + 0.1f)) {
    faultBitmap &= ~(1UL << FAULT_UNDER_VOLTAGE);
    Serial.println("[FAULT] UV cleared");
    changed = true;
  }

  /* ── Over Current recovery (charge & discharge) ── */
  if (!overcurrent) {
    if (isBitSet(FAULT_OVER_CURRENT_CHARGE)) {
      faultBitmap &= ~(1UL << FAULT_OVER_CURRENT_CHARGE);
      Serial.println("[FAULT] OC-CHG cleared");
      changed = true;
    }
    if (isBitSet(FAULT_OVER_CURRENT_DISCHARGE)) {
      faultBitmap &= ~(1UL << FAULT_OVER_CURRENT_DISCHARGE);
      Serial.println("[FAULT] OC-DIS cleared");
      changed = true;
    }
  }

  /* ── Over Temperature recovery (2 °C hysteresis) ── */
  if (isBitSet(FAULT_OVER_TEMPERATURE) && temperature < (MAX_CELL_TEMP - 2.0f)) {
    faultBitmap &= ~(1UL << FAULT_OVER_TEMPERATURE);
    Serial.println("[FAULT] OT cleared");
    changed = true;
  }

  /* ── Under Temperature recovery (2 °C hysteresis) ── */
  if (isBitSet(FAULT_UNDER_TEMPERATURE) && temperature > (MIN_CELL_TEMP + 2.0f)) {
    faultBitmap &= ~(1UL << FAULT_UNDER_TEMPERATURE);
    Serial.println("[FAULT] UT cleared");
    changed = true;
  }

  /* ── Cell Imbalance (clears if OV/UV gone; re-evaluated next evaluateSystemFaults) ── */
  /* Left to manual clear – imbalance needs operator attention */

  /*
   * Non-recoverable faults that always require manual clearFaults():
   *   FAULT_THERMAL_RUNAWAY, FAULT_IMPACT_DETECTED,
   *   FAULT_GEOFENCE_VIOLATION, FAULT_BATTERY_AGING,
   *   FAULT_CELL_IMBALANCE, FAULT_SENSOR_FAILURE, FAULT_COMMUNICATION_LOSS
   */

  if (!changed) return;

  /* Check if ANY fault bit is still set */
  /* Non-zero bitmap → still faulted */
  if (faultBitmap == 0) {
    /* All recoverable faults cleared → unlock system */
    currentFault.active   = false;
    currentFault.latched  = false;
    currentFault.severity = 0;
    strncpy(currentFault.faultMessage, "NONE", sizeof(currentFault.faultMessage));
    currentFault.primaryFault = FAULT_NONE;
    allowMotor();
    Serial.println("[FAULT] All faults resolved – system recovered, motor relay ON");
  } else {
    /* Still faulted on other bits – update primary fault message to most recent set bit */
    /* Walk the bitmap from highest severity down */
    static const struct { FaultType t; const char* msg; } priority[] = {
      { FAULT_THERMAL_RUNAWAY,        "THERMAL RUNAWAY"       },
      { FAULT_OVER_TEMPERATURE,       "OVER TEMPERATURE"      },
      { FAULT_UNDER_TEMPERATURE,      "UNDER TEMPERATURE"     },
      { FAULT_OVER_VOLTAGE,           "OVER VOLTAGE"          },
      { FAULT_UNDER_VOLTAGE,          "UNDER VOLTAGE"         },
      { FAULT_OVER_CURRENT_CHARGE,    "OVER CURRENT CHARGE"   },
      { FAULT_OVER_CURRENT_DISCHARGE, "OVER CURRENT DISCHARGE"},
      { FAULT_CELL_IMBALANCE,         "CELL IMBALANCE"        },
      { FAULT_IMPACT_DETECTED,        "IMPACT DETECTED"       },
      { FAULT_GEOFENCE_VIOLATION,     "GEOFENCE VIOLATION"    },
      { FAULT_BATTERY_AGING,          "BATTERY AGING"         },
    };
    for (auto& p : priority) {
      if (isBitSet(p.t)) {
        strncpy(currentFault.faultMessage, p.msg,
                sizeof(currentFault.faultMessage) - 1);
        currentFault.primaryFault = p.t;
        break;
      }
    }
    Serial.printf("[FAULT] Partial recovery – remaining: %s\n",
                  currentFault.faultMessage);
  }
}

void clearFaults() {
  memset(&currentFault, 0, sizeof(currentFault));
  strncpy(currentFault.faultMessage, "NONE", sizeof(currentFault.faultMessage));
  currentFault.primaryFault = FAULT_NONE;
  faultBitmap = 0;
  allowMotor();   // re-enable motor only after manual clear
  Serial.println("[FAULT] Cleared – motor relay restored");
}

bool shouldAllowMotor() { return !currentFault.latched; }

/* ================= Edge Analytics ================= */

EdgeAnalytics performEdgeAnalytics(float v, float i, float t) {
  voltageWindow[windowIndex] = v;
  currentWindow[windowIndex] = i;
  tempWindow[windowIndex]    = t;

  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  if (samplesCollected < WINDOW_SIZE) samplesCollected++;

  analytics.voltageMovingAvg     = avgWindow(voltageWindow);
  analytics.currentMovingAvg     = avgWindow(currentWindow);
  analytics.temperatureMovingAvg = avgWindow(tempWindow);

  /* Anomaly scoring */
  analytics.anomalyScore = 0;

  if (fabsf(v - analytics.voltageMovingAvg) > 0.5f)
    analytics.anomalyScore += 30;

  if (fabsf(t - analytics.temperatureMovingAvg) > 5.0f)
    analytics.anomalyScore += 30;

  if (fabsf(i) > MAX_DISCHARGE_CURRENT * 0.8f)
    analytics.anomalyScore += 40;

  analytics.anomalyDetected = (analytics.anomalyScore >= 60);
  analytics.trendWarning    = (analytics.anomalyScore >= 40);

  return analytics;
}

EdgeAnalytics getEdgeAnalytics() { return analytics; }

/* ================= External Fault ================= */

void triggerExternalFault(FaultType type, const char* message) {
  latchFault(message, type, 3);
}
