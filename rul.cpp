#include "rul.h"
#include "soh.h"
#include "nvs_logger.h"
#include "config.h"

/* ================= Private ================= */

static bool          initialized    = false;
static int           rulCycles      = RUL_CYCLES_NEW;
static unsigned long rulHours       = 0;
static float         rulPercentage  = 100.0f;

static float avgPackVoltage = NOMINAL_CELL_VOLTAGE * NUM_CELLS;
static float avgTemperature = 25.0f;

/* ================= Helpers ================= */

static float clamp(float v, float lo, float hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void updateMovingAverages(float voltage, float temperature) {
  const float alpha = 0.1f;
  avgPackVoltage = avgPackVoltage * (1.0f - alpha) + voltage * alpha;
  avgTemperature = avgTemperature * (1.0f - alpha) + temperature * alpha;
}

/* ================= Public ================= */

void initRUL() {
  if (initialized) return;

  float soh    = getSOH();
  rulCycles    = (int)((float)RUL_CYCLES_NEW * (soh / 100.0f));
  rulPercentage = soh;

  initialized  = true;
  Serial.printf("[RUL] Initialized: %d cycles remaining\n", rulCycles);
}

void updateRUL(float packVoltage, float temperature,
               float soh, unsigned long cycleCount) {
  if (!initialized) initRUL();

  updateMovingAverages(packVoltage, temperature);

  float vFactor = getVoltageRULFactor(avgPackVoltage);
  float tFactor = getTemperatureRULFactor(avgTemperature);
  float cFactor = getCycleRULFactor(cycleCount, (unsigned long)RUL_CYCLES_NEW);

  float combined = clamp(
    vFactor * RUL_VOLTAGE_WEIGHT +
    tFactor * RUL_TEMP_WEIGHT   +
    cFactor * RUL_CYCLE_WEIGHT,
    0.0f, 1.0f
  );

  rulPercentage = clamp(soh * combined, 0.0f, 100.0f);

  unsigned long totalCycles = (unsigned long)((float)RUL_CYCLES_NEW * (soh / 100.0f));
  rulCycles  = (cycleCount < totalCycles) ? (int)(totalCycles - cycleCount) : 0;

  /* 1 cycle per day assumption → hours = cycles × 24 × combined-factor */
  rulHours   = (unsigned long)((float)(rulCycles * 24) * combined);
}

int           estimateRUL()        { return rulCycles;     }
unsigned long estimateRULHours()   { return rulHours;      }
unsigned long estimateRULDays()    { return rulHours / 24; }
float         getRULPercentage()   { return rulPercentage; }

/* ================= Factor Functions ================= */

float getVoltageRULFactor(float packVoltage) {
  float nominal = NOMINAL_CELL_VOLTAGE * NUM_CELLS;
  float ratio   = packVoltage / nominal;

  if (ratio >= 1.00f) return 1.0f;
  if (ratio >= 0.95f) return 0.9f;
  if (ratio >= 0.90f) return 0.7f;
  if (ratio >= 0.85f) return 0.5f;
  return 0.3f;
}

float getTemperatureRULFactor(float temperature) {
  if (temperature >= 20.0f && temperature <= 30.0f) return 1.0f;

  if (temperature < 20.0f)
    return clamp(1.0f - (20.0f - temperature) * 0.01f, 0.7f, 1.0f);

  return clamp(1.0f - (temperature - 30.0f) * 0.03f, 0.2f, 1.0f);
}

float getCycleRULFactor(unsigned long cycleCount, unsigned long maxCycles) {
  if (cycleCount >= maxCycles) return 0.0f;
  return 1.0f - ((float)cycleCount / (float)maxCycles);
}

/* ================= Replacement Prediction ================= */

unsigned long predictReplacementDate() {
  float         soh    = getSOH();
  if (soh <= SOH_MIN_THRESHOLD) return 0;

  unsigned long cycles = getCycleCount();
  if (cycles == 0) cycles = 1;

  float degradePerCycle = (100.0f - soh) / (float)cycles;
  if (degradePerCycle <= 0.0f) return 0;

  float remainingSOH = soh - SOH_MIN_THRESHOLD;
  return (unsigned long)(remainingSOH / degradePerCycle);  // days (1 cycle/day)
}

bool replacementNeeded() {
  return (predictReplacementDate() <= 30 || getSOH() <= SOH_MIN_THRESHOLD);
}
