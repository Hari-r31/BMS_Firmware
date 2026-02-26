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
static bool thermalTripped = false;

bool isChargingActive() { return chargingActive; }
bool isFanActive()      { return fanActive;      }
bool isThermalTripped() { return thermalTripped; }

/* ═══════════════════════════════════════════
   INTERNAL ALERT HELPER
   Sends the same message via both Telegram and GSM SMS.
   smsShort must be ≤ 160 chars; telegramMsg can be longer.
   ═══════════════════════════════════════════ */

/* forceSend=true  → sendTelegramForced (boot, fault, charging, thermal)
   forceSend=false → sendTelegramAlert  (geofence, shock, free fall repeats) */
static void sendAlert(const char* telegramMsg, const char* smsShort,
                      bool forceSend = false) {
  if (forceSend)
    sendTelegramForced(String(telegramMsg));
  else
    sendTelegramAlert(String(telegramMsg));
  gsmSendSMS(smsShort);
}

/* ═══════════════════════════════════════════
   GPS LOCATION STRING  (shared helper)
   ═══════════════════════════════════════════ */

static void appendGPSLocation(char* buf, size_t bufSize) {
#if ENABLE_GEOLOCATION
  if (gpsGetLatitude() != 0.0f || gpsGetLongitude() != 0.0f) {
    char loc[64];
    snprintf(loc, sizeof(loc), "\nLocation: %.5f, %.5f",
             gpsGetLatitude(), gpsGetLongitude());
    strncat(buf, loc, bufSize - strlen(buf) - 1);
  }
#else
  (void)buf; (void)bufSize;
#endif
}

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

  pinMode(CHARGE_RELAY_PIN,      OUTPUT);
  pinMode(LOAD_MOTOR_RELAY_PIN,  OUTPUT);
  pinMode(COOLING_FAN_RELAY_PIN, OUTPUT);

  /* ALL relays OFF during init.
     Motor relay is enabled AFTER all subsystems are ready to prevent
     relay coil inrush from browning out the 3.3V rail mid-init. */
  digitalWrite(CHARGE_RELAY_PIN,      LOW);
  digitalWrite(LOAD_MOTOR_RELAY_PIN,  LOW);
  digitalWrite(COOLING_FAN_RELAY_PIN, LOW);

  initFaultManager();
  storageInit();

  initSOC(CELL_CAPACITY_AH, initialPackVoltage);
  initSOH();
  initRUL();

  wifiInit();
  gsmInit();
  telegramInit();

#if ENABLE_LOCAL_DISPLAY
  lcdInit();
#endif

#if ENABLE_GEOLOCATION
  initGPS();
#endif

#if ENABLE_IMPACT_DETECTION
  initAccelerometer();
#endif

  Serial.println("[SYS] All systems initialized");

  /* Enable motor relay now that 3.3V rail is stable and all init is done.
     200 ms delay lets capacitors on the relay driver fully charge first. */
  delay(200);
  digitalWrite(LOAD_MOTOR_RELAY_PIN, HIGH);
  Serial.println("[MOTOR] Relay enabled after init");

  /* ── Startup alert ── */
  char bootMsg[160];
  snprintf(bootMsg, sizeof(bootMsg),
           "BMS ONLINE [%s]\nFirmware: %s\nVoltage: %.2fV  SOC: %.1f%%",
           DEVICE_ID, FIRMWARE_VERSION,
           initialPackVoltage, getSOC());
  sendAlert(bootMsg, "BMS: DEVICE STARTED", true);   // boot – forced
}

/* ═══════════════════════════════════════════
   DIAGNOSTICS
   ═══════════════════════════════════════════ */

void performSystemDiagnostics() {
  Serial.println("--- SYSTEM DIAGNOSTICS ---");
  Serial.printf("WiFi : %s\n", wifiConnected() ? "Connected" : "NOT connected");
  Serial.printf("GSM  : %s\n", gsmIsReady()    ? "Ready"     : "NOT ready");
#if ENABLE_GEOLOCATION
  Serial.printf("GPS  : %s\n", gpsHealthy()    ? "Fix OK"    : "No fix");
#endif
  Serial.printf("SOH  : %.1f%%\n", getSOH());
  Serial.printf("RUL  : %d cycles / %lu days\n", estimateRUL(), estimateRULDays());
  Serial.printf("Faults stored: %lu\n", getFaultCount());
  Serial.println("--- DIAGNOSTICS DONE ---");
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
  updateSOC(currentA, dtMs);
  correctSOCFromVoltage(packVoltage);
  updateSOH(currentA, temp, fault);
  updateRUL(packVoltage, temp, getSOH(), cycleCount);
}

/* ═══════════════════════════════════════════
   EXTERNAL EVENTS  (GPS / Accelerometer)
   ─────────────────────────────────────────────
   Three separate accelerometer events:
     1. FREE FALL  – magnitude < 0.3g for 3 consecutive samples
     2. IMPACT     – free fall followed by sudden deceleration > 2.5g
     3. SHOCK      – any single reading > 4.0g
   Each has its own cooldown so they don't block each other.
   GPS location appended when available.
   ═══════════════════════════════════════════ */

void checkExternalEvents() {

  static unsigned long lastGeofenceAlertMs = 0;
  static unsigned long lastFreeFallAlertMs = 0;
  static unsigned long lastImpactAlertMs   = 0;
  static unsigned long lastShockAlertMs    = 0;

  /* Separate cooldowns per event type */
  static const unsigned long GEOFENCE_COOLDOWN_MS  = 60000UL;  // 60 s
  static const unsigned long FREEFALL_COOLDOWN_MS  = 30000UL;  // 30 s
  static const unsigned long IMPACT_COOLDOWN_MS    = 10000UL;  // 10 s (serious – shorter)
  static const unsigned long SHOCK_COOLDOWN_MS     = 10000UL;  // 10 s


  /* ── Accelerometer ── */
#if ENABLE_IMPACT_DETECTION
  AccelData accel = readAccelerometer();

  /* 1. FREE FALL */
  if (accel.freeFallDetected) {
    if (millis() - lastFreeFallAlertMs >= FREEFALL_COOLDOWN_MS) {
      lastFreeFallAlertMs = millis();

      char msg[200];
      snprintf(msg, sizeof(msg),
               "BMS ALERT [%s]\nFREE FALL DETECTED\nMagnitude: %.2fg",
               DEVICE_ID, accel.magnitude);
      appendGPSLocation(msg, sizeof(msg));

      sendAlert(msg, "BMS: FREE FALL DETECTED");
      Serial.printf("[ACCEL ALERT] Free fall  mag=%.2fg\n", accel.magnitude);
    }
  }

  /* 2. IMPACT (free fall followed by hard deceleration) */
  if (accel.impactDetected) {
    triggerExternalFault(FAULT_IMPACT_DETECTED, "IMPACT DETECTED");

    if (millis() - lastImpactAlertMs >= IMPACT_COOLDOWN_MS) {
      lastImpactAlertMs = millis();

      char msg[200];
      snprintf(msg, sizeof(msg),
               "BMS ALERT [%s]\nIMPACT DETECTED\nMagnitude: %.2fg  Total impacts: %u",
               DEVICE_ID, accel.magnitude, (unsigned int)accel.impactCount);
      appendGPSLocation(msg, sizeof(msg));

      sendAlert(msg, "BMS: IMPACT DETECTED");
      Serial.printf("[ACCEL ALERT] Impact  mag=%.2fg  total=%u\n",
                    accel.magnitude, (unsigned int)accel.impactCount);
    }
  }

  /* 3. SHOCK (high-g spike – no free fall required) */
  if (accel.shockDetected) {
    triggerExternalFault(FAULT_IMPACT_DETECTED, "SHOCK DETECTED");

    if (millis() - lastShockAlertMs >= SHOCK_COOLDOWN_MS) {
      lastShockAlertMs = millis();

      char msg[200];
      snprintf(msg, sizeof(msg),
               "BMS ALERT [%s]\nSHOCK / SPIKE DETECTED\nMagnitude: %.2fg  Total shocks: %u",
               DEVICE_ID, accel.magnitude, (unsigned int)accel.shockCount);
      appendGPSLocation(msg, sizeof(msg));

      sendAlert(msg, "BMS: SHOCK DETECTED");
      Serial.printf("[ACCEL ALERT] Shock  mag=%.2fg  total=%u\n",
                    accel.magnitude, (unsigned int)accel.shockCount);
    }
  }
#endif
}

/* ═══════════════════════════════════════════
   CHARGING CONTROL
   ─────────────────────────────────────────────
   Alerts sent on:
     - Charging STARTED  (charger connected + current flowing in)
     - Charging COMPLETE (pack reached CHARGE_STOP_V)
     - Charging STOPPED  by fault or thermal trip
   ═══════════════════════════════════════════ */

void controlCharging(float packVoltage, bool fault) {

  /* Fault OR thermal trip → immediately cut charge relay */
  if (fault || thermalTripped) {
    if (chargingActive) {
      chargingActive = false;
      digitalWrite(CHARGE_RELAY_PIN, LOW);

      const char* reason = fault ? "fault" : "high temperature";
      char msg[160];
      snprintf(msg, sizeof(msg),
               "BMS ALERT [%s]\nCHARGING STOPPED\nReason: %s\nVoltage: %.2fV",
               DEVICE_ID, reason, packVoltage);
      sendAlert(msg, "BMS: CHARGING STOPPED", true);

      Serial.printf("[CHG] Stopped by %s → relay OFF\n", reason);
    }
    return;
  }

  /* Start charging – relay ready alert */
  if (!chargingActive && packVoltage <= CHARGE_START_V) {
    chargingActive = true;
    digitalWrite(CHARGE_RELAY_PIN, HIGH);

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS INFO [%s]\nBATTERY READY TO CHARGE\nYou can now connect your charger\nVoltage: %.2fV  SOC: %.1f%%",
             DEVICE_ID, packVoltage, getSOC());
    sendAlert(msg, "BMS: YOU CAN CONNECT CHARGER", true);

    Serial.println("[CHG] Charge relay ON – ready alert sent");
  }

  /* Charging complete */
  if (chargingActive && packVoltage >= CHARGE_STOP_V) {
    chargingActive = false;
    digitalWrite(CHARGE_RELAY_PIN, LOW);
    incrementCycleCount();

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS INFO [%s]\nCHARGING COMPLETE\nVoltage: %.2fV  SOC: %.1f%%  Cycles: %lu",
             DEVICE_ID, packVoltage, getSOC(), getCycleCount());
    sendAlert(msg, "BMS: CHARGING COMPLETE", true);

    Serial.println("[CHG] Charge relay OFF – charging complete – alert sent");
  }
}

/* ═══════════════════════════════════════════
   CHARGING CURRENT MONITOR
   Purely based on live INA219 current reading.
   Sends two distinct alerts:
     1. Current starts flowing IN  → "Charging is in progress"
     2. Current stops flowing IN   → "Charging current stopped"
   ═══════════════════════════════════════════ */

#define CHG_CURRENT_THRESHOLD  -0.2f   // A  current more negative than this = charging

void monitorChargingCurrent(float currentA, float packVoltage) {

  static bool wasChargingCurrent = false;

  bool isChargingCurrent = (currentA < CHG_CURRENT_THRESHOLD);

  /* Current just started flowing INTO battery */
  if (isChargingCurrent && !wasChargingCurrent) {
    wasChargingCurrent = true;

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS INFO [%s]\nCHARGING IN PROGRESS\nCurrent: %.2fA  Voltage: %.2fV  SOC: %.1f%%",
             DEVICE_ID, fabsf(currentA), packVoltage, getSOC());
    sendAlert(msg, "BMS: CHARGING IN PROGRESS", true);

    Serial.printf("[CHG] Current flowing IN (%.2fA) – in-progress alert sent\n",
                  fabsf(currentA));
  }

  /* Current stopped flowing INTO battery */
  if (!isChargingCurrent && wasChargingCurrent) {
    wasChargingCurrent = false;

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS INFO [%s]\nCHARGING CURRENT STOPPED\nVoltage: %.2fV  SOC: %.1f%%",
             DEVICE_ID, packVoltage, getSOC());
    sendAlert(msg, "BMS: CHARGING CURRENT STOPPED", true);

    Serial.printf("[CHG] Current no longer flowing in – stopped alert sent\n");
  }
}

/* ═══════════════════════════════════════════
   MOTOR RELAY CONTROL
   Based purely on live INA219 current.
   ═══════════════════════════════════════════ */

#define MOTOR_CHARGE_CURRENT_THRESHOLD  -0.2f

/* Timestamp of last motor-ON transition – used for inrush blanking */
static unsigned long motorOnTimeMs = 0;

/* How long to blank fault/sensor evaluation after motor energises.
   Motor inrush typically lasts < 200 ms; 500 ms gives plenty of margin. */
#define MOTOR_START_BLANK_MS  500UL

bool isMotorStartBlanking() {
  return (motorOnTimeMs > 0 &&
          (millis() - motorOnTimeMs) < MOTOR_START_BLANK_MS);
}

void controlMotorRelay(bool fault, float currentA) {

  bool actuallyCharging = (currentA < MOTOR_CHARGE_CURRENT_THRESHOLD);
  bool allow = !fault && !thermalTripped && !actuallyCharging;

  static bool lastState = true;
  if (allow != lastState) {
    digitalWrite(LOAD_MOTOR_RELAY_PIN, allow ? HIGH : LOW);

    if (allow) {
      /* Motor just turned ON – start blanking window */
      motorOnTimeMs = millis();
      Serial.println("[MOTOR] ON – inrush blanking started");
    } else {
      motorOnTimeMs = 0;
      Serial.printf("[MOTOR] OFF  (fault=%d trip=%d current=%.2fA)\n",
                    (int)fault, (int)thermalTripped, currentA);
    }
    lastState = allow;
  } else {
    digitalWrite(LOAD_MOTOR_RELAY_PIN, allow ? HIGH : LOW);
  }
}

/* ═══════════════════════════════════════════
   THERMAL MANAGEMENT
   ─────────────────────────────────────────────
   Fan ON  : >= FAN_ON_TEMP  (40°C)
   Fan OFF : <  FAN_OFF_TEMP (35°C)
   TRIP    : >= MAX_CELL_TEMP (60°C) → cut both relays + alert
   CLEAR   : <  FAN_OFF_TEMP  (35°C) → unlock relays + alert
   ═══════════════════════════════════════════ */

#define THERMAL_TRIP_TEMP   MAX_CELL_TEMP
#define THERMAL_CLEAR_TEMP  FAN_OFF_TEMP

void controlThermalManagement(float temperature, bool fault) {

  /* ── THERMAL TRIP ── */
  if (!thermalTripped && temperature >= THERMAL_TRIP_TEMP) {
    thermalTripped = true;

    digitalWrite(CHARGE_RELAY_PIN,     LOW);
    digitalWrite(LOAD_MOTOR_RELAY_PIN, LOW);
    if (chargingActive) chargingActive = false;

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS ALERT [%s]\nTHERMAL PROTECTION ACTIVE\nTemp: %.1fC  Both relays CUT",
             DEVICE_ID, temperature);
    sendAlert(msg, "BMS: THERMAL PROTECTION ON", true);

    Serial.printf("[THERMAL] TRIP at %.1fC – both relays OFF – alert sent\n", temperature);
  }

  /* ── THERMAL CLEAR ── */
  if (thermalTripped && temperature < THERMAL_CLEAR_TEMP) {
    thermalTripped = false;

    char msg[160];
    snprintf(msg, sizeof(msg),
             "BMS INFO [%s]\nTHERMAL PROTECTION CLEARED\nTemp: %.1fC  Relays restored",
             DEVICE_ID, temperature);
    sendAlert(msg, "BMS: THERMAL PROTECTION OFF", true);

    Serial.printf("[THERMAL] CLEARED at %.1fC – relays unlocked – alert sent\n", temperature);
  }

  /* ── FAN ── */
  bool shouldBeOn = fault ||
                    thermalTripped ||
                    (temperature >= FAN_ON_TEMP) ||
                    (fanActive && temperature >= FAN_OFF_TEMP);

  if (shouldBeOn && !fanActive) {
    fanActive = true;
    digitalWrite(COOLING_FAN_RELAY_PIN, HIGH);
    Serial.printf("[FAN] ON  (T=%.1fC fault=%d trip=%d)\n",
                  temperature, (int)fault, (int)thermalTripped);
  } else if (!shouldBeOn && fanActive) {
    fanActive = false;
    digitalWrite(COOLING_FAN_RELAY_PIN, LOW);
    Serial.printf("[FAN] OFF (T=%.1fC)\n", temperature);
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

  float displayCurrent = (fabsf(iData.current) < 0.005f) ? 0.0f : iData.current;

  const char* dirStr =
    iData.direction == CURRENT_CHARGING    ? "CHARGING"    :
    iData.direction == CURRENT_DISCHARGING ? "DISCHARGING" : "IDLE";

  Serial.println("===== TELEMETRY =====");
  Serial.printf("Voltage  : %.2f V\n",       packVoltage);
  Serial.printf("Current  : %.2f A  [%s]\n", displayCurrent, dirStr);
  Serial.printf("Power    : %.1f W\n",        iData.powerWatts);
  Serial.printf("Temp     : %.1f C\n",        temperature);
  Serial.printf("SOC      : %.1f %%\n",       soc);
  Serial.printf("SOH      : %.1f %%\n",       getSOH());
  Serial.printf("RUL      : %d cycles  (%lu days)\n",
                estimateRUL(), estimateRULDays());
  Serial.printf("Status   : %s\n",            fault ? "FAULT" : "NORMAL");
  Serial.printf("Charging : %s  Fan: %s  ThermalTrip: %s\n",
                chargingActive  ? "ON" : "OFF",
                fanActive       ? "ON" : "OFF",
                thermalTripped  ? "YES" : "NO");
  Serial.println("=====================\n");
}

/* ═══════════════════════════════════════════
   CLOUD UPLOAD
   ═══════════════════════════════════════════ */

void uploadSystemData(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      float soc,
                      bool  fault) {

  float lat = 0.0f, lon = 0.0f;
#if ENABLE_GEOLOCATION
  lat = gpsGetLatitude();
  lon = gpsGetLongitude();
#endif

  uint32_t impacts = 0, shocks = 0;
#if ENABLE_IMPACT_DETECTION
  impacts = getImpactCount();
  shocks  = getShockCount();
#endif

  uploadComprehensiveTelemetry(
    packVoltage,
    iData.current,
    iData.powerWatts,
    temperature,
    soc,
    getSOH(),
    estimateRUL(),
    fault,
    faultReason(),
    lat, lon,
    impacts, shocks,
    chargingActive,
    fanActive,
    (bool)digitalRead(CHARGE_RELAY_PIN),
    (bool)digitalRead(LOAD_MOTOR_RELAY_PIN)
  );
}
