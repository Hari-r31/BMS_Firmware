#include "system.h"
#include "config.h"
#include "voltage.h"
#include "current.h"
#include "temperature.h"
#include "fault_manager.h"
#include "wifi_cloud.h"
#include "lcd.h"
#include "nvs_logger.h"
#include "accelerometer.h"


/* ================= Setup ================= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  printSystemBanner();

  lcdInit();
  pinMode(CHARGE_RELAY_PIN, OUTPUT);
  pinMode(FAN_RELAY_PIN, OUTPUT);

  digitalWrite(CHARGE_RELAY_PIN, LOW);
  digitalWrite(FAN_RELAY_PIN, LOW);

  initializeAllSystems();
  performSystemDiagnostics();

  Serial.println("✓ BMS SYSTEM READY");
}

/* ================= Main Loop ================= */
void loop() {
  static bool lastFaultState = false;

  #if ENABLE_IMPACT_DETECTION
    static unsigned long lastAccel = 0;
    if (millis() - lastAccel >= 10) {   // 100 Hz
      lastAccel = millis();
      readAccelerometer();
    }
  #endif

  wifiEnsure();

  float packVoltage = readPackVoltage();
  CurrentData iData = readCurrentData();
  float temperature = readPackTemperature();

  iData.direction = digitalRead(CHARGE_RELAY_PIN)
                    ? CURRENT_CHARGING
                    : CURRENT_DISCHARGING;

  iData.overcurrentWarning =
    checkOvercurrent(iData.current, iData.direction);

  evaluateSystemFaults(
    packVoltage,
    packVoltage / NUM_CELLS,
    packVoltage / NUM_CELLS,
    0.0,
    iData.current,
    iData.overcurrentWarning,
    temperature,
    temperature
  );

  bool fault = isFaulted();

  controlCharging(packVoltage, fault);
  controlThermalManagement(temperature, fault);

  updateSystemHealth(fault, temperature, getCycleCount());

  lcdUpdate(packVoltage, iData.current, temperature, fault);
  displayTelemetry(packVoltage, iData, temperature, fault);
  uploadSystemData(packVoltage, iData, temperature, fault);

  checkExternalEvents();

  lastFaultState = fault;
  delay(SENSOR_READ_INTERVAL_MS);
}
