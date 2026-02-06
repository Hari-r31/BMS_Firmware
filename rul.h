#pragma once
#include <Arduino.h>

/* ================= RUL Estimation ================= */

void initRUL();

int estimateRUL();
unsigned long estimateRULHours();
unsigned long estimateRULDays();
float getRULPercentage();

void updateRUL(
  float packVoltage,
  float temperature,
  float soh,
  unsigned long cycleCount
);

float getVoltageRULFactor(float packVoltage);
float getTemperatureRULFactor(float temperature);
float getCycleRULFactor(unsigned long cycleCount, unsigned long maxCycles);

unsigned long predictReplacementDate();
bool replacementNeeded();
