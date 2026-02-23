#pragma once

#include <Arduino.h>

/** Call once in setup() before any send calls. */
void telegramInit();

/**
 * Send alert – respects 30 s cooldown.
 * Use for repeating conditions (geofence, shock, free fall).
 * The very first call ever always goes through regardless of cooldown.
 */
bool sendTelegramAlert(const String& message);

/**
 * Send alert – BYPASSES cooldown entirely.
 * Use for critical one-time events that must never be dropped:
 *   boot, fault latch, charging start/stop, thermal trip/clear.
 */
bool sendTelegramForced(const String& message);
