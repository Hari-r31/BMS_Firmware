#include "temperature.h"
#include "config.h"
#include "DHT.h"

/* ================= Private ================= */

static bool initialized = false;
static DHT dhtPack(TEMP_PACK_PIN, DHT11);
static unsigned long lastReadTime = 0;
static float lastTemp = 25.0;

/* ================= Helpers ================= */

static float readSensorSafe() {
  float t = dhtPack.readTemperature();

  if (isnan(t) || t < -20.0 || t > 80.0) {
    Serial.println("[TEMP] Invalid reading, using last value");
    return lastTemp;
  }

  lastTemp = t;
  return t;
}

/* ================= Public ================= */

void initTemperature() {
  if (initialized) return;

  Serial.println("[TEMP] Initializing pack temperature sensor...");
  dhtPack.begin();

  delay(2000); // DHT11 startup time
  initialized = true;

  Serial.println("[TEMP] Pack temperature monitoring ready");
}

float readPackTemperature() {
  if (!initialized) initTemperature();

  // DHT11 requires ~2s between reads
  if (millis() - lastReadTime < 2000) {
    return lastTemp;
  }

  lastReadTime = millis();
  return readSensorSafe();
}

float readTemperature() {
  return readPackTemperature();
}

TemperatureData readTemperatureData() {
  TemperatureData data;

  data.packTemp = readPackTemperature();
  data.overTempWarning = (data.packTemp >= MAX_PACK_TEMP);

  return data;
}

bool temperatureSystemHealthy() {
  float t = readPackTemperature();

  if (t < -20.0 || t > 80.0) {
    Serial.println("[TEMP] Sensor health check failed");
    return false;
  }

  return true;
}
