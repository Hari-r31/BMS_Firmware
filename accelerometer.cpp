#include "accelerometer.h"
#include "config.h"
#include <Wire.h>
#include <math.h>
#include <string.h>

/* ================= MPU6050 RAW REGISTERS ================= */

#define MPU_ADDR           0x68
#define REG_PWR_MGMT_1     0x6B
#define REG_ACCEL_XOUT_H   0x3B

/* ================= Detection Thresholds ================= */

// RELAXED + REALISTIC (MPU6050 clone friendly)
#define FREE_FALL_G          0.75
#define IMPACT_G             2.5
#define SHOCK_G              4.0

#define IMPACT_WINDOW_MS     250     // ESP32-safe
#define FREE_FALL_SAMPLES    3       // debounce (≈30 ms @100 Hz)

/* ================= Private State ================= */

static bool initialized = false;
static AccelData currentData;

static bool inFreeFall = false;
static unsigned long freeFallTime = 0;
static uint8_t freeFallCount = 0;

static uint32_t impactCount = 0;
static uint32_t shockCount  = 0;

/* ================= Low-level I2C ================= */

static void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static int16_t read16(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)2);
  return (Wire.read() << 8) | Wire.read();
}

/* ================= Init ================= */

void initAccelerometer() {
#if ENABLE_IMPACT_DETECTION
  if (initialized) return;

  Serial.println("[ACCEL] Initializing RAW MPU6050");

  Wire.begin(ACCEL_SDA, ACCEL_SCL);
  Wire.setClock(400000);

  writeReg(REG_PWR_MGMT_1, 0x00); // Wake up
  delay(100);

  memset(&currentData, 0, sizeof(currentData));
  initialized = true;

  Serial.println("[ACCEL] RAW MPU6050 ready");
#endif
}

/* ================= Read ================= */

AccelData readAccelerometer() {
  if (!initialized) initAccelerometer();
  if (!initialized) return currentData;

  int16_t ax = read16(REG_ACCEL_XOUT_H);
  int16_t ay = read16(REG_ACCEL_XOUT_H + 2);
  int16_t az = read16(REG_ACCEL_XOUT_H + 4);

  // ±2g → 16384 LSB/g
  currentData.accelX = ax / 16384.0;
  currentData.accelY = ay / 16384.0;
  currentData.accelZ = az / 16384.0;

  currentData.magnitude = sqrt(
    currentData.accelX * currentData.accelX +
    currentData.accelY * currentData.accelY +
    currentData.accelZ * currentData.accelZ
  );

  unsigned long now = millis();

  /* ================= FREE FALL (DEBOUNCED) ================= */

  if (currentData.magnitude < FREE_FALL_G) {
    if (freeFallCount < FREE_FALL_SAMPLES)
      freeFallCount++;

    if (freeFallCount >= FREE_FALL_SAMPLES && !inFreeFall) {
      inFreeFall = true;
      freeFallTime = now;
      currentData.freeFallDetected = true;
      Serial.println("[ACCEL] 🟦 FREE FALL");
    }
  } else {
    freeFallCount = 0;
    currentData.freeFallDetected = false;
  }

  /* ================= IMPACT (AFTER FREE FALL) ================= */

  if (inFreeFall &&
      currentData.magnitude > IMPACT_G &&
      (now - freeFallTime) <= IMPACT_WINDOW_MS) {

    currentData.impactDetected = true;
    impactCount++;
    inFreeFall = false;
    freeFallCount = 0;

    Serial.println("[ACCEL] ⚠️ IMPACT");
  } else {
    currentData.impactDetected = false;
  }

  /* ================= SHOCK (ANYTIME) ================= */

  if (currentData.magnitude > SHOCK_G) {
    currentData.shockDetected = true;
    shockCount++;
    inFreeFall = false;
    freeFallCount = 0;

    Serial.println("[ACCEL] 🚨 SHOCK");
  } else {
    currentData.shockDetected = false;
  }

  /* ================= TIMEOUT ================= */

  if (inFreeFall && (now - freeFallTime) > IMPACT_WINDOW_MS) {
    inFreeFall = false;
    freeFallCount = 0;
  }

  currentData.impactCount = impactCount;
  currentData.shockCount  = shockCount;

  return currentData;
}

/* ================= Helpers ================= */

float getAccelMagnitude(float x, float y, float z) {
  return sqrt(x*x + y*y + z*z);
}

bool checkImpact() {
  return readAccelerometer().impactDetected;
}

bool checkShock() {
  return readAccelerometer().shockDetected;
}

uint32_t getImpactCount() {
  return impactCount;
}

uint32_t getShockCount() {
  return shockCount;
}

void resetImpactCounters() {
  impactCount = 0;
  shockCount = 0;
}

bool accelerometerHealthy() {
  AccelData d = readAccelerometer();
  return (d.magnitude > 0.4 && d.magnitude < 2.5);
}

float getTiltAngle(const AccelData& data) {
  return acos(fabs(data.accelZ)) * 180.0 / PI;
}
