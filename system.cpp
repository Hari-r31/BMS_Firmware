#include "system.h"
#include "config.h"
#include "fault_manager.h"
#include "soh.h"
#include "rul.h"
#include "soc.h"
#include "wifi_cloud.h"
#include "gsm_sms.h"
#include "telegram.h"
#include "nvs_logger.h"
#include "lcd.h"

#if ENABLE_GEOLOCATION
  #include "gps.h"
#endif

#if ENABLE_IMPACT_DETECTION
  #include "accelerometer.h"
#endif

/* ═══════════════════════════════════════════
   GLOBAL SYSTEM STATE
   ═══════════════════════════════════════════ */

static bool chargingActive = false;
static bool fanActive      = false;

/* ── Accessors ── */
bool isChargingActive() { return chargingActive; }
bool isFanActive()      { return fanActive;      }

/* ═══════════════════════════════════════════
   BANNER
   ═══════════════════════════════════════════ */

void printSystemBanner() {
  Serial.println("=================================");
  Serial.println("        EV BMS SYSTEM");
  Serial.print  (" Firmware: "); Serial.println(FIRMWARE_VERSION);
  Serial.print  (" Device:   "); Serial.println(DEVICE_ID);
  Serial.println("=================================");
}

/* ═══════════════════════════════════════════
   INITIALIZATION
   ═══════════════════════════════════════════ */

void initializeAllSystems(float initialPackVoltage) {
  /* Core subsystems */
  initFaultManager();
  storageInit();

  /* Battery intelligence */
  initSOC(CELL_CAPACITY_AH, initialPackVoltage);
  initSOH();
  initRUL();

  /* Comms */
  wifiInit();
  gsmInit();
  telegramInit();

  /* Optional hardware */
#if ENABLE_GEOLOCATION
  initGPS();
#endif

#if ENABLE_IMPACT_DETECTION
  initAccelerometer();
#endif

  /* Default relay states per spec:
     - Charge relay OFF  (disabled until voltage triggers it)
     - Motor relay ON    (load enabled by default when safe)
     - Fan relay OFF     */
  digitalWrite(CHARGE_RELAY_PIN, LOW);
  digitalWrite(MOTOR_RELAY_PIN,  HIGH);   // motor enabled at startup
  digitalWrite(FAN_RELAY_PIN,    LOW);
}

/* ═══════════════════════════════════════════
   DIAGNOSTICS
   ═══════════════════════════════════════════ */

void performSystemDiagnostics() {
  Serial.println("--- SYSTEM DIAGNOSTICS ---");

  Serial.print("WiFi: ");
  Serial.println(wifiConnected() ? "Connected" : "⚠ Not connected");

  Serial.print("GSM:  ");
  Serial.println(gsmIsReady()    ? "Ready"     : "⚠ Not ready");

#if ENABLE_GEOLOCATION
  Serial.print("GPS:  ");
  Serial.println(gpsHealthy()    ? "Fix"       : "⚠ No fix");
#endif

  Serial.println("--- DIAGNOSTICS COMPLETE ---");
}

/* ═══════════════════════════════════════════
   SYSTEM HEALTH  (SOC + SOH + RUL)
   ═══════════════════════════════════════════ */

void updateSystemHealth(float currentA,
                        float packVoltage,
                        bool  fault,
                        float temp,
                        unsigned long cycleCount,
                        unsigned long dtMs) {

  /* SOC – Coulomb counting, voltage correction when idle */
  updateSOC(currentA, dtMs);
  correctSOCFromVoltage(packVoltage);

  /* SOH – temperature stress + fault events */
  updateSOH(currentA, temp, fault);

  /* RUL – multi-factor weighted estimate */
  updateRUL(packVoltage, temp, getSOH(), cycleCount);
}

/* ═══════════════════════════════════════════
   EXTERNAL EVENTS  (GPS, Impact)
   ═══════════════════════════════════════════ */

void checkExternalEvents() {

#if ENABLE_GEOLOCATION
  updateGPS();
  if (isGeofenceViolated()) {
    triggerExternalFault(FAULT_GEOFENCE_VIOLATION, "GEOFENCE VIOLATION");

    // Build alert with GPS coordinates
    char msg[160];
#if ENABLE_GEOLOCATION
    snprintf(msg, sizeof(msg),
             "BMS ALERT: Geo-fence violated!\nLat:%.5f Lon:%.5f",
             gpsGetLatitude(), gpsGetLongitude());
#else
    snprintf(msg, sizeof(msg), "BMS ALERT: Geo-fence violated!");
#endif
    sendTelegramAlert(String(msg));
    gsmSendSMS("BMS: GEOFENCE ALERT");
  }
#endif

#if ENABLE_IMPACT_DETECTION
  AccelData accel = readAccelerometer();
  if (accel.impactDetected || accel.shockDetected) {
    triggerExternalFault(FAULT_IMPACT_DETECTED, "IMPACT DETECTED");

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS ALERT: Impact detected!\nMag:%.2fg Shocks:%lu",
             accel.magnitude, (unsigned long)accel.shockCount);

#if ENABLE_GEOLOCATION
    char loc[64];
    snprintf(loc, sizeof(loc), "\nLat:%.5f Lon:%.5f",
             gpsGetLatitude(), gpsGetLongitude());
    strncat(msg, loc, sizeof(msg) - strlen(msg) - 1);
#endif
    sendTelegramAlert(String(msg));
    gsmSendSMS("BMS: IMPACT DETECTED");
  }
#endif
}

/* ═══════════════════════════════════════════
   CHARGING CONTROL
   Spec: Motor relay is cut while charging (interlock).
         Charge relay opens when fault OR pack ≥ CHARGE_STOP_V.
   ═══════════════════════════════════════════ */

void controlCharging(float packVoltage, bool fault) {

  /* Fault → immediately stop charging */
  if (fault) {
    if (chargingActive) {
      chargingActive = false;
      digitalWrite(CHARGE_RELAY_PIN, LOW);
      Serial.println("[CHG] Fault → charge relay OFF");
    }
    return;
  }

  /* Start charging when pack voltage is low enough */
  if (!chargingActive && packVoltage <= CHARGE_START_V) {
    chargingActive = true;
    digitalWrite(CHARGE_RELAY_PIN, HIGH);
    digitalWrite(MOTOR_RELAY_PIN,  LOW);    // interlock: motor OFF while charging
    Serial.println("[CHG] Charge relay ON  | Motor relay OFF (interlock)");
  }

  /* Stop charging when fully charged */
  if (chargingActive && packVoltage >= CHARGE_STOP_V) {
    chargingActive = false;
    digitalWrite(CHARGE_RELAY_PIN, LOW);
    incrementCycleCount();
    Serial.println("[CHG] Charge relay OFF | Cycle incremented");
  }
}

/* ═══════════════════════════════════════════
   MOTOR RELAY CONTROL
   Spec: Motor allowed only when not faulted AND not charging.
   ═══════════════════════════════════════════ */

void controlMotorRelay(bool fault, bool charging) {
  bool motorAllowed = (!fault && !charging);
  digitalWrite(MOTOR_RELAY_PIN, motorAllowed ? HIGH : LOW);
}

/* ═══════════════════════════════════════════
   THERMAL MANAGEMENT  (fan hysteresis)
   Spec: High temp → immediately enable fan.
         Fan stays ON during fault.
   ═══════════════════════════════════════════ */

void controlThermalManagement(float temperature, bool fault) {

  bool shouldBeOn = fault ||
                    (temperature >= FAN_ON_TEMP) ||
                    (fanActive && temperature >= FAN_OFF_TEMP);

  if (shouldBeOn && !fanActive) {
    fanActive = true;
    digitalWrite(FAN_RELAY_PIN, HIGH);
    Serial.printf("[FAN] ON  (T=%.1f°C fault=%d)\n", temperature, (int)fault);
  } else if (!shouldBeOn && fanActive) {
    fanActive = false;
    digitalWrite(FAN_RELAY_PIN, LOW);
    Serial.printf("[FAN] OFF (T=%.1f°C)\n", temperature);
  }
}

/* ═══════════════════════════════════════════
   SERIAL TELEMETRY
   ═══════════════════════════════════════════ */

void displayTelemetry(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      float soc,
                      bool  fault) {

  const char* dirStr =
    iData.direction == CURRENT_CHARGING    ? "CHARGING" :
    iData.direction == CURRENT_DISCHARGING ? "DISCHARGING" : "IDLE";

  Serial.println("===== TELEMETRY =====");
  Serial.printf("Voltage : %.2f V\n", packVoltage);
  Serial.printf("Current : %.2f A  [%s]\n", iData.current, dirStr);
  Serial.printf("Power   : %.1f W\n", iData.powerWatts);
  Serial.printf("Temp    : %.1f °C\n", temperature);
  Serial.printf("SOC     : %.1f %%\n", soc);
  Serial.printf("SOH     : %.1f %%\n", getSOH());
  Serial.printf("RUL     : %d cycles  (%lu days)\n",
                estimateRUL(), estimateRULDays());
  Serial.printf("Status  : %s\n", fault ? "FAULT" : "NORMAL");
  Serial.printf("Charging: %s  Fan: %s\n",
                chargingActive ? "ON" : "OFF",
                fanActive      ? "ON" : "OFF");
  Serial.println("=====================\n");
}

/* ═══════════════════════════════════════════
   CLOUD UPLOAD
   ═══════════════════════════════════════════ */

void uploadSystemData(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      bool  fault) {

  float lat = 0.0f;
  float lon = 0.0f;

#if ENABLE_GEOLOCATION
  lat = gpsGetLatitude();
  lon = gpsGetLongitude();
#endif

  uint32_t impacts = 0;
  uint32_t shocks  = 0;
#if ENABLE_IMPACT_DETECTION
  impacts = getImpactCount();
  shocks  = getShockCount();
#endif

  uploadComprehensiveTelemetry(
    packVoltage,
    iData.current,
    iData.powerWatts,
    temperature,
    getSOH(),
    estimateRUL(),
    fault,
    faultReason(),
    lat,
    lon,
    impacts,
    shocks,
    chargingActive,
    fanActive,
    digitalRead(CHARGE_RELAY_PIN),
    digitalRead(MOTOR_RELAY_PIN)
  );
}
