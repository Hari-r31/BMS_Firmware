#pragma once
#include <Arduino.h>

/* ================= SOC (State of Charge) ================= */

/**
 * Initialize SOC with known capacity and optional starting voltage.
 * Call once in setup() after sensor calibration.
 */
void initSOC(float capacityAh, float initialVoltage);

/**
 * Update SOC using Coulomb counting (call every loop).
 * @param currentA  Signed current in Amps.
 *                  Positive  → discharging (current leaving battery)
 *                  Negative  → charging    (current entering battery)
 * @param dtMs      Elapsed time since last call (ms)
 */
void updateSOC(float currentA, unsigned long dtMs);

/**
 * Voltage correction: snap SOC when battery is idle (|I| < deadband).
 * Maps pack voltage to approximate SOC for Li-ion 3S pack.
 */
void correctSOCFromVoltage(float packVoltage);

/** Returns SOC in percent [0.0 – 100.0] */
float getSOC();

/** Remaining charge in Ah */
float getRemainingAh();

/** Save / load SOC to / from NVS */
void saveSOC();
void loadSOC();
void resetSOC(float percent = 100.0);
