#pragma once
#include <Arduino.h>

/* ================= Storage ================= */
void storageInit();

/* ================= Fault Counter ================= */
void incrementFaultCount();
unsigned long getFaultCount();

/* ================= Cycle Counter ================= */
void incrementCycleCount();
unsigned long getCycleCount();
