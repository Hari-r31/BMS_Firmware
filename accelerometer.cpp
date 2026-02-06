#include "accelerometer.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

/* ================= Private Variables ================= */
static Adafruit_MPU6050 mpu;
static bool initialized = false;
static AccelData currentData;
static uint32_t impactCount = 0;
static uint32_t shockCount = 0;

// Circular buffer for vibration calculation
#define VIBRATION_BUFFER_SIZE 50
static float vibrationBuffer[VIBRATION_BUFFER_SIZE];
static uint8_t vibrationIndex = 0;

/* ================= Private Functions ================= */

/**
 * Add value to vibration buffer
 */
static void addToVibrationBuffer(float value) {
  vibrationBuffer[vibrationIndex] = value;
  vibrationIndex = (vibrationIndex + 1) % VIBRATION_BUFFER_SIZE;
}

/* ================= Public Functions ================= */

void initAccelerometer() {
  if (initialized) return;
  
  #if ENABLE_IMPACT_DETECTION
  Serial.println("[ACCEL] Initializing accelerometer...");
  
  Wire.begin(ACCEL_SDA, ACCEL_SCL);
  
  if (!mpu.begin()) {
    Serial.println("[ACCEL] Failed to find MPU6050 chip!");
    return;
  }
  
  Serial.println("[ACCEL] MPU6050 Found!");
  
  // Set accelerometer range (±8g is good for vehicle monitoring)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println("[ACCEL] Accelerometer range: ±8G");
  
  // Set gyro range
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  
  // Set filter bandwidth
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  // Configure interrupt for motion detection
  pinMode(ACCEL_INT_PIN, INPUT);
  
  // Initialize data structure
  currentData.accelX = 0.0;
  currentData.accelY = 0.0;
  currentData.accelZ = 0.0;
  currentData.magnitude = 0.0;
  currentData.vibrationRMS = 0.0;
  currentData.impactDetected = false;
  currentData.shockDetected = false;
  currentData.impactCount = 0;
  currentData.shockCount = 0;
  
  // Initialize vibration buffer
  for (int i = 0; i < VIBRATION_BUFFER_SIZE; i++) {
    vibrationBuffer[i] = 0.0;
  }
  
  initialized = true;
  Serial.println("[ACCEL] Accelerometer initialized successfully");
  #else
  Serial.println("[ACCEL] Impact detection disabled in config");
  #endif
}

AccelData readAccelerometer() {
  if (!initialized) {
    initAccelerometer();
  }
  
  if (!initialized) {
    // Return default data if initialization failed
    return currentData;
  }
  
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);
  
  // Convert to g (m/s² to g: divide by 9.81)
  currentData.accelX = accel.acceleration.x / 9.81;
  currentData.accelY = accel.acceleration.y / 9.81;
  currentData.accelZ = accel.acceleration.z / 9.81;
  
  // Calculate magnitude
  currentData.magnitude = getAccelMagnitude(
    currentData.accelX,
    currentData.accelY,
    currentData.accelZ
  );
  
  // Add to vibration buffer
  addToVibrationBuffer(currentData.magnitude);
  
  // Calculate vibration level
  currentData.vibrationRMS = calculateVibration();
  
  // Check for impact (subtract gravity component)
  float dynamicAccel = fabs(currentData.magnitude - 1.0);
  
  if (dynamicAccel >= IMPACT_THRESHOLD_G) {
    if (!currentData.impactDetected) {
      currentData.impactDetected = true;
      impactCount++;
      Serial.println("[ACCEL] ⚠️ IMPACT DETECTED!");
      Serial.print("[ACCEL] Magnitude: ");
      Serial.print(currentData.magnitude);
      Serial.println(" g");
    }
  } else {
    currentData.impactDetected = false;
  }
  
  // Check for severe shock
  if (dynamicAccel >= SHOCK_THRESHOLD_G) {
    if (!currentData.shockDetected) {
      currentData.shockDetected = true;
      shockCount++;
      Serial.println("[ACCEL] 🚨 SEVERE SHOCK DETECTED!");
      Serial.print("[ACCEL] Magnitude: ");
      Serial.print(currentData.magnitude);
      Serial.println(" g");
    }
  } else {
    currentData.shockDetected = false;
  }
  
  currentData.impactCount = impactCount;
  currentData.shockCount = shockCount;
  
  return currentData;
}

float getAccelMagnitude(float x, float y, float z) {
  return sqrt(x * x + y * y + z * z);
}

bool checkImpact() {
  AccelData data = readAccelerometer();
  return data.impactDetected;
}

bool checkShock() {
  AccelData data = readAccelerometer();
  return data.shockDetected;
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
  Serial.println("[ACCEL] Impact counters reset");
}

float calculateVibration() {
  // Calculate RMS of vibration buffer
  float sumSquares = 0.0;
  int validSamples = 0;
  
  for (int i = 0; i < VIBRATION_BUFFER_SIZE; i++) {
    if (vibrationBuffer[i] > 0.0) {
      // Remove gravity (1g) before calculating vibration
      float vibComponent = fabs(vibrationBuffer[i] - 1.0);
      sumSquares += vibComponent * vibComponent;
      validSamples++;
    }
  }
  
  if (validSamples == 0) return 0.0;
  
  return sqrt(sumSquares / validSamples);
}

bool accelerometerHealthy() {
  if (!initialized) return false;
  
  AccelData data = readAccelerometer();
  
  // Check if we're getting reasonable acceleration values
  // Total should be around 1g when stationary (gravity)
  if (data.magnitude < 0.5 || data.magnitude > 2.0) {
    Serial.println("[ACCEL] Warning: Unusual acceleration magnitude");
    return false;
  }
  
  return true;
}

float getTiltAngle(const AccelData& data) {
  // Calculate tilt angle from vertical using Z-axis
  // When upright, Z should be ±1g
  float angle = acos(fabs(data.accelZ)) * 180.0 / PI;
  return angle;
}
