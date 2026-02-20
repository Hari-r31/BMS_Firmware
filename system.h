#pragma once
#include <Arduino.h>
#include "current.h"

/* ── Lifecycle ── */
void printSystemBanner();
void initializeAllSystems(float initialPackVoltage);
void performSystemDiagnostics();

/**
 * updateSystemHealth – called every loop.
 * Updates SOC (Coulomb counting + voltage correction), SOH, and RUL.
 *
 * @param currentA    Signed current  (+ discharge, - charge)
 * @param packVoltage Current pack voltage
 * @param fault       True if any fault is latched
 * @param temp        Pack temperature
 * @param cycleCount  NVS cycle count
 * @param dtMs        Loop elapsed time in ms
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
 * controlCharging – manages the charge relay.
 * Motor relay is cut while charging (charging interlock).
 *
 * @param packVoltage Current pack voltage
 * @param fault       True = immediately open charge relay
 */
void controlCharging(float packVoltage, bool fault);

/**
 * controlMotorRelay – manages the motor / load relay.
 * Motor is only allowed when: not faulted AND not charging.
 *
 * @param fault    True = motor relay must be OFF
 * @param charging True = motor relay must be OFF (charging interlock)
 */
void controlMotorRelay(bool fault, bool charging);

/**
 * controlThermalManagement – fan hysteresis control.
 * Fan is forced ON during a fault.
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
                      bool  fault);

/* ── Accessors ── */
bool isChargingActive();
bool isFanActive();
