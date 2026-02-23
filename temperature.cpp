#include "temperature.h"
#include "config.h"
#include <DHT.h>

/* ================= Private ================= */

static DHT           dhtPack(TEMP_PACK_PIN, DHT11);
static bool          initialized  = false;
static float         lastTemp     = 25.0f;
static unsigned long lastReadTime = 0;

#define DHT_MIN_INTERVAL_MS  2000UL   // DHT11 requires ≥2 s between reads

/* ================= Init ================= */

void initTemperature() {
  if (initialized) return;

  dhtPack.begin();
  delay(2000);   // DHT11 startup stabilization

  initialized = true;
  Serial.println("[TEMP] DHT11 initialized");
}

/* ================= Read ================= */

float readPackTemperature() {
  if (!initialized) initTemperature();

  if (millis() - lastReadTime < DHT_MIN_INTERVAL_MS)
    return lastTemp;

  lastReadTime = millis();

  float t = dhtPack.readTemperature();
  if (isnan(t) || t < -20.0f || t > 85.0f) {
    Serial.println("[TEMP] Invalid reading – using last value");
    return lastTemp;
  }

  lastTemp = t;
  return t;
}

float readTemperature() { return readPackTemperature(); }

TemperatureData readTemperatureData() {
  TemperatureData d;
  d.packTemp       = readPackTemperature();
  d.overTempWarning = (d.packTemp >= MAX_PACK_TEMP);
  return d;
}

bool temperatureSystemHealthy() {
  float t = readPackTemperature();
  return (t >= -20.0f && t <= 85.0f);
}
