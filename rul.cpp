#include "rul.h"
#include "soh.h"
#include "config.h"

/* ================= Private ================= */

static bool initialized = false;
static int rulCycles = RUL_CYCLES_NEW;
static unsigned long rulHours = 0;
static float rulPercentage = 100.0;

/* Moving averages */
static float avgPackVoltage = NOMINAL_CELL_VOLTAGE * NUM_CELLS;
static float avgTemperature = 25.0;

/* ================= Helpers ================= */

static void updateMovingAverages(float voltage, float temperature) {
  const float alpha = 0.1;
  avgPackVoltage = avgPackVoltage * (1.0 - alpha) + voltage * alpha;
  avgTemperature = avgTemperature * (1.0 - alpha) + temperature * alpha;
}

static float clamp(float v, float minV, float maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

/* ================= Public ================= */

void initRUL() {
  if (initialized) return;

  float soh = getSOH();
  rulCycles = (int)(RUL_CYCLES_NEW * (soh / 100.0));
  rulPercentage = soh;

  initialized = true;

  Serial.print("[RUL] Initialized, cycles remaining: ");
  Serial.println(rulCycles);
}

int estimateRUL() {
  if (!initialized) initRUL();
  return rulCycles;
}

unsigned long estimateRULHours() {
  if (!initialized) initRUL();
  return rulHours;
}

unsigned long estimateRULDays() {
  return rulHours / 24;
}

float getRULPercentage() {
  return rulPercentage;
}

void updateRUL(
  float packVoltage,
  float temperature,
  float soh,
  unsigned long cycleCount
) {
  if (!initialized) initRUL();

  updateMovingAverages(packVoltage, temperature);

  float vFactor = getVoltageRULFactor(avgPackVoltage);
  float tFactor = getTemperatureRULFactor(avgTemperature);
  float cFactor = getCycleRULFactor(cycleCount, RUL_CYCLES_NEW);

  float combined =
    vFactor * RUL_VOLTAGE_WEIGHT +
    tFactor * RUL_TEMP_WEIGHT +
    cFactor * RUL_CYCLE_WEIGHT;

  combined = clamp(combined, 0.0, 1.0);

  rulPercentage = clamp(soh * combined, 0.0, 100.0);

  unsigned long totalCycles =
    (unsigned long)(RUL_CYCLES_NEW * (soh / 100.0));

  rulCycles = (cycleCount < totalCycles)
                ? (totalCycles - cycleCount)
                : 0;

  /* Assumption: 1 cycle per day */
  rulHours = rulCycles * 24;
  rulHours = (unsigned long)(rulHours * combined);
}

/* ================= Factors ================= */

float getVoltageRULFactor(float packVoltage) {
  float nominalPackVoltage = NOMINAL_CELL_VOLTAGE * NUM_CELLS;
  float ratio = packVoltage / nominalPackVoltage;

  if (ratio >= 1.0) return 1.0;
  if (ratio >= 0.95) return 0.9;
  if (ratio >= 0.90) return 0.7;
  if (ratio >= 0.85) return 0.5;
  return 0.3;
}

float getTemperatureRULFactor(float temperature) {
  if (temperature >= 20.0 && temperature <= 30.0) return 1.0;

  if (temperature < 20.0) {
    float d = 20.0 - temperature;
    return clamp(1.0 - d * 0.01, 0.7, 1.0);
  }

  float d = temperature - 30.0;
  return clamp(1.0 - d * 0.03, 0.2, 1.0);
}

float getCycleRULFactor(unsigned long cycleCount, unsigned long maxCycles) {
  if (cycleCount >= maxCycles) return 0.0;
  return 1.0 - ((float)cycleCount / maxCycles);
}

/* ================= Replacement ================= */

unsigned long predictReplacementDate() {
  float soh = getSOH();
  if (soh <= SOH_MIN_THRESHOLD) return 0;

  unsigned long cycles = getCycleCount();
  if (cycles == 0) cycles = 1;

  float degradationPerCycle = (100.0 - soh) / cycles;
  if (degradationPerCycle <= 0.0) return 0;

  float remainingSOH = soh - SOH_MIN_THRESHOLD;
  unsigned long cyclesLeft =
    (unsigned long)(remainingSOH / degradationPerCycle);

  return cyclesLeft;  // days (1 cycle/day assumption)
}

bool replacementNeeded() {
  return (predictReplacementDate() <= 30 ||
          getSOH() <= SOH_MIN_THRESHOLD);
}
