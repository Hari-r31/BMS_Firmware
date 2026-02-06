#include <WiFi.h>
#include <HTTPClient.h>
#include "wifi_cloud.h"
#include "config.h"

/* ================= Private ================= */

static unsigned long uploadCount = 0;
static unsigned long lastUploadTime = 0;
static uint8_t connectionQuality = 0;

/* ================= WiFi ================= */

void wifiInit() {
  Serial.println("[WIFI] Initializing...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void wifiEnsure() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 5000) return;
  lastCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Reconnecting...");
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
}

bool wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

/* ================= Cloud ================= */

void uploadComprehensiveTelemetry(
  float packVoltage,
  float current,
  float power,
  float tempPack,
  float soh,
  int rul,
  bool fault,
  const char* faultMessage,
  float latitude,
  float longitude,
  uint32_t impactCount,
  uint32_t shockCount
) {
  if (millis() - lastUploadTime < CLOUD_UPLOAD_INTERVAL_MS) return;
  if (!wifiConnected()) return;

  HTTPClient http;
  http.setTimeout(8000);
  http.begin(SUPABASE_URL);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("Prefer", "return=minimal");

  char body[768];
  snprintf(body, sizeof(body),
    "{"
    "\"device_id\":\"%s\","
    "\"timestamp\":%lu,"
    "\"pack_voltage\":%.2f,"
    "\"current\":%.2f,"
    "\"power\":%.2f,"
    "\"temp_pack\":%.1f,"
    "\"soh\":%.1f,"
    "\"rul\":%d,"
    "\"fault\":%s,"
    "\"fault_message\":\"%s\","
    "\"latitude\":%.6f,"
    "\"longitude\":%.6f,"
    "\"impact_count\":%lu,"
    "\"shock_count\":%lu"
    "}",
    DEVICE_ID,
    millis(),
    packVoltage,
    current,
    power,
    tempPack,
    soh,
    rul,
    fault ? "true" : "false",
    faultMessage ? faultMessage : "",
    latitude,
    longitude,
    impactCount,
    shockCount
  );

  int code = http.POST(body);

  if (code >= 200 && code < 300) {
    uploadCount++;
    lastUploadTime = millis();
    Serial.println("[CLOUD] ✓ Telemetry uploaded");
  } else {
    Serial.print("[CLOUD] ✗ Upload failed (");
    Serial.print(code);
    Serial.println(")");
  }

  http.end();
}

/* ================= Status ================= */

unsigned long getUploadCount() {
  return uploadCount;
}

uint8_t getConnectionQuality() {
  return connectionQuality;
}
