#pragma once

#include <Arduino.h>

/* ================= GPS Data Structure ================= */

struct GPSData {
  bool valid;
  float latitude;
  float longitude;
  float altitude;
  float speed;
  uint8_t satellites;
  bool geofenceViolation;
  float distanceFromHome;
};

/* ================= Core API ================= */

void initGPS();
void updateGPS();
GPSData getGPSData();

bool hasGPSFix();
bool isGeofenceViolated();
bool gpsHealthy();

void setGeofenceEnabled(bool enable);
void setHomeLocation(float lat, float lon);

/* ================= Utility ================= */

float calculateDistance(float lat1, float lon1, float lat2, float lon2);
void getGPSLocationString(char* buffer, size_t bufferSize);

/* ================= LEGACY HELPERS (FIX YOUR ERROR) ================= */

// These are REQUIRED because BMS_Firmware.ino calls them
float gpsGetLatitude();
float gpsGetLongitude();
