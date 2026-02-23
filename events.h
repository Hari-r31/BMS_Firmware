#pragma once
#include <Arduino.h>

/*
 * ============================================================
 *  BMS Event System
 *  Tracks discrete system events (faults, mode changes, alerts)
 *  in a fixed-size ring buffer.
 * ============================================================
 */

/* ──────────────────────────────────────────────────────────
   EVENT TYPES
   ────────────────────────────────────────────────────────── */

typedef enum : uint8_t {
  EVENT_NONE               =  0,
  EVENT_FAULT_TRIGGERED    =  1,   // A fault was latched
  EVENT_FAULT_CLEARED      =  2,   // A fault was automatically cleared
  EVENT_FAULTS_ALL_CLEAR   =  3,   // All faults resolved – system recovered
  EVENT_CHARGING_START     =  4,   // Charge relay energised
  EVENT_CHARGING_STOP      =  5,   // Charge relay de-energised (pack full)
  EVENT_MOTOR_ON           =  6,   // Motor/load relay enabled
  EVENT_MOTOR_OFF          =  7,   // Motor/load relay disabled (fault or charge)
  EVENT_FAN_ON             =  8,   // Cooling fan activated
  EVENT_FAN_OFF            =  9,   // Cooling fan deactivated
  EVENT_IMPACT_DETECTED    = 10,   // Accelerometer impact event
  EVENT_GEOFENCE_VIOLATION = 11,   // Vehicle left geofenced area
  EVENT_SOC_LOW            = 12,   // SOC fell below 20 %
  EVENT_SOC_CRITICAL       = 13,   // SOC fell below 10 %
  EVENT_SOH_DEGRADED       = 14,   // SOH crossed replacement threshold
  EVENT_SMS_SENT           = 15,   // GSM SMS dispatched
  EVENT_TELEGRAM_SENT      = 16,   // Telegram alert dispatched
  EVENT_CLOUD_UPLOAD       = 17,   // Cloud telemetry upload succeeded
  EVENT_SYSTEM_BOOT        = 18,   // System power-on / reboot
  EVENT_CALIBRATION_DONE   = 19,   // Sensor calibration complete
} EventType;

/* ──────────────────────────────────────────────────────────
   EVENT RECORD
   ────────────────────────────────────────────────────────── */

struct SystemEvent {
  EventType     type;             // What happened
  uint8_t       faultType;        // FaultType enum value (0 if not fault-related)
  unsigned long timestamp;        // millis() at time of event
  float         value;            // Contextual measurement (voltage, temp, …)
  char          description[48];  // Human-readable detail string
};

/* ──────────────────────────────────────────────────────────
   RING-BUFFER SIZE
   ────────────────────────────────────────────────────────── */

#define EVENT_QUEUE_SIZE  32    // Number of events stored before wrap-around

/* ──────────────────────────────────────────────────────────
   API
   ────────────────────────────────────────────────────────── */

/** Must be called once in setup() before any other events_ call. */
void eventsInit();

/**
 * Push a new event onto the ring buffer.
 * @param type        Event category.
 * @param faultType   FaultType value (0 = FAULT_NONE when not fault-related).
 * @param value       Numeric context (e.g., voltage at the time of OV fault).
 * @param description Short text label (≤ 47 chars).
 */
void eventsPush(EventType type, uint8_t faultType,
                float value, const char* description);

/**
 * Pop the oldest event from the buffer.
 * @param out  Pointer to SystemEvent to fill.
 * @return     true if an event was available; false if buffer was empty.
 */
bool eventsPop(SystemEvent* out);

/** Returns number of unread events currently in the buffer. */
uint16_t eventsAvailable();

/** Discard all buffered events. */
void eventsClear();

/** Returns a copy of the most recently pushed event (does NOT remove it). */
SystemEvent eventsGetLast();

/** Print all pending events to Serial (flushes the buffer). */
void eventsDump();
