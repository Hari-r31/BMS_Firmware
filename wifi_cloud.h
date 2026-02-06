#pragma once
#include <Arduino.h>

/* ================= WiFi ================= */

void wifiInit();
void wifiEnsure();
bool wifiConnected();

/* ================= Cloud Upload ================= */

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
);

/* ================= Status ================= */

unsigned long getUploadCount();
uint8_t getConnectionQuality();
