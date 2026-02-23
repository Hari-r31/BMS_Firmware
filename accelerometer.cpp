#include "accelerometer.h"
#include "config.h"
#include <Wire.h>
#include <math.h>
#include <string.h>

/* ================= MPU6050 Registers ================= */

#define MPU_ADDR          0x68
#define REG_PWR_MGMT_1    0x6B
#define REG_ACCEL_XOUT_H  0x3B

/* ================= Detection Thresholds ================= */

#define FREE_FALL_G       0.30f   // magnitude < this → free fall candidate
#define IMPACT_G          2.50f   // magnitude > this after free fall → impact
#define SHOCK_G           4.00f   // magnitude > this any time → direct shock

#define IMPACT_WINDOW_MS  250     // max ms between free-fall end and impact
#define FREE_FALL_SAMPLES 3       // consecutive samples required (~30 ms @100 Hz)

/* ================= Private State ================= */

static bool     initialized   = false;
static AccelData currentData;

static bool     inFreeFall    = false;
static unsigned long freeFallTime = 0;
static uint8_t  freeFallCount = 0;

static uint32_t impactCount   = 0;
static uint32_t shockCount    = 0;

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
  return (int16_t)((Wire.read() << 8) | Wire.read());
}

/* ================= Init ================= */

void initAccelerometer() {
  if (initialized) return;

  Serial.println("[ACCEL] Initializing MPU6050");

  Wire.begin(ACCEL_SDA, ACCEL_SCL);
  Wire.setClock(400000);

  writeReg(REG_PWR_MGMT_1, 0x00);  // wake up, use internal 8 MHz oscillator
  delay(100);

  memset(&currentData, 0, sizeof(currentData));
  initialized = true;

  Serial.println("[ACCEL] MPU6050 ready");
}

/* ================= Read (single authoritative call) ================= */

AccelData readAccelerometer() {
  if (!initialized) initAccelerometer();

  /* --- Raw ADC → g  (±2g range → 16384 LSB/g) --- */
  int16_t ax = read16(REG_ACCEL_XOUT_H);
  int16_t ay = read16(REG_ACCEL_XOUT_H + 2);
  int16_t az = read16(REG_ACCEL_XOUT_H + 4);

  currentData.accelX    = ax / 16384.0f;
  currentData.accelY    = ay / 16384.0f;
  currentData.accelZ    = az / 16384.0f;
  currentData.magnitude = sqrtf(
    currentData.accelX * currentData.accelX +
    currentData.accelY * currentData.accelY +
    currentData.accelZ * currentData.accelZ
  );

  unsigned long now = millis();

  /* ── Reset per-call event flags ── */
  currentData.freeFallDetected = false;
  currentData.impactDetected   = false;
  currentData.shockDetected    = false;

  /* ── FREE FALL (debounced) ── */
  if (currentData.magnitude < FREE_FALL_G) {
    if (freeFallCount < FREE_FALL_SAMPLES) freeFallCount++;

    if (freeFallCount >= FREE_FALL_SAMPLES && !inFreeFall) {
      inFreeFall   = true;
      freeFallTime = now;
      currentData.freeFallDetected = true;
      Serial.println("[ACCEL] FREE FALL detected");
    }
  } else {
    freeFallCount = 0;
  }

  /* ── IMPACT (free-fall → sudden deceleration) ── */
  if (inFreeFall &&
      currentData.magnitude > IMPACT_G &&
      (now - freeFallTime) <= IMPACT_WINDOW_MS) {

    currentData.impactDetected = true;
    impactCount++;
    inFreeFall    = false;
    freeFallCount = 0;
    Serial.printf("[ACCEL] IMPACT detected (mag=%.2fg, total=%u)\n",
                  currentData.magnitude, impactCount);
  }

  /* ── SHOCK (high-g any time) ── */
  if (currentData.magnitude > SHOCK_G) {
    currentData.shockDetected = true;
    shockCount++;
    inFreeFall    = false;
    freeFallCount = 0;
    Serial.printf("[ACCEL] SHOCK detected (mag=%.2fg, total=%u)\n",
                  currentData.magnitude, shockCount);
  }

  /* ── Free-fall timeout (no impact arrived) ── */
  if (inFreeFall && (now - freeFallTime) > IMPACT_WINDOW_MS) {
    inFreeFall    = false;
    freeFallCount = 0;
  }

  currentData.impactCount = impactCount;
  currentData.shockCount  = shockCount;

  return currentData;
}

/* ================= Utilities ================= */

float getAccelMagnitude(float x, float y, float z) {
  return sqrtf(x * x + y * y + z * z);
}

uint32_t getImpactCount()  { return impactCount; }
uint32_t getShockCount()   { return shockCount;  }

void resetImpactCounters() {
  impactCount = 0;
  shockCount  = 0;
}

bool accelerometerHealthy() {
  /* A flat-stationary MPU6050 reads ~1 g due to gravity */
  return (currentData.magnitude > 0.5f && currentData.magnitude < 2.0f);
}

float getTiltAngle(const AccelData& data) {
  return acosf(fabsf(data.accelZ)) * 180.0f / (float)M_PI;
}
