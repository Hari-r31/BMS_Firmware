/* ═══════════════════════════════════════════════════════════════════════════
   GPS – Hardware GPS (primary) + WiFi MAC geolocation (fallback)
   ─────────────────────────────────────────────────────────────────────────
   PRIORITY
   ─────────────────────────────────────────────────────────────────────────
   1. Hardware GPS module (UART1, TinyGPS++)
      Tried first on every update cycle.  If a valid fix exists (location age
      < 2 s) it is used and WiFi geo is skipped entirely.

   2. WiFi MAC geolocation  (BeaconDB – free, no API key)
      Activated automatically when hardware GPS has no fix.
      The ESP32 scans nearby APs → sends BSSIDs + RSSI to BeaconDB →
      receives lat/lon + accuracy estimate.
      Accuracy: 15–100 m in urban areas.

   GEOFENCE
   ─────────────────────────────────────────────────────────────────────────
   Geofence violation checking is done on the UI side.
   Firmware reports only raw coordinates.  The UI compares them against the
   saved home location and radius that the user configured.
   ═══════════════════════════════════════════════════════════════════════════ */

#include "gps.h"
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>

/* ── Hardware GPS (always compiled when ENABLE_HARDWARE_GPS is set) ── */
#if ENABLE_HARDWARE_GPS
  #include <TinyGPS++.h>
  #include <HardwareSerial.h>
  static TinyGPSPlus    hwGps;
  static HardwareSerial hwGpsSerial(1);
  /* Track whether the module has ever produced a sentence so we know
     whether it is actually wired up.                               */
  static bool    hwGpsModulePresent = false;
  static uint32_t hwGpsInitMs       = 0;
  /* Give the module this many ms to prove it exists before we decide
     it is absent and drop straight to WiFi geo.                    */
  #define HW_GPS_DETECT_TIMEOUT_MS  5000
#endif

/* ═══════════════════════════════════════════
   WiFi GEO CONFIGURATION
   ═══════════════════════════════════════════ */

/* Minimum APs in scan to attempt BeaconDB lookup */
#define MIN_APS_FOR_GEO        2

/* How often to run a WiFi geo fix when HW GPS is absent (ms) */
#define WIFI_GEO_INTERVAL_MS   30000UL

/* HTTP timeout for the geolocation API */
#define GEO_API_TIMEOUT_MS     8000

static const char* GEO_API_URL = "https://api.beacondb.net/v1/geolocate";

/* ═══════════════════════════════════════════
   STATE
   ═══════════════════════════════════════════ */

static bool          initialized  = false;
static GPSData       currentData  = {};
static unsigned long lastGeoMs    = 0;

#define EARTH_RADIUS_M 6371000.0f
static float toRadians(float deg) { return deg * (float)M_PI / 180.0f; }

/* ═══════════════════════════════════════════
   INIT
   ═══════════════════════════════════════════ */

void initGPS() {
  if (initialized) return;

  memset(&currentData, 0, sizeof(currentData));
  currentData.source = GPSData::Source::NONE;

#if ENABLE_HARDWARE_GPS
  hwGpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  hwGpsInitMs = millis();
  Serial.println("[GPS] Hardware GPS module initialised on UART1 (primary)");
#endif

  Serial.println("[GPS] WiFi geolocation ready as fallback (BeaconDB)");
  Serial.printf("[GPS] Device MAC: %s\n", WiFi.macAddress().c_str());

  initialized = true;
  Serial.println("[GPS] Initialized");
}

/* ═══════════════════════════════════════════
   HARDWARE GPS POLL
   Returns true if a fresh valid fix is available.
   ═══════════════════════════════════════════ */

static bool pollHardwareGPS() {
#if ENABLE_HARDWARE_GPS
  /* Drain UART buffer */
  while (hwGpsSerial.available()) {
    char c = hwGpsSerial.read();
    hwGps.encode(c);
    /* Any character means the module is wired and talking */
    if (!hwGpsModulePresent) {
      hwGpsModulePresent = true;
      Serial.println("[GPS] Hardware GPS module detected");
    }
  }

  /* If no NMEA data seen within the detect timeout, treat as absent */
  if (!hwGpsModulePresent &&
      (millis() - hwGpsInitMs) > HW_GPS_DETECT_TIMEOUT_MS) {
    return false;   // module not present → caller will use WiFi geo
  }

  /* Module present but not yet fixed */
  if (!hwGps.location.isValid() || hwGps.location.age() >= 2000) {
    return false;
  }

  /* Valid hardware fix ─ update shared state */
  currentData.valid      = true;
  currentData.latitude   = (float)hwGps.location.lat();
  currentData.longitude  = (float)hwGps.location.lng();
  currentData.accuracy   = 5.0f;   // hardware GPS ~3–5 m CEP
  currentData.altitude   = hwGps.altitude.isValid()   ? (float)hwGps.altitude.meters()    : 0.0f;
  currentData.speed      = hwGps.speed.isValid()      ? (float)hwGps.speed.kmph()         : 0.0f;
  currentData.satellites = hwGps.satellites.isValid() ? (uint8_t)hwGps.satellites.value() : 0;
  currentData.source     = GPSData::Source::HARDWARE_GPS;

  return true;

#else
  return false;   // hardware GPS not compiled in
#endif
}

/* ═══════════════════════════════════════════
   WiFi GEOLOCATION
   Scan nearby APs → POST to BeaconDB → parse lat/lon
   Returns true on a good fix.
   ═══════════════════════════════════════════ */

static bool wifiGeolocate() {
  if (WiFi.status() != WL_CONNECTED) return false;

  Serial.println("[GPS] HW GPS unavailable – trying WiFi geolocation...");

  int n = WiFi.scanNetworks(false, true, false, 100);

  if (n < MIN_APS_FOR_GEO) {
    Serial.printf("[GPS] Only %d APs found – cannot geolocate\n", n);
    WiFi.scanDelete();
    return false;
  }

  int count = min(n, 20);
  DynamicJsonDocument doc(50 + count * 90);
  JsonArray aps = doc.createNestedArray("wifiAccessPoints");

  for (int i = 0; i < count; i++) {
    JsonObject ap = aps.createNestedObject();
    ap["macAddress"]     = WiFi.BSSIDstr(i);
    ap["signalStrength"] = WiFi.RSSI(i);
    ap["channel"]        = WiFi.channel(i);
  }

  WiFi.scanDelete();

  String body;
  serializeJson(doc, body);

  NetworkClientSecure client;
  client.setInsecure();
  client.setTimeout(GEO_API_TIMEOUT_MS / 1000);

  HTTPClient http;
  http.setTimeout(GEO_API_TIMEOUT_MS);

  if (!http.begin(client, GEO_API_URL)) {
    Serial.println("[GPS] geo http.begin failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);

  if (code != 200) {
    Serial.printf("[GPS] Geo API returned HTTP %d\n", code);
    http.end();
    return false;
  }

  String resp = http.getString();
  http.end();

  DynamicJsonDocument resp_doc(256);
  if (deserializeJson(resp_doc, resp)) {
    Serial.println("[GPS] JSON parse error from geo API");
    return false;
  }

  float lat = resp_doc["location"]["lat"] | 0.0f;
  float lng = resp_doc["location"]["lng"] | 0.0f;
  float acc = resp_doc["accuracy"]        | 999.0f;

  if (lat == 0.0f && lng == 0.0f) {
    Serial.println("[GPS] Geo API returned 0,0 – no fix");
    return false;
  }

  currentData.valid      = true;
  currentData.latitude   = lat;
  currentData.longitude  = lng;
  currentData.accuracy   = acc;
  currentData.satellites = 0;
  currentData.altitude   = 0.0f;
  currentData.speed      = 0.0f;
  currentData.source     = GPSData::Source::WIFI_GEO;

  Serial.printf("[GPS] WiFi geo fix  lat=%.6f  lon=%.6f  acc=%.0fm\n",
                lat, lng, acc);
  return true;
}

/* ═══════════════════════════════════════════
   UPDATE  – called every loop
   ═══════════════════════════════════════════ */

void updateGPS() {
  if (!initialized) return;

  /* ── 1. Try hardware GPS first ── */
  bool hwFix = pollHardwareGPS();

  if (hwFix) {
    /* Good hardware fix – nothing else to do */
    return;
  }

  /* ── 2. Fall back to WiFi geolocation on its own interval ── */
  if (millis() - lastGeoMs >= WIFI_GEO_INTERVAL_MS) {
    lastGeoMs = millis();

    bool wfFix = wifiGeolocate();

    if (!wfFix) {
      /* Both sources failed */
      currentData.valid  = false;
      currentData.source = GPSData::Source::NONE;
      Serial.println("[GPS] No fix from hardware GPS or WiFi geo");
    }
  }

  /* NOTE: Geofence checking removed from firmware.
           The UI receives lat/lon and compares against user-configured
           home coordinates and radius.                              */
}

/* ═══════════════════════════════════════════
   GETTERS
   ═══════════════════════════════════════════ */

GPSData getGPSData()     { return currentData; }
bool    hasGPSFix()      { return currentData.valid; }
bool    gpsHealthy()     { return currentData.valid; }
float   gpsGetLatitude() { return currentData.valid ? currentData.latitude  : 0.0f; }
float   gpsGetLongitude(){ return currentData.valid ? currentData.longitude : 0.0f; }

/* ═══════════════════════════════════════════
   HAVERSINE DISTANCE  (kept as utility – UI may call via cloud)
   ═══════════════════════════════════════════ */

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  float dLat = toRadians(lat2 - lat1);
  float dLon = toRadians(lon2 - lon1);
  float a = sinf(dLat / 2.0f) * sinf(dLat / 2.0f)
          + cosf(toRadians(lat1)) * cosf(toRadians(lat2))
          * sinf(dLon / 2.0f) * sinf(dLon / 2.0f);
  return EARTH_RADIUS_M * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

/* ═══════════════════════════════════════════
   FORMATTING
   ═══════════════════════════════════════════ */

void getGPSLocationString(char* buffer, size_t size) {
  if (!currentData.valid) {
    snprintf(buffer, size, "GPS: NO FIX");
    return;
  }

  const char* src =
    currentData.source == GPSData::Source::HARDWARE_GPS ? "HW"   :
    currentData.source == GPSData::Source::WIFI_GEO     ? "WiFi" : "?";

  snprintf(buffer, size,
           "Lat:%.6f Lon:%.6f Acc:%.0fm [%s]",
           currentData.latitude, currentData.longitude,
           currentData.accuracy, src);
}
