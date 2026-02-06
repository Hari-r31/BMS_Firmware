#pragma once
#include <Arduino.h>
#include "current.h"

/* Lifecycle */
void printSystemBanner();
void initializeAllSystems();
void performSystemDiagnostics();
void updateSystemHealth(bool fault, float temp, unsigned long cycleCount);
void checkExternalEvents();

/* Control */
void controlCharging(float packVoltage, bool fault);
void controlThermalManagement(float temperature, bool fault);

/* Telemetry */
void displayTelemetry(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      bool fault);

void uploadSystemData(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      bool fault);
