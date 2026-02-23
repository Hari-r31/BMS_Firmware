#pragma once
#include <Arduino.h>

/*
 * ============================================================
 *  BMS Runtime Statistics
 *  Accumulates operational counters & extremes for SOH/RUL
 *  input, cloud upload, and diagnostics.
 *  Backed by NVS (persists across power cycles).
 * ============================================================
 */

/* ──────────────────────────────────────────────────────────
   STATISTICS RECORD
   ────────────────────────────────────────────────────────── */

#define FAULT_TYPE_COUNT  14   // must match FaultType enum size

struct BmsStatistics {

  /* ── Electrical Extremes ── */
  float         peakVoltage;            // Highest pack voltage seen (V)
  float         minVoltage;             // Lowest pack voltage seen (V)
  float         peakCurrentDischarge;   // Peak discharge current (A, positive)
  float         peakCurrentCharge;      // Peak charge current (A, positive)
  float         peakTemperature;        // Highest temperature recorded (°C)
  float         minTemperature;         // Lowest temperature recorded (°C)

  /* ── Energy / Charge Throughput ── */
  float         totalEnergyOutWh;       // Cumulative energy delivered to load (Wh)
  float         totalEnergyInWh;        // Cumulative energy absorbed during charge (Wh)
  float         totalChargeAh;          // Cumulative amp-hours charged into battery
  float         totalDischargeAh;       // Cumulative amp-hours discharged from battery

  /* ── Cycle & Fault Counters ── */
  unsigned long totalCycles;            // Completed charge/discharge cycles (NVS-persisted)
  unsigned long totalFaults;            // Total fault events (all types combined)
  uint32_t      faultCountByType[FAULT_TYPE_COUNT]; // Per-fault-type breakdown

  /* ── Time Accumulators (seconds) ── */
  unsigned long totalOperatingSec;      // Total powered-on time
  unsigned long highTempSec;            // Time spent above HIGH_TEMP_THRESHOLD (45 °C)
  unsigned long chargingSec;            // Time in charging state
  unsigned long dischargingSec;         // Time in discharging state
  unsigned long idleSec;                // Time idle (|I| < deadband)

  /* ── Communication Counters ── */
  unsigned long totalCloudUploads;      // Successful Supabase pushes
  unsigned long totalSmsSent;           // GSM SMS messages dispatched
  unsigned long totalTelegramSent;      // Telegram alerts dispatched
  unsigned long cloudUploadErrors;      // Failed upload attempts

  /* ── SOH / SOC Telemetry ── */
  float         socAtLastChargeStart;   // SOC when charging began (for DoD calc)
  float         avgSocOverLifetime;     // Rolling average SOC (EMA)
};

/* ──────────────────────────────────────────────────────────
   API
   ────────────────────────────────────────────────────────── */

/** Call once in setup() to load persisted values from NVS. */
void statisticsInit();

/**
 * Call every main loop with live sensor readings.
 * Updates time accumulators, extremes, and energy throughput.
 *
 * @param voltage    Pack voltage (V)
 * @param current    Signed current (+ = discharge, − = charge) (A)
 * @param power      Instantaneous power (W, always positive)
 * @param temperature Pack temperature (°C)
 * @param charging   True when charge relay is active
 * @param dtMs       Elapsed time since last call (ms)
 */
void statisticsUpdate(float voltage,
                      float current,
                      float power,
                      float temperature,
                      bool  charging,
                      unsigned long dtMs);

/**
 * Increment the fault counter for the given FaultType index.
 * Also increments totalFaults.
 * @param faultType  FaultType enum value cast to uint8_t.
 */
void statisticsRecordFault(uint8_t faultType);

/** Record that a cloud upload succeeded. */
void statisticsRecordUpload(bool success);

/** Record that an SMS was dispatched. */
void statisticsRecordSms();

/** Record that a Telegram alert was dispatched. */
void statisticsRecordTelegram();

/** Returns a copy of the current statistics struct. */
BmsStatistics getStatistics();

/**
 * Save statistics to NVS.
 * Called automatically every STATS_SAVE_INTERVAL_MS; can be forced manually.
 */
void statisticsSave();

/** Load statistics from NVS (called by statisticsInit). */
void statisticsLoad();

/** Zero-out all counters and save. Use only for factory reset. */
void statisticsReset();

/** Print a formatted summary to Serial. */
void statisticsDump();
