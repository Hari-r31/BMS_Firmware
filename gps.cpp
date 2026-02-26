/* ═══════════════════════════════════════════════════════════════════════════
   GPS – WiFi-based geolocation (primary) + hardware GPS (fallback)
   ─────────────────────────────────────────────────────────────────────────
   HOW IT WORKS
   ─────────────────────────────────────────────────────────────────────────
   The ESP32 scans nearby WiFi access points and collects their:
     • BSSID  – the AP's MAC address (unique hardware identifier)
     • RSSI   – signal strength (closer AP = stronger signal = better weight)

   These are sent to the BeaconDB geolocation API (free, no API key needed):
     POST https://api.beacondb.net/v1/geolocate
     Body: { "wifiAccessPoints": [ {"macAddress":"AA:BB:CC:DD:EE:FF",
                                     "signalStrength":-65}, ... ] }

   The API returns:
     { "location": {"lat": 12.9716, "lng": 77.5946}, "accuracy": 42 }

   Accuracy is typically 15–100 m in urban areas (comparable to A-GPS).
   Falls back to hardware GPS module (if wired on UART1) when WiFi scan
   finds fewer than MIN_APS_FOR_GEO access points.

   PRIVACY NOTE
   ─────────────────────────────────────────────────────────────────────────
   Only nearby AP MAC addresses are sent – NOT the ESP32's own MAC, and NOT
   any connected credentials. BeaconDB is an open-source community database.
   ═══════════════════════════════════════════════════════════════════════════ */

#include "gps.h"
#include "config.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>

#if ENABLE_GEOLOCATION && ENABLE_HARDWARE_GPS
  #include <TinyGPS++.h>
  #include <HardwareSerial.h>
  static TinyGPSPlus    hwGps;
  static HardwareSerial hwGpsSerial(1);
#endif

/* ═══════════════════════════════════════════
   CONFIGURATION
   ═══════════════════════════════════════════ */

/* Minimum access points in scan to attempt WiFi geolocation.
   Fewer APs → less accurate → fall through to hardware GPS. */
#define MIN_APS_FOR_GEO     2

/* How often to do a WiFi geo fix (ms).  Scanning temporarily pauses
   normal WiFi traffic so don't do it too often. */
#define WIFI_GEO_INTERVAL_MS   30000UL   // every 30 s

/* How long to wait for the geolocation API response */
#define GEO_API_TIMEOUT_MS  8000

/* BeaconDB – free, open-source, no API key required */
static const char* GEO_API_URL = "https://api.beacondb.net/v1/geolocate";

/* ═══════════════════════════════════════════
   STATE
   ═══════════════════════════════════════════ */

static bool     initialized    = false;
static GPSData  currentData    = {};
static bool     geofenceEnabled = GEOFENCE_ENABLED;
static float    homeLat         = GEOFENCE_LAT;
static float    homeLon         = GEOFENCE_LON;
static unsigned long lastGeoMs  = 0;

#define EARTH_RADIUS_M 6371000.0f
static float toRadians(float deg) { return deg * (float)M_PI / 180.0f; }

/* ═══════════════════════════════════════════
   INIT
   ═══════════════════════════════════════════ */

void initGPS() {
  if (initialized) return;

  memset(&currentData, 0, sizeof(currentData));
  currentData.source = GPSData::Source::NONE;

  Serial.printf("[GPS] ESP32 MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println("[GPS] WiFi geolocation ready (BeaconDB, no API key needed)");

#if ENABLE_GEOLOCATION && ENABLE_HARDWARE_GPS
  /* Optional hardware GPS on UART1 as fallback */
  hwGpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("[GPS] Hardware GPS module initialised on UART1 (fallback)");
#endif

  initialized = true;
  Serial.println("[GPS] Initialized");
}

/* ═══════════════════════════════════════════
   WIFI GEOLOCATION
   Scan nearby APs → build JSON → POST to BeaconDB → parse lat/lon
   ═══════════════════════════════════════════ */

static bool wifiGeolocate() {
  /* WiFi must be connected first (we just borrow the radio briefly) */
  if (WiFi.status() != WL_CONNECTED) return false;

  /* ── Scan nearby access points ── */
  Serial.println("[GPS] Scanning WiFi APs for geolocation...");

  /* Async=false, show_hidden=true, passive=false, max_ms_per_chan=100 */
  int n = WiFi.scanNetworks(false, true, false, 100);

  if (n < MIN_APS_FOR_GEO) {
    Serial.printf("[GPS] Only %d APs found – insufficient for geo fix\n", n);
    WiFi.scanDelete();
    return false;
  }

  Serial.printf("[GPS] Found %d APs – building geolocation request\n", n);

  /* ── Build JSON payload ── */
  /* Cap at 20 strongest APs to keep payload small */
  int count = min(n, 20);

  /* JSON: ~80 bytes per AP + 50 overhead */
  DynamicJsonDocument doc(50 + count * 90);
  JsonArray aps = doc.createNestedArray("wifiAccessPoints");

  for (int i = 0; i < count; i++) {
    JsonObject ap = aps.createNestedObject();
    ap["macAddress"]    = WiFi.BSSIDstr(i);    // e.g. "AA:BB:CC:DD:EE:FF"
    ap["signalStrength"] = WiFi.RSSI(i);        // e.g. -65
    /* Optional channel hint – helps API distinguish co-located APs */
    ap["channel"]       = WiFi.channel(i);
  }

  WiFi.scanDelete();   // free scan memory before HTTP call

  String body;
  serializeJson(doc, body);

  /* ── POST to BeaconDB ── */
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

  /* ── Parse response ── */
  String resp = http.getString();
  http.end();

  DynamicJsonDocument resp_doc(256);
  DeserializationError err = deserializeJson(resp_doc, resp);

  if (err) {
    Serial.printf("[GPS] JSON parse error: %s\n", err.c_str());
    return false;
  }

  float lat = resp_doc["location"]["lat"] | 0.0f;
  float lng = resp_doc["location"]["lng"] | 0.0f;
  float acc = resp_doc["accuracy"]        | 999.0f;

  if (lat == 0.0f && lng == 0.0f) {
    Serial.println("[GPS] Geo API returned 0,0 – no fix");
    return false;
  }

  /* ── Update state ── */
  currentData.valid      = true;
  currentData.latitude   = lat;
  currentData.longitude  = lng;
  currentData.accuracy   = acc;
  currentData.satellites = 0;     // not applicable for WiFi geo
  currentData.altitude   = 0.0f;
  currentData.speed      = 0.0f;
  currentData.source     = GPSData::Source::WIFI_GEO;

  Serial.printf("[GPS] WiFi geo fix  lat=%.6f  lon=%.6f  acc=%.0fm\n",
                lat, lng, acc);

  /* Log our own MAC so it's visible in serial alongside the fix */
  Serial.printf("[GPS] Device MAC: %s\n", WiFi.macAddress().c_str());

  return true;
}

/* ═══════════════════════════════════════════
   HARDWARE GPS POLL  (runs only if wired)
   ═══════════════════════════════════════════ */

static void pollHardwareGPS() {
#if ENABLE_GEOLOCATION && ENABLE_HARDWARE_GPS
  while (hwGpsSerial.available())
    hwGps.encode(hwGpsSerial.read());

  if (hwGps.location.isValid() && hwGps.location.age() < 2000) {
    currentData.valid      = true;
    currentData.latitude   = (float)hwGps.location.lat();
    currentData.longitude  = (float)hwGps.location.lng();
    currentData.accuracy   = 5.0f;   // hardware GPS ~3–5 m CEP
    currentData.altitude   = hwGps.altitude.isValid() ? (float)hwGps.altitude.meters() : 0.0f;
    currentData.speed      = hwGps.speed.isValid()    ? (float)hwGps.speed.kmph()      : 0.0f;
    currentData.satellites = hwGps.satellites.isValid() ? (uint8_t)hwGps.satellites.value() : 0;
    currentData.source     = GPSData::Source::HARDWARE_GPS;
  }
#endif
}

/* ═══════════════════════════════════════════
   GEOFENCE
   ═══════════════════════════════════════════ */

static void updateGeofence() {
  if (!geofenceEnabled || !currentData.valid) return;

  currentData.distanceFromHome = calculateDistance(
    currentData.latitude, currentData.longitude, homeLat, homeLon);

  currentData.geofenceViolation =
    (currentData.distanceFromHome > GEOFENCE_RADIUS_M);
}

/* ═══════════════════════════════════════════
   UPDATE  – called every loop
   ═══════════════════════════════════════════ */

void updateGPS() {
  if (!initialized) return;

  /* Always drain hardware GPS UART if wired */
  pollHardwareGPS();

  /* WiFi geo runs on its own interval (30 s) */
  if (millis() - lastGeoMs >= WIFI_GEO_INTERVAL_MS) {
    lastGeoMs = millis();

    bool ok = wifiGeolocate();

    /* If WiFi geo failed and we have no hardware fix either, mark invalid */
    if (!ok && currentData.source != GPSData::Source::HARDWARE_GPS) {
      currentData.valid = false;
    }
  }

  updateGeofence();
}

/* ═══════════════════════════════════════════
   GETTERS
   ═══════════════════════════════════════════ */

GPSData getGPSData()         { return currentData; }
bool    hasGPSFix()          { return currentData.valid; }
bool    isGeofenceViolated() { return geofenceEnabled && currentData.geofenceViolation; }
bool    gpsHealthy()         { return currentData.valid; }
float   gpsGetLatitude()     { return currentData.valid ? currentData.latitude  : 0.0f; }
float   gpsGetLongitude()    { return currentData.valid ? currentData.longitude : 0.0f; }

void setGeofenceEnabled(bool e)          { geofenceEnabled = e; }
void setHomeLocation(float lat, float lon) { homeLat = lat; homeLon = lon; }

/* ═══════════════════════════════════════════
   HAVERSINE DISTANCE
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
    currentData.source == GPSData::Source::WIFI_GEO    ? "WiFi" :
    currentData.source == GPSData::Source::HARDWARE_GPS ? "HW"   : "?";

  snprintf(buffer, size,
           "Lat:%.6f Lon:%.6f Acc:%.0fm [%s]",
           currentData.latitude, currentData.longitude,
           currentData.accuracy, src);
}
