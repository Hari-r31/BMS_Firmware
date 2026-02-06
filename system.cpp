#include "system.h"
#include "config.h"
#include "fault_manager.h"
#include "soh.h"
#include "rul.h"
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

/* ================= Banner ================= */
void printSystemBanner() {
  Serial.println("=================================");
  Serial.println("        EV BMS SYSTEM");
  Serial.print  (" Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print  (" Device ID: ");
  Serial.println(DEVICE_ID);
  Serial.println("=================================");
}

/* ================= Initialization ================= */
void initializeAllSystems() {
  initFaultManager();
  initSOH();
  initRUL();

  wifiInit();
  gsmInit();
  storageInit();


#if ENABLE_GEOLOCATION
  initGPS();
#endif

#if ENABLE_IMPACT_DETECTION
  initAccelerometer();
#endif
}

/* ================= Diagnostics ================= */
void performSystemDiagnostics() {
  if (!wifiConnected()) {
    Serial.println("⚠ WiFi not connected");
  }

  if (!gsmIsReady()) {
    Serial.println("⚠ GSM not ready");
  }

#if ENABLE_GEOLOCATION
  if (!gpsHealthy()) {
    Serial.println("⚠ GPS not healthy");
  }
#endif
}

/* ================= Health ================= */
void updateSystemHealth(bool fault, float temp, unsigned long cycleCount) {
  updateSOH(0.0, temp, fault);

  updateRUL(
    0.0,
    temp,
    getSOH(),
    cycleCount
  );
}

/* ================= External Events ================= */
void checkExternalEvents() {

#if ENABLE_GEOLOCATION
  if (isGeofenceViolated()) {
    triggerExternalFault(
      FAULT_GEOFENCE_VIOLATION,
      "GEOFENCE VIOLATION"
    );
  }
#endif

#if ENABLE_IMPACT_DETECTION
  if (getImpactCount() > 0) {
    triggerExternalFault(
      FAULT_IMPACT_DETECTED,
      "IMPACT DETECTED"
    );
  }
#endif
}

/* ================= Charging ================= */
void controlCharging(float packVoltage, bool fault) {
  static bool chargingActive = false;

  if (fault) {
    chargingActive = false;
    digitalWrite(CHARGE_RELAY_PIN, LOW);
    return;
  }

  if (!chargingActive && packVoltage <= CHARGE_START_V) {
    chargingActive = true;
    digitalWrite(CHARGE_RELAY_PIN, HIGH);
  }

  if (chargingActive && packVoltage >= CHARGE_STOP_V) {
    chargingActive = false;
    digitalWrite(CHARGE_RELAY_PIN, LOW);
    incrementCycleCount();
  }
}

/* ================= Thermal ================= */
void controlThermalManagement(float temperature, bool fault) {
  static bool fanActive = false;

  if ((temperature >= FAN_ON_TEMP || fault) && !fanActive) {
    fanActive = true;
    digitalWrite(FAN_RELAY_PIN, HIGH);
  }

  if (temperature < FAN_OFF_TEMP && !fault && fanActive) {
    fanActive = false;
    digitalWrite(FAN_RELAY_PIN, LOW);
  }
}

/* ================= Telemetry ================= */
void displayTelemetry(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      bool fault) {

  Serial.println("===== TELEMETRY =====");
  Serial.print("Voltage: "); Serial.print(packVoltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(iData.current); Serial.println(" A");
  Serial.print("Temp: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Status: "); Serial.println(fault ? "FAULT" : "NORMAL");
  Serial.println("=====================\n");
}

/* ================= Cloud ================= */
void uploadSystemData(float packVoltage,
                      const CurrentData& iData,
                      float temperature,
                      bool fault) {

  uploadComprehensiveTelemetry(
    packVoltage,
    iData.current,
    iData.powerWatts,
    temperature,
    getSOH(),
    estimateRUL(),
    fault,
    faultReason(),
    gpsGetLatitude(),
    gpsGetLongitude(),
    getImpactCount(),
    getShockCount()
  );
}
