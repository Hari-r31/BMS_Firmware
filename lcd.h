#pragma once

void lcdInit();

/**
 * Update the 16×2 LCD display.
 *
 * Rotates between three info screens when no fault is active:
 *   Screen 0: Voltage / Current / Temperature / Mode
 *   Screen 1: SOC % / SOH %
 *   Screen 2: RUL (months) / Fan state
 *
 * When fault is active the fault screen overrides all rotation.
 *
 * @param v          Pack voltage (V)
 * @param i          Current in A (+ = discharge, - = charge)
 * @param t          Temperature (°C)
 * @param soc        State of Charge (0–100 %)
 * @param soh        State of Health (0–100 %)
 * @param rulMonths  Remaining useful life in months
 * @param fault      True if any fault is latched
 * @param faultMsg   Short fault description (≤ 16 chars)
 * @param charging   True when charge relay is active
 * @param fanOn      True when cooling fan relay is active
 */
void lcdUpdate(float v, float i, float t,
               float soc, float soh, int rulMonths,
               bool fault, const char* faultMsg,
               bool charging, bool fanOn);
