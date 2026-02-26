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
  float   accuracy;         // metres – WiFi geo gives this; hardware GPS gives ~5 m
  uint8_t satellites;

  // Source of fix
  enum class Source { NONE, HARDWARE_GPS, WIFI_GEO } source;

  // NOTE: Geofence comparison is handled by the UI.
  //       Firmware only reports coordinates.
};

/* ═══════════════════════════════════════════
   CORE API
   ═══════════════════════════════════════════ */

void    initGPS();
void    updateGPS();
GPSData getGPSData();

bool    hasGPSFix();
bool    gpsHealthy();

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
