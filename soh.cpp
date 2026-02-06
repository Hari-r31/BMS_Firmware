#include "soh.h"
#include "config.h"
#include "nvs_logger.h"   // ✅ Cycle count comes ONLY from NVS
#include <Preferences.h>

/* ================= Private ================= */

static float soh = 100.0;
static unsigned long lastSaveTime = 0;
static unsigned long lastUpdateTime = 0;
static unsigned long totalHighTempSeconds = 0;
static bool initialized = false;
static bool faultLatched = false;

static Preferences prefs;

/* ================= Constants ================= */

#define SOH_SAVE_INTERVAL_MS 300000
#define HIGH_TEMP_THRESHOLD 45.0

/* ================= Helpers ================= */

static float clamp(float v, float minV, float maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

static float getTemperatureDegradationFactor(float t) {
  if (t < 25.0) return 0.5;
  if (t < 35.0) return 1.0;
  if (t < 45.0) return 2.0;
  if (t < 55.0) return 4.0;
  return 8.0;
}

static float getCycleDegradationFactor(float depth) {
  if (depth < 20.0) return 0.2;
  if (depth < 50.0) return 0.5;
  if (depth < 80.0) return 1.0;
  return 2.0;
}

/* ================= Public ================= */

void initSOH() {
  if (initialized) return;

  loadSOH();
  lastUpdateTime = millis();
  initialized = true;

  Serial.print("[SOH] Initialized: ");
  Serial.print(soh, 1);
  Serial.println("%");
}

float getSOH() {
  return soh;
}

void updateSOH(float current, float temperature, bool fault) {
  if (!initialized) initSOH();

  unsigned long now = millis();
  unsigned long elapsed = now - lastUpdateTime;
  if (elapsed < 1000) return;

  float elapsedHours = elapsed / 3600000.0;

  /* ---------- Temperature degradation ---------- */
  if (temperature > HIGH_TEMP_THRESHOLD) {
    totalHighTempSeconds += elapsed / 1000;
    float factor = getTemperatureDegradationFactor(temperature);
    float degradation = SOH_DEGRADE_HIGH_TEMP * elapsedHours * factor;
    soh -= degradation;
  }

  /* ---------- Fault degradation (edge-triggered) ---------- */
  if (fault && !faultLatched) {
    degradeSOH();
    faultLatched = true;
  }
  if (!fault) {
    faultLatched = false;
  }

  soh = clamp(soh, SOH_MIN_THRESHOLD, 100.0);
  lastUpdateTime = now;

  if (now - lastSaveTime > SOH_SAVE_INTERVAL_MS) {
    saveSOH();
  }
}

void degradeSOH() {
  soh -= SOH_DEGRADE_PER_FAULT;
  soh = clamp(soh, SOH_MIN_THRESHOLD, 100.0);

  Serial.print("[SOH] Fault degradation → ");
  Serial.print(soh, 1);
  Serial.println("%");
}

void degradeSOHByTemperature(float temperature, unsigned long duration) {
  float hours = duration / 3600.0;
  float factor = getTemperatureDegradationFactor(temperature);
  soh -= SOH_DEGRADE_HIGH_TEMP * hours * factor;
  soh = clamp(soh, SOH_MIN_THRESHOLD, 100.0);
}

void degradeSOHByCycle(float cycleDepth) {
  float factor = getCycleDegradationFactor(cycleDepth);
  float degradation = SOH_DEGRADE_PER_CYCLE * factor;
  soh -= degradation;
  soh = clamp(soh, SOH_MIN_THRESHOLD, 100.0);

  Serial.print("[SOH] Cycle degradation (");
  Serial.print(cycleDepth);
  Serial.print("%) → ");
  Serial.print(soh, 1);
  Serial.println("%");
}

float calculateSOHFromCapacity(float measured, float nominal) {
  return clamp((measured / nominal) * 100.0, 0.0, 100.0);
}

float getRemainingCapacity() {
  return INITIAL_CAPACITY_AH * (soh / 100.0);
}

bool needsReplacement() {
  return soh <= SOH_MIN_THRESHOLD;
}

/* ================= Persistence ================= */

void saveSOH() {
  prefs.begin("bms", false);
  prefs.putFloat("soh", soh);
  prefs.putULong("hightemp_s", totalHighTempSeconds);
  prefs.end();

  lastSaveTime = millis();
}

void loadSOH() {
  prefs.begin("bms", true);
  soh = prefs.getFloat("soh", 100.0);
  totalHighTempSeconds = prefs.getULong("hightemp_s", 0);
  prefs.end();
}

void resetSOH() {
  soh = 100.0;
  totalHighTempSeconds = 0;
  saveSOH();
}
