#pragma once

/* =========================================================
   WIFI
   ========================================================= */
static const char* WIFI_SSID = "scrap";
static const char* WIFI_PASS = "we4rscrap!";

/* =========================================================
   SUPABASE CLOUD
   ========================================================= */
static const char* SUPABASE_URL =
  "https://omfcwxwtblbhuucqhljf.supabase.co/rest/v1/bms_telemetry";

static const char* SUPABASE_KEY =
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im9tZmN3eHd0YmxiaHV1Y3FobGpmIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Njk1MTUxMjgsImV4cCI6MjA4NTA5MTEyOH0.--PZM64ogLL_DIUS7N6ZgFH1L5E8WBWZ4X642L3xnno"
;

/* =========================================================
   GSM MODULE
   ========================================================= */
#define GSM_ALERT_NUMBER "+919491147433"
#define GSM_RX 16
#define GSM_TX 17
#define GSM_BAUD 9600

/* =========================================================
   I2C BUS
   ========================================================= */
#define I2C_SDA 21
#define I2C_SCL 22

/* =========================================================
   HARDWARE PINS
   ========================================================= */
// Sensors
#define VOLTAGE_PACK_PIN 34
#define CURRENT_PIN 35
#define TEMP_PACK_PIN 4

// Accelerometer
#define ACCEL_INT_PIN 23

// Relays
#define CHARGE_RELAY_PIN 25
#define MOTOR_RELAY_PIN 26
#define FAN_RELAY_PIN 27

/* =========================================================
   SYSTEM
   ========================================================= */
#define DEVICE_ID "BMS_EV_001"
#define FIRMWARE_VERSION "2.0.0"

/* =========================================================
   BATTERY CONFIGURATION
   ========================================================= */
#define NUM_CELLS 3
#define NUM_PACKS 1
#define NOMINAL_CELL_VOLTAGE 3.7
#define CELL_CAPACITY_AH 50.0
#define INITIAL_CAPACITY_AH CELL_CAPACITY_AH

/* =========================================================
   VOLTAGE THRESHOLDS
   ========================================================= */
#define CELL_MAX_VOLTAGE 4.25
#define CELL_MIN_VOLTAGE 3.00
#define MAX_CELL_IMBALANCE 0.20   // estimated only

#define MAX_VOLTAGE (CELL_MAX_VOLTAGE * NUM_CELLS)
#define MIN_VOLTAGE (CELL_MIN_VOLTAGE * NUM_CELLS)

#define CHARGE_START_V (3.3 * NUM_CELLS)
#define CHARGE_STOP_V  (4.1 * NUM_CELLS)

/* =========================================================
   CURRENT THRESHOLDS
   ========================================================= */
#define MAX_CHARGE_CURRENT     30.0
#define MAX_DISCHARGE_CURRENT  60.0
#define OVERCURRENT_DURATION_MS 1000

/* =========================================================
   TEMPERATURE THRESHOLDS
   ========================================================= */
#define MAX_CELL_TEMP 60.0
#define MIN_CELL_TEMP 0.0
#define MAX_PACK_TEMP 65.0

#define FAN_ON_TEMP  40.0
#define FAN_OFF_TEMP 35.0

/* =========================================================
   ADC
   ========================================================= */
#define ADC_RESOLUTION 4095.0
#define ADC_VREF 3.3
#define ADC_SAMPLES 300

/* =========================================================
   VOLTAGE SENSOR MODULE
   ========================================================= */
#define VOLTAGE_SENSOR_MAX_VOLTAGE 25.0

/* =========================================================
   CURRENT SENSOR (ACS712)
   ========================================================= */
#define CURRENT_SENSOR_MV_PER_A 66
#define CURRENT_SENSOR_VREF 2.5

/* =========================================================
   ACCELEROMETER / IMPACT
   ========================================================= */
#define ENABLE_IMPACT_DETECTION true
#define IMPACT_THRESHOLD_G 0.4
#define SHOCK_THRESHOLD_G 1

/* =========================================================
   GPS / GEOFENCE
   ========================================================= */
#define ENABLE_GEOLOCATION true
#define GEOFENCE_ENABLED true

#define ENABLE_LOCAL_DISPLAY true
#define ENABLE_CLOUD_DASHBOARD true


#define GEOFENCE_LAT 12.9716
#define GEOFENCE_LON 77.5946
#define GEOFENCE_RADIUS_M 100.0

/* =========================================================
   SOH (STATE OF HEALTH)
   ========================================================= */
#define SOH_MIN_THRESHOLD 60.0
#define SOH_DEGRADE_PER_FAULT 0.2
#define SOH_DEGRADE_PER_CYCLE 0.05
#define SOH_DEGRADE_HIGH_TEMP 0.1

/* =========================================================
   RUL (REMAINING USEFUL LIFE)
   ========================================================= */
#define RUL_CYCLES_NEW 1000
#define RUL_VOLTAGE_WEIGHT 0.3
#define RUL_TEMP_WEIGHT 0.3
#define RUL_CYCLE_WEIGHT 0.4

/* =========================================================
   EDGE COMPUTING
   ========================================================= */
#define EDGE_PROCESSING_ENABLED true

/* =========================================================
   CLOUD
   ========================================================= */
#define CLOUD_UPLOAD_INTERVAL_MS 10000

/* =========================================================
   TIMING / SYSTEM
   ========================================================= */
#define DEBOUNCE_TIME_MS 50
#define RELAY_SWITCH_DELAY_MS 100
#define SENSOR_READ_INTERVAL_MS 200
#define WATCHDOG_TIMEOUT_MS 30000

/* =========================================================
   TELEGRAM
   ========================================================= */
#define TELEGRAM_BOT_TOKEN "8590024470:AAHevONCpIOeMUtcexgg1F_9Ov0kj7SVaIA"
#define TELEGRAM_CHAT_ID  "1680413436"

#define ACCEL_SDA I2C_SDA
#define ACCEL_SCL I2C_SCL

