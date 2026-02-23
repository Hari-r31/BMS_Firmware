#include "gps.h"
#include "config.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <math.h>

/* ================= Private ================= */

static TinyGPSPlus      gps;
static HardwareSerial   gpsSerial(1);   // UART1
static bool             initialized = false;
static GPSData          currentData;

static bool  geofenceEnabled = GEOFENCE_ENABLED;
static float homeLat         = GEOFENCE_LAT;
static float homeLon         = GEOFENCE_LON;

#define EARTH_RADIUS_M 6371000.0f

static float toRadians(float deg) { return deg * (float)M_PI / 180.0f; }

/* ================= Init ================= */

void initGPS() {
  if (initialized) return;

  /* GPS_RX / GPS_TX defined in config.h */
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  memset(&currentData, 0, sizeof(currentData));
  initialized = true;

  Serial.println("[GPS] Initialized (UART1)");
}

/* ================= Update – call every loop ================= */

void updateGPS() {
  if (!initialized) return;

  while (gpsSerial.available())
    gps.encode(gpsSerial.read());

  if (gps.location.isValid()) {
    currentData.valid     = true;
    currentData.latitude  = (float)gps.location.lat();
    currentData.longitude = (float)gps.location.lng();

    if (gps.altitude.isValid())
      currentData.altitude = (float)gps.altitude.meters();

    if (gps.speed.isValid())
      currentData.speed = (float)gps.speed.kmph();

    if (gps.satellites.isValid())
      currentData.satellites = (uint8_t)gps.satellites.value();

    if (geofenceEnabled) {
      currentData.distanceFromHome = calculateDistance(
        currentData.latitude, currentData.longitude, homeLat, homeLon);
      currentData.geofenceViolation =
        (currentData.distanceFromHome > GEOFENCE_RADIUS_M);
    }
  } else {
    currentData.valid = false;
  }
}

/* ================= Getters ================= */

GPSData getGPSData()         { return currentData; }
bool    hasGPSFix()          { return currentData.valid && currentData.satellites >= 3; }
bool    isGeofenceViolated() { return geofenceEnabled && currentData.geofenceViolation; }
bool    gpsHealthy()         { return hasGPSFix(); }

float gpsGetLatitude()  { return currentData.valid ? currentData.latitude  : 0.0f; }
float gpsGetLongitude() { return currentData.valid ? currentData.longitude : 0.0f; }

/* ================= Geofence control ================= */

void setGeofenceEnabled(bool enable)       { geofenceEnabled = enable; }
void setHomeLocation(float lat, float lon) { homeLat = lat; homeLon = lon; }

/* ================= Haversine distance (metres) ================= */

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  float dLat = toRadians(lat2 - lat1);
  float dLon = toRadians(lon2 - lon1);

  float a = sinf(dLat / 2.0f) * sinf(dLat / 2.0f) +
            cosf(toRadians(lat1)) * cosf(toRadians(lat2)) *
            sinf(dLon / 2.0f) * sinf(dLon / 2.0f);

  return EARTH_RADIUS_M * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

/* ================= Formatting ================= */

void getGPSLocationString(char* buffer, size_t size) {
  if (currentData.valid)
    snprintf(buffer, size, "Lat:%.6f Lon:%.6f Spd:%.1fkm/h Sat:%u",
             currentData.latitude, currentData.longitude,
             currentData.speed, currentData.satellites);
  else
    snprintf(buffer, size, "GPS: NO FIX");
}
