#pragma once
#include <Arduino.h>
#include "current.h"

/* ── Lifecycle ── */
void printSystemBanner();
void initializeAllSystems(float initialPackVoltage);
void performSystemDiagnostics();

/**
 * updateSystemHealth – call every loop.
 * Updates SOC (Coulomb counting + voltage correction), SOH, and RUL.
 *
 * @param currentA    Signed current (+ = discharge, − = charge)
 * @param packVoltage Current pack voltage
 * @param fault       True if any fault is latched
 * @param temp        Pack temperature
 * @param cycleCount  NVS cycle count
 * @param dtMs        Loop elapsed time (ms)
 */
void updateSystemHealth(float currentA,
                        float packVoltage,
                        bool  fault,
                        float temp,
                        unsigned long cycleCount,
                        unsigned long dtMs);

void checkExternalEvents();

/* ── Relay control ── */

/**
 * controlCharging – manages charge relay with motor interlock.
 * @param packVoltage  Current pack voltage
 * @param fault        True = immediately disable charging
 */
void controlCharging(float packVoltage, bool fault);

/**
 * controlMotorRelay – motor ON only when not faulted AND current is NOT
 * flowing INTO the battery (i.e. not actually charging).
 * Decision is based on live INA219 current, not relay state.
 *
 * @param fault     True if any fault is latched
 * @param currentA  Live signed current (negative = charging)
 */
void controlMotorRelay(bool fault, float currentA);

/**
 * isMotorStartBlanking – returns true for 500 ms after motor relay energises.
 * During this window fault evaluation and current readings are skipped
 * to ignore inrush voltage/current spikes.
 */
bool isMotorStartBlanking();

/**
 * controlThermalManagement – fan hysteresis + forced ON during fault.
 */
void controlThermalManagement(float temperature, bool fault);

/* ── Telemetry ── */
void displayTelemetry(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      float soc,
                      bool  fault);

void uploadSystemData(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      float soc,
                      bool  fault);

/**
 * monitorChargingCurrent – call every loop with live current.
 * Sends alerts based purely on actual current flow:
 *   - Current starts flowing in  → "Charging in progress"
 *   - Current stops flowing in   → "Charging current stopped"
 */
void monitorChargingCurrent(float currentA, float packVoltage);

/* ── Accessors ── */
bool isChargingActive();
bool isFanActive();
bool isThermalTripped();
