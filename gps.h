#pragma once
#include <Arduino.h>

/* ═══════════════════════════════════════════
   GPS DATA STRUCTURE
   ═══════════════════════════════════════════ */

struct GPSData {
  bool    valid;
  float   latitude;
  float   longitude;
  float   altitude;
  float   speed;
  float   accuracy;         // metres – WiFi geo gives this; hardware GPS gives 0
  uint8_t satellites;
  bool    geofenceViolation;
  float   distanceFromHome;

  // Source of fix
  enum class Source { NONE, WIFI_GEO, HARDWARE_GPS } source;
};

/* ═══════════════════════════════════════════
   CORE API  (unchanged – all callers still compile)
   ═══════════════════════════════════════════ */

void    initGPS();
void    updateGPS();
GPSData getGPSData();

bool    hasGPSFix();
bool    isGeofenceViolated();
bool    gpsHealthy();

void    setGeofenceEnabled(bool enable);
void    setHomeLocation(float lat, float lon);

/* ═══════════════════════════════════════════
   UTILITIES
   ═══════════════════════════════════════════ */

float calculateDistance(float lat1, float lon1, float lat2, float lon2);
void  getGPSLocationString(char* buffer, size_t bufferSize);

/* ═══════════════════════════════════════════
   LEGACY HELPERS
   ═══════════════════════════════════════════ */

float gpsGetLatitude();
float gpsGetLongitude();
