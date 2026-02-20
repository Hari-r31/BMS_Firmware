/*  ╔══════════════════════════════════════════════════════════╗
 *  ║   Smart Battery Management System (BMS) for EV          ║
 *  ║   Firmware v2.1.0                                        ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  System Operation Flow
 *  ─────────────────────
 *  1.  MCU boots → GPIO init → peripheral init
 *  2.  Sensor calibration (voltage offset, current zero, temp check)
 *  3.  Continuous sensing loop (100 ms):
 *        a.  Read pack voltage, current, temperature
 *        b.  Read accelerometer (100 Hz inner loop)
 *        c.  Update GPS
 *        d.  Edge processing (moving average + anomaly detection)
 *        e.  Battery intelligence (SOC, SOH, RUL)
 *        f.  Protection / fault evaluation
 *        g.  Relay control (charging interlock, motor, fan)
 *        h.  LCD update + serial telemetry
 *        i.  Cloud upload (rate-limited)
 */

#include "system.h"
#include "config.h"
#include "voltage.h"
#include "current.h"
#include "temperature.h"
#include "fault_manager.h"
#include "wifi_cloud.h"
#include "lcd.h"
#include "nvs_logger.h"
#include "soc.h"
#include "soh.h"
#include "rul.h"

#if ENABLE_IMPACT_DETECTION
  #include "accelerometer.h"
#endif

/* ═══════════════════════════════════════════
   SETUP
   ═══════════════════════════════════════════ */

void setup() {
  Serial.begin(115200);
  delay(500);

  /* ── 1. Banner ── */
  printSystemBanner();

  /* ── 2. GPIO configuration (all outputs default LOW/safe) ── */
  pinMode(CHARGE_RELAY_PIN, OUTPUT);
  pinMode(MOTOR_RELAY_PIN,  OUTPUT);
  pinMode(FAN_RELAY_PIN,    OUTPUT);

  digitalWrite(CHARGE_RELAY_PIN, LOW);   // charging disabled
  digitalWrite(MOTOR_RELAY_PIN,  LOW);   // motor disabled until init completes
  digitalWrite(FAN_RELAY_PIN,    LOW);   // fan off

  /* ── 3. LCD startup message ── */
  lcdInit();

  /* ── 4. Sensor initialisation & calibration ── */
  Serial.println("[INIT] Voltage sensor...");
  initVoltage();

  Serial.println("[INIT] Current sensor...");
  initCurrent();        // includes zero-offset calibration (NO LOAD)

  Serial.println("[INIT] Temperature sensor...");
  initTemperature();

  /* Read initial pack voltage for SOC seed */
  float initVoltage_V = readPackVoltage();
  Serial.printf("[INIT] Initial pack voltage: %.2f V\n", initVoltage_V);

  /* ── 5. All subsystems (WiFi, GSM, GPS, accel, NVS, SOC/SOH/RUL) ── */
  initializeAllSystems(initVoltage_V);

  /* ── 6. Diagnostics ── */
  performSystemDiagnostics();

  /* ── 7. Enable motor relay (load) – safe to run now ── */
  digitalWrite(MOTOR_RELAY_PIN, HIGH);
  Serial.println("[INIT] Motor relay ON (load enabled)");

  Serial.println("=================================");
  Serial.println("     BMS SYSTEM READY");
  Serial.println("=================================");
}

/* ═══════════════════════════════════════════
   MAIN LOOP
   ═══════════════════════════════════════════ */

void loop() {
  static unsigned long lastLoopMs = 0;
  static unsigned long lastAccelMs = 0;

  unsigned long now = millis();
  unsigned long dtMs = now - lastLoopMs;
  if (dtMs < SENSOR_READ_INTERVAL_MS) return;
  lastLoopMs = now;

  /* ── A. Accelerometer at 100 Hz (inner fast loop) ── */
#if ENABLE_IMPACT_DETECTION
  if (now - lastAccelMs >= 10) {
    lastAccelMs = now;
    readAccelerometer();
  }
#endif

  /* ── B. WiFi keep-alive ── */
  wifiEnsure();

  /* ── C. Sense: voltage, current, temperature ── */
  float packVoltage = readPackVoltage();
  CurrentData iData = readCurrentData();
  float temperature = readPackTemperature();

  /* Determine current direction from charge relay state */
  iData.direction = digitalRead(CHARGE_RELAY_PIN)
                    ? CURRENT_CHARGING
                    : (iData.current > 0.5f ? CURRENT_DISCHARGING : CURRENT_IDLE);

  /* Power calculation */
  iData.powerWatts = calculatePower(iData.current, packVoltage);

  /* Overcurrent check (direction-aware) */
  iData.overcurrentWarning = checkOvercurrent(iData.current, iData.direction);

  /* ── D. Edge processing (moving average + anomaly detection) ── */
  EdgeAnalytics edge = performEdgeAnalytics(packVoltage, iData.current, temperature);

  if (edge.anomalyDetected) {
    Serial.printf("[EDGE] ⚠ Anomaly score: %d\n", edge.anomalyScore);
  }

  /* ── E. Battery intelligence (SOC, SOH, RUL) ── */
  float signedCurrent =
    (iData.direction == CURRENT_CHARGING)    ? -iData.current :
    (iData.direction == CURRENT_DISCHARGING) ?  iData.current : 0.0f;

  updateSystemHealth(signedCurrent, packVoltage, false,
                     temperature, getCycleCount(), dtMs);

  /* ── F. Fault evaluation ── */
  evaluateSystemFaults(
    packVoltage,
    packVoltage / NUM_CELLS,   // approx cell min (single sensor)
    packVoltage / NUM_CELLS,   // approx cell max
    0.0f,                       // cell imbalance (N/A – single pack sensor)
    iData.current,
    iData.overcurrentWarning,
    temperature,
    temperature
  );

  bool fault = isFaulted();

  /* If newly faulted, send alerts */
  static bool lastFaultState = false;
  if (fault && !lastFaultState) {
    Serial.printf("[FAULT] NEW FAULT: %s\n", faultReason());

    char alertMsg[160];
    snprintf(alertMsg, sizeof(alertMsg),
             "🚨 BMS FAULT\nDevice: %s\nFault: %s\nVoltage: %.2fV\nTemp: %.1f°C",
             DEVICE_ID, faultReason(), packVoltage, temperature);
    sendTelegramAlert(String(alertMsg));
    gsmSendSMS(alertMsg);
  }
  lastFaultState = fault;

  /* ── G. Relay control ── */

  /* 1. Charging relay + interlock */
  controlCharging(packVoltage, fault);

  /* 2. Motor / load relay (OFF during fault or charging) */
  controlMotorRelay(fault, isChargingActive());

  /* 3. Thermal management */
  controlThermalManagement(temperature, fault);

  /* ── H. External events (GPS geo-fence, impact) ── */
  checkExternalEvents();

  /* ── I. Displays ── */
  float soc = getSOC();
  float soh = getSOH();
  int   rulMonths = (int)(estimateRULDays() / 30);

  lcdUpdate(packVoltage, signedCurrent, temperature,
            soc, soh, rulMonths,
            fault, faultReason(),
            isChargingActive(), isFanActive());

  displayTelemetry(packVoltage, iData, temperature, soc, fault);

  /* ── J. Cloud upload (rate-limited inside function) ── */
  uploadSystemData(packVoltage, iData, temperature, fault);
}
