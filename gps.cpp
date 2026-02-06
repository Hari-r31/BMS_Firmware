#include "gps.h"
#include "config.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <math.h>

/* ================= Private ================= */

static TinyGPSPlus gps;
static HardwareSerial gpsSerial(1);
static bool initialized = false;

static GPSData currentData;

static bool geofenceEnabled = GEOFENCE_ENABLED;
static float homeLat = GEOFENCE_LAT;
static float homeLon = GEOFENCE_LON;

#define EARTH_RADIUS_M 6371000.0

static float toRadians(float deg) {
  return deg * PI / 180.0;
}

/* ================= Init ================= */

void initGPS() {
#if ENABLE_GPS
  if (initialized) return;

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  memset(&currentData, 0, sizeof(currentData));
  initialized = true;

  Serial.println("[GPS] Initialized");
#endif
}

/* ================= Update ================= */

void updateGPS() {
  if (!initialized) return;

  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    currentData.valid = true;
    currentData.latitude = gps.location.lat();
    currentData.longitude = gps.location.lng();

    if (gps.altitude.isValid())
      currentData.altitude = gps.altitude.meters();

    if (gps.speed.isValid())
      currentData.speed = gps.speed.kmph();

    if (gps.satellites.isValid())
      currentData.satellites = gps.satellites.value();

    if (geofenceEnabled) {
      currentData.distanceFromHome =
        calculateDistance(
          currentData.latitude,
          currentData.longitude,
          homeLat,
          homeLon
        );

      currentData.geofenceViolation =
        currentData.distanceFromHome > GEOFENCE_RADIUS_M;
    }
  } else {
    currentData.valid = false;
  }
}

/* ================= Getters ================= */

GPSData getGPSData() {
  return currentData;
}

bool hasGPSFix() {
  return currentData.valid && currentData.satellites >= 3;
}

bool isGeofenceViolated() {
  return geofenceEnabled && currentData.geofenceViolation;
}

bool gpsHealthy() {
  return hasGPSFix();
}

/* ================= Distance ================= */

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  float dLat = toRadians(lat2 - lat1);
  float dLon = toRadians(lon2 - lon1);

  float a =
    sin(dLat / 2) * sin(dLat / 2) +
    cos(toRadians(lat1)) * cos(toRadians(lat2)) *
    sin(dLon / 2) * sin(dLon / 2);

  return EARTH_RADIUS_M * 2 * atan2(sqrt(a), sqrt(1 - a));
}

/* ================= Formatting ================= */

void getGPSLocationString(char* buffer, size_t size) {
  if (currentData.valid) {
    snprintf(
      buffer, size,
      "Lat:%.6f Lon:%.6f Spd:%.1fkm/h Sat:%d",
      currentData.latitude,
      currentData.longitude,
      currentData.speed,
      currentData.satellites
    );
  } else {
    snprintf(buffer, size, "GPS: NO FIX");
  }
}

/* ================= GeoFence ================= */

void setGeofenceEnabled(bool enable) {
  geofenceEnabled = enable;
}

void setHomeLocation(float lat, float lon) {
  homeLat = lat;
  homeLon = lon;
}

/* ================= LEGACY WRAPPERS ================= */

// 🔧 THESE FIX YOUR COMPILATION ERROR

float gpsGetLatitude() {
  return currentData.valid ? currentData.latitude : 0.0;
}

float gpsGetLongitude() {
  return currentData.valid ? currentData.longitude : 0.0;
}
