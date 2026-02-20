#include "soc.h"
#include "config.h"
#include <Preferences.h>
#include <math.h>

/* ================= Private ================= */

static float soc          = 100.0;   // percent
static float remainingAh  = 0.0;
static float ratedCapAh   = CELL_CAPACITY_AH;
static bool  initialized  = false;
static bool  correctionDue = false;

static Preferences prefs;

#define SOC_SAVE_INTERVAL_MS  120000UL   // save every 2 min
static unsigned long lastSaveMs = 0;

/* Efficiency factors (Coulombic efficiency) */
#define CHARGE_EFF     0.98f   // charging  (charge that actually enters)
#define DISCHARGE_EFF  1.00f   // discharging (all charge assumed to leave)

/* Idle threshold for voltage correction */
#define IDLE_CURRENT_A  0.5f

/* ================= Voltage → SOC lookup (3S Li-ion, per-cell) ================= */
// Approximate OCV vs. SOC for a Li-ion cell (3.0 – 4.2 V range)
static float voltageToSOC_3S(float packV) {
  // Per-cell voltage
  float v = packV / NUM_CELLS;

  // Clamp
  if (v >= 4.15f) return 100.0f;
  if (v <= 3.00f) return 0.0f;

  // Piecewise linear approximation
  const float vs[] = { 3.00f, 3.20f, 3.40f, 3.60f, 3.70f, 3.80f, 3.90f, 4.00f, 4.10f, 4.15f };
  const float ss[] = {  0.0f, 5.0f, 15.0f, 30.0f, 50.0f, 65.0f, 80.0f, 90.0f, 97.0f,100.0f };
  const int n = 10;

  for (int i = 0; i < n - 1; i++) {
    if (v >= vs[i] && v < vs[i + 1]) {
      float t = (v - vs[i]) / (vs[i + 1] - vs[i]);
      return ss[i] + t * (ss[i + 1] - ss[i]);
    }
  }
  return 50.0f;   // fallback
}

/* ================= Public ================= */

void initSOC(float capacityAh, float initialVoltage) {
  if (initialized) return;

  ratedCapAh = capacityAh;

  // Try to load saved value first
  prefs.begin("bms_soc", true);
  float saved = prefs.getFloat("soc", -1.0f);
  prefs.end();

  if (saved >= 0.0f && saved <= 100.0f) {
    soc = saved;
    Serial.printf("[SOC] Loaded from NVS: %.1f%%\n", soc);
  } else {
    // Estimate from OCV on first boot
    soc = voltageToSOC_3S(initialVoltage);
    Serial.printf("[SOC] Estimated from voltage: %.1f%%\n", soc);
  }

  remainingAh = ratedCapAh * (soc / 100.0f);
  initialized = true;
}

void updateSOC(float currentA, unsigned long dtMs) {
  if (!initialized) return;

  // Convert ms → hours
  float dtH = dtMs / 3600000.0f;

  // Positive = discharging, Negative = charging
  float deltaAh;
  if (currentA > IDLE_CURRENT_A) {
    // Discharging: subtract from remaining
    deltaAh = -currentA * DISCHARGE_EFF * dtH;
  } else if (currentA < -IDLE_CURRENT_A) {
    // Charging: add to remaining (apply Coulombic efficiency)
    deltaAh = -currentA * CHARGE_EFF * dtH;  // currentA is negative, result positive
  } else {
    // Idle – schedule voltage correction
    correctionDue = true;
    return;
  }

  remainingAh += deltaAh;
  if (remainingAh < 0.0f) remainingAh = 0.0f;
  if (remainingAh > ratedCapAh) remainingAh = ratedCapAh;

  soc = (remainingAh / ratedCapAh) * 100.0f;

  // Periodic NVS save
  if (millis() - lastSaveMs > SOC_SAVE_INTERVAL_MS) {
    saveSOC();
  }
}

void correctSOCFromVoltage(float packVoltage) {
  if (!correctionDue) return;

  float voltageSoc = voltageToSOC_3S(packVoltage);

  // Soft correction: blend 10% voltage estimate into Coulomb count
  soc = soc * 0.90f + voltageSoc * 0.10f;
  if (soc < 0.0f)   soc = 0.0f;
  if (soc > 100.0f) soc = 100.0f;

  remainingAh = ratedCapAh * (soc / 100.0f);
  correctionDue = false;
}

float getSOC() {
  return soc;
}

float getRemainingAh() {
  return remainingAh;
}

void saveSOC() {
  prefs.begin("bms_soc", false);
  prefs.putFloat("soc", soc);
  prefs.end();
  lastSaveMs = millis();
  Serial.printf("[SOC] Saved: %.1f%%\n", soc);
}

void loadSOC() {
  prefs.begin("bms_soc", true);
  soc = prefs.getFloat("soc", 100.0f);
  prefs.end();
  remainingAh = ratedCapAh * (soc / 100.0f);
}

void resetSOC(float percent) {
  soc = percent;
  remainingAh = ratedCapAh * (soc / 100.0f);
  saveSOC();
}
