/*
 * ============================================================
 *  EV Battery Management System – Main Firmware
 *  Device : ESP32
 *  Board  : esp32 by Espressif (Arduino core)
 * ============================================================
 *
 * REQUIRED LIBRARIES (install via Library Manager):
 *   - Adafruit INA219            (current sensor)
 *   - DHT sensor library         (temperature)
 *   - ArduinoJson                (WiFi geolocation JSON parsing)
 *   - TinyGPS++                  (optional – only if hardware GPS wired)
 *   - hd44780                    (LCD I2C)
 *   - Preferences                (ESP32 NVS – built-in)
 * ============================================================
 */

#include <Arduino.h>

#include "config.h"
#include "system.h"
#include "voltage.h"
#include "current.h"
#include "temperature.h"
#include "fault_manager.h"
#include "soc.h"
#include "soh.h"
#include "rul.h"
#include "nvs_logger.h"
#include "lcd.h"
#include "wifi_cloud.h"

#if ENABLE_GEOLOCATION
  #include "gps.h"
#endif

#if ENABLE_IMPACT_DETECTION
  #include "accelerometer.h"
#endif

/* ──────────────────────────────────────────────────────────────
   TIMING
   ────────────────────────────────────────────────────────────── */

#define LOOP_INTERVAL_MS       100UL   // main loop cadence (100 ms)
#define TELEMETRY_INTERVAL_MS 2000UL   // serial print cadence
#define WATCHDOG_INTERVAL_MS 30000UL   // reboot if loop stalls

static unsigned long lastLoopTime      = 0;
static unsigned long lastTelemetryTime = 0;
static unsigned long lastWatchdog      = 0;

/* ──────────────────────────────────────────────────────────────
   SETUP
   ────────────────────────────────────────────────────────────── */

void setup() {
  Serial.begin(115200);
  delay(500);

  printSystemBanner();

  /* ── 1. Sensor calibration reads (voltage first – needed for SOC init) ── */
  initVoltage();
  float bootVoltage = readPackVoltage();
  Serial.printf("[BOOT] Pack voltage at startup: %.2f V\n", bootVoltage);

  initCurrent();
  initTemperature();

  /* ── 2. Initialize all subsystems ── */
  initializeAllSystems(bootVoltage);

  /* ── 3. Post-init diagnostics ── */
  delay(1000);   // let GPS/GSM settle
  performSystemDiagnostics();

  lastLoopTime      = millis();
  lastTelemetryTime = millis();
  lastWatchdog      = millis();

  Serial.println("[BOOT] Setup complete – entering monitoring loop\n");
}

/* ──────────────────────────────────────────────────────────────
   MAIN LOOP  – runs every LOOP_INTERVAL_MS
   ────────────────────────────────────────────────────────────── */

void loop() {

  unsigned long now = millis();

  /* ── Enforce fixed loop cadence ── */
  unsigned long elapsed = now - lastLoopTime;
  if (elapsed < LOOP_INTERVAL_MS) {
    delay(LOOP_INTERVAL_MS - elapsed);
    return;
  }
  unsigned long dtMs = elapsed;   // actual elapsed (may be slightly > 100 ms)
  lastLoopTime = millis();

  /* ── Watchdog pet ── */
  lastWatchdog = millis();

  /* ── WiFi keep-alive ── */
  wifiEnsure();

  /* ══════════════════════════════════════════════════════════
     STEP 1 – CONTINUOUS SENSING
     ══════════════════════════════════════════════════════════ */

  float        packVoltage = readPackVoltage();
  CurrentData  iData       = readCurrentData();
  float        temperature = readPackTemperature();

  /* For 3S pack, estimate per-cell values from pack voltage */
  float cellAvg    = packVoltage / (float)NUM_CELLS;
  float cellMin    = cellAvg;   // single sensor → treat avg as min/max
  float cellMax    = cellAvg;
  float cellImbal  = 0.0f;      // individual cell monitoring not wired – use 0

  /* ══════════════════════════════════════════════════════════
     STEP 2 – EDGE PROCESSING (moving average + anomaly score)
     ══════════════════════════════════════════════════════════ */

  EdgeAnalytics edge = performEdgeAnalytics(packVoltage,
                                            iData.current,
                                            temperature);

  if (edge.anomalyDetected)
    Serial.printf("[EDGE] Anomaly score=%u – monitoring\n", edge.anomalyScore);

  /* ══════════════════════════════════════════════════════════
     STEP 3 – BATTERY INTELLIGENCE  (SOC / SOH / RUL)
     ══════════════════════════════════════════════════════════ */

  bool fault = isFaulted();

  updateSystemHealth(
    iData.current,
    packVoltage,
    fault,
    temperature,
    getCycleCount(),
    dtMs
  );

  float soc = getSOC();

  /* ══════════════════════════════════════════════════════════
     STEP 4 – PROTECTION LOGIC
     ══════════════════════════════════════════════════════════ */

  /* ── Skip fault evaluation + current logic during motor inrush (500 ms) ── */
  bool blanking = isMotorStartBlanking();
  if (blanking) {
    Serial.println("[BLANK] Motor inrush – skipping fault eval");
  }

  /* Electrical + thermal protection (skip during motor inrush) */
  if (!blanking) {
    evaluateSystemFaults(
      packVoltage,
      cellMin,
      cellMax,
      cellImbal,
      iData.current,
      iData.overCurrent,
      temperature,   // tempMax
      temperature    // tempMin  (single sensor)
    );
  }

  /* GPS / Impact external events */
  checkExternalEvents();

  /* ── Auto-recover faults whose conditions have cleared ── */
  if (!blanking) {
    autoCheckFaultRecovery(
      packVoltage,
      iData.current,
      iData.overCurrent,
      temperature
    );
  }

  /* Refresh fault flag after evaluation and recovery check */
  fault = isFaulted();

  /* ── SOH: Battery aging check ── */
  if (needsReplacement() && !isFaultActive(FAULT_BATTERY_AGING))
    triggerExternalFault(FAULT_BATTERY_AGING, "BATTERY AGING");

  /* ══════════════════════════════════════════════════════════
     STEP 5 – RELAY / ACTUATOR CONTROL
     ══════════════════════════════════════════════════════════ */

  /* Charging interlock (also handles motor relay during charge) */
  controlCharging(packVoltage, fault);

  /* Motor relay – driven purely by live current, never relay state */
  controlMotorRelay(fault, iData.current);

  /* Charging current monitor – alerts based on actual INA219 reading */
  monitorChargingCurrent(iData.current, packVoltage);

  /* Thermal management */
  controlThermalManagement(temperature, fault);

  /* ══════════════════════════════════════════════════════════
     STEP 6 – DISPLAY  (LCD)
     ══════════════════════════════════════════════════════════ */

#if ENABLE_LOCAL_DISPLAY
  lcdUpdate(
    packVoltage,
    iData.current,
    temperature,
    soc,
    getSOH(),
    (int)(estimateRULDays() / 30),   // days → approximate months
    fault,
    faultReason(),
    isChargingActive(),
    isFanActive()
  );
#endif

  /* ══════════════════════════════════════════════════════════
     STEP 7 – SERIAL TELEMETRY  (every 2 s)
     ══════════════════════════════════════════════════════════ */

  if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    displayTelemetry(packVoltage, iData, temperature, soc, fault);
    lastTelemetryTime = millis();
  }

  /* ══════════════════════════════════════════════════════════
     STEP 8 – CLOUD UPLOAD  (throttled inside uploadSystemData)
     ══════════════════════════════════════════════════════════ */

#if ENABLE_CLOUD_DASHBOARD
  uploadSystemData(packVoltage, iData, temperature, soc, fault);
#endif
}
