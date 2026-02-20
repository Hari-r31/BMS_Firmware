#pragma once
#include <Arduino.h>

/* ================= SOH (State of Health) ================= */

void initSOH();
float getSOH();

/**
 * Update SOH based on temperature stress + fault events.
 * Call every loop.
 *
 * @param current     Pack current (A) – used for future depth-of-discharge
 * @param temperature Pack temperature (°C)
 * @param fault       True if any fault is currently latched
 */
void updateSOH(float current, float temperature, bool fault);

/* Degradation helpers */
void degradeSOH();
void degradeSOHByTemperature(float temperature, unsigned long durationMs);
void degradeSOHByCycle(float cycleDepthPercent);

/* Capacity helpers */
float calculateSOHFromCapacity(float measuredCapacity, float nominalCapacity);
float getRemainingCapacity();

/* Replacement */
bool needsReplacement();

/* Persistence */
void saveSOH();
void loadSOH();
void resetSOH();
