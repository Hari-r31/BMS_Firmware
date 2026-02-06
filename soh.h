#pragma once
#include <Arduino.h>

/* ================= SOH ================= */

void initSOH();
float getSOH();

/**
 * Update SOH based on temperature + fault
 */
void updateSOH(float current, float temperature, bool fault);

/* Degradation helpers */
void degradeSOH();
void degradeSOHByTemperature(float temperature, unsigned long duration);
void degradeSOHByCycle(float cycleDepth);

/* Capacity helpers */
float calculateSOHFromCapacity(float measuredCapacity, float nominalCapacity);
float getRemainingCapacity();

/* Cycle tracking */
unsigned long getCycleCount();
void incrementCycleCount();

/* Replacement */
bool needsReplacement();

/* Persistence */
void saveSOH();
void loadSOH();
void resetSOH();
