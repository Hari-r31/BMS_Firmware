#include "soc.h"
#include "config.h"
#include <Preferences.h>
#include <math.h>

/* ================= Private ================= */

static float soc         = 100.0f;
static float remainingAh = 0.0f;
static float ratedCapAh  = CELL_CAPACITY_AH;
static bool  initialized = false;
static bool  correctionDue = false;

static Preferences prefs;

#define SOC_SAVE_INTERVAL_MS  120000UL   // save every 2 min

/* Coulombic efficiency */
#define CHARGE_EFF     0.98f
#define DISCHARGE_EFF  1.00f

/* Dead-band: |current| < this → treat as idle */
#define IDLE_CURRENT_A  0.15f

/* ================= OCV → SOC lookup (3S Li-ion) ================= */

/*
 * Per-cell OCV (V) vs SOC (%).
 * Approximate piecewise-linear for a typical 18650 / LiPo cell.
 */
static float voltageToSOC_3S(float packV) {
  float v = packV / NUM_CELLS;   // per-cell voltage

  if (v >= 4.15f) return 100.0f;
  if (v <= 3.00f) return   0.0f;

  static const float vs[] = { 3.00f, 3.20f, 3.40f, 3.60f, 3.70f, 3.80f, 3.90f, 4.00f, 4.10f, 4.15f };
  static const float ss[] = {  0.0f,  5.0f, 15.0f, 30.0f, 50.0f, 65.0f, 80.0f, 90.0f, 97.0f,100.0f };
  const int n = 10;

  for (int i = 0; i < n - 1; i++) {
    if (v >= vs[i] && v < vs[i + 1]) {
      float t = (v - vs[i]) / (vs[i + 1] - vs[i]);
      return ss[i] + t * (ss[i + 1] - ss[i]);
    }
  }
  return 50.0f;
}

/* ================= Public ================= */

void initSOC(float capacityAh, float initialVoltage) {
  if (initialized) return;

  ratedCapAh = capacityAh;

  prefs.begin("bms_soc", true);
  float saved = prefs.getFloat("soc", -1.0f);
  prefs.end();

  if (saved >= 0.0f && saved <= 100.0f) {
    soc = saved;
    Serial.printf("[SOC] Loaded from NVS: %.1f%%\n", soc);
  } else {
    soc = voltageToSOC_3S(initialVoltage);
    Serial.printf("[SOC] Estimated from OCV: %.1f%%\n", soc);
  }

  remainingAh  = ratedCapAh * (soc / 100.0f);
  initialized  = true;
}

void updateSOC(float currentA, unsigned long dtMs) {
  if (!initialized) return;

  float dtH = (float)dtMs / 3600000.0f;

  /*
   * Convention matches INA219 readout:
   *   currentA > 0  → discharging → subtract capacity
   *   currentA < 0  → charging    → add capacity (with Coulombic efficiency)
   */
  if (fabsf(currentA) < IDLE_CURRENT_A) {
    /* Idle – schedule a voltage-based correction on next opportunity */
    correctionDue = true;
    return;
  }

  float deltaAh;
  if (currentA > 0.0f) {
    /* Discharging */
    deltaAh = -(currentA * DISCHARGE_EFF * dtH);
  } else {
    /* Charging (currentA is negative → -currentA is positive) */
    deltaAh = (-currentA) * CHARGE_EFF * dtH;
  }

  remainingAh += deltaAh;
  remainingAh  = fmaxf(0.0f, fminf(remainingAh, ratedCapAh));
  soc          = (remainingAh / ratedCapAh) * 100.0f;

  static unsigned long lastSaveMs = 0;
  if (millis() - lastSaveMs > SOC_SAVE_INTERVAL_MS) {
    saveSOC();
    lastSaveMs = millis();
  }
}

void correctSOCFromVoltage(float packVoltage) {
  if (!correctionDue) return;

  float voltageSoc = voltageToSOC_3S(packVoltage);

  /* Soft blend: 10% voltage estimate, 90% Coulomb-count */
  soc = soc * 0.90f + voltageSoc * 0.10f;
  soc = fmaxf(0.0f, fminf(soc, 100.0f));

  remainingAh   = ratedCapAh * (soc / 100.0f);
  correctionDue = false;
}

float getSOC()         { return soc;         }
float getRemainingAh() { return remainingAh; }

void saveSOC() {
  prefs.begin("bms_soc", false);
  prefs.putFloat("soc", soc);
  prefs.end();
  Serial.printf("[SOC] Saved: %.1f%%\n", soc);
}

void loadSOC() {
  prefs.begin("bms_soc", true);
  soc = prefs.getFloat("soc", 100.0f);
  prefs.end();
  remainingAh = ratedCapAh * (soc / 100.0f);
}

void resetSOC(float percent) {
  soc         = percent;
  remainingAh = ratedCapAh * (soc / 100.0f);
  saveSOC();
}
