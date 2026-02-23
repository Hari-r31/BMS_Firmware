#include "soh.h"
#include "config.h"
#include "nvs_logger.h"
#include <Preferences.h>

/* ================= Private ================= */

static float         soh                   = 100.0f;
static unsigned long lastSaveTime          = 0;
static unsigned long lastUpdateTime        = 0;
static unsigned long totalHighTempSeconds  = 0;
static bool          initialized           = false;
static bool          faultLatched          = false;

static Preferences prefs;

#define SOH_SAVE_INTERVAL_MS  300000UL   // save every 5 min
#define HIGH_TEMP_THRESHOLD   45.0f

/* ================= Helpers ================= */

static float clamp(float v, float lo, float hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

static float tempDegradeFactor(float t) {
  if (t < 25.0f) return 0.5f;
  if (t < 35.0f) return 1.0f;
  if (t < 45.0f) return 2.0f;
  if (t < 55.0f) return 4.0f;
  return 8.0f;
}

static float cycleDegradeFactor(float depth) {
  if (depth < 20.0f) return 0.2f;
  if (depth < 50.0f) return 0.5f;
  if (depth < 80.0f) return 1.0f;
  return 2.0f;
}

/* ================= Public ================= */

void initSOH() {
  if (initialized) return;

  loadSOH();
  lastUpdateTime = millis();
  initialized    = true;

  Serial.printf("[SOH] Initialized: %.1f%%\n", soh);
}

float getSOH() { return soh; }

void updateSOH(float current, float temperature, bool fault) {
  if (!initialized) initSOH();

  unsigned long now     = millis();
  unsigned long elapsed = now - lastUpdateTime;
  if (elapsed < 1000) return;   // update at most once per second

  float elapsedH = (float)elapsed / 3600000.0f;

  /* ── Temperature stress ── */
  if (temperature > HIGH_TEMP_THRESHOLD) {
    totalHighTempSeconds += elapsed / 1000;
    float degradation = SOH_DEGRADE_HIGH_TEMP * elapsedH *
                        tempDegradeFactor(temperature);
    soh -= degradation;
  }

  /* ── Fault degradation (edge-triggered – once per fault event) ── */
  if (fault && !faultLatched) {
    degradeSOH();
    faultLatched = true;
  }
  if (!fault) faultLatched = false;

  soh            = clamp(soh, SOH_MIN_THRESHOLD, 100.0f);
  lastUpdateTime = now;

  if (now - lastSaveTime > SOH_SAVE_INTERVAL_MS) saveSOH();
}

void degradeSOH() {
  soh = clamp(soh - SOH_DEGRADE_PER_FAULT, SOH_MIN_THRESHOLD, 100.0f);
  Serial.printf("[SOH] Fault degradation → %.1f%%\n", soh);
}

void degradeSOHByTemperature(float temperature, unsigned long durationMs) {
  float hours     = (float)durationMs / 3600000.0f;
  float degrade   = SOH_DEGRADE_HIGH_TEMP * hours * tempDegradeFactor(temperature);
  soh             = clamp(soh - degrade, SOH_MIN_THRESHOLD, 100.0f);
}

void degradeSOHByCycle(float cycleDepth) {
  float degrade = SOH_DEGRADE_PER_CYCLE * cycleDegradeFactor(cycleDepth);
  soh           = clamp(soh - degrade, SOH_MIN_THRESHOLD, 100.0f);
  Serial.printf("[SOH] Cycle degrade (%.0f%% DoD) → %.1f%%\n", cycleDepth, soh);
}

float calculateSOHFromCapacity(float measured, float nominal) {
  return clamp((measured / nominal) * 100.0f, 0.0f, 100.0f);
}

float getRemainingCapacity() {
  return INITIAL_CAPACITY_AH * (soh / 100.0f);
}

bool needsReplacement() { return soh <= SOH_MIN_THRESHOLD; }

/* ================= Persistence ================= */

void saveSOH() {
  prefs.begin("bms", false);
  prefs.putFloat("soh",        soh);
  prefs.putULong("hightemp_s", totalHighTempSeconds);
  prefs.end();
  lastSaveTime = millis();
}

void loadSOH() {
  prefs.begin("bms", true);
  soh                  = prefs.getFloat("soh",        100.0f);
  totalHighTempSeconds = prefs.getULong("hightemp_s", 0);
  prefs.end();
}

void resetSOH() {
  soh                  = 100.0f;
  totalHighTempSeconds = 0;
  saveSOH();
}
