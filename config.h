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
  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im9tZmN3eHd0YmxiaHV1Y3FobGpmIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Njk1MTUxMjgsImV4cCI6MjA4NTA5MTEyOH0.--PZM64ogLL_DIUS7N6ZgFH1L5E8WBWZ4X642L3xnno";

/* =========================================================
   GSM MODULE
   ========================================================= */
#define GSM_ALERT_NUMBER "+919491147433"
#define GSM_RX            16
#define GSM_TX            17
#define GSM_BAUD          9600

/* =========================================================
   GPS MODULE
   ========================================================= */
#define GPS_RX   18
#define GPS_TX   19
#define GPS_BAUD 9600

/* =========================================================
   I2C BUS
   ========================================================= */
#define I2C_SDA 21
#define I2C_SCL 22

/* =========================================================
   HARDWARE PINS
   ========================================================= */
#define VOLTAGE_PACK_PIN      34
#define TEMP_PACK_PIN          4

/* Relay pins */
#define CHARGE_RELAY_PIN       25
#define LOAD_MOTOR_RELAY_PIN   33
#define COOLING_FAN_RELAY_PIN  27

/* Unified aliases used throughout system.cpp */
#define MOTOR_RELAY_PIN   LOAD_MOTOR_RELAY_PIN
#define FAN_RELAY_PIN     COOLING_FAN_RELAY_PIN

/* =========================================================
   SYSTEM
   ========================================================= */
#define DEVICE_ID          "BMS_EV_001"
#define FIRMWARE_VERSION   "1.0.0"

/* =========================================================
   BATTERY CONFIGURATION
   ========================================================= */
#define NUM_CELLS           3
#define NUM_PACKS           1
#define NOMINAL_CELL_VOLTAGE  3.75f
#define CELL_CAPACITY_AH    50.0f
#define INITIAL_CAPACITY_AH CELL_CAPACITY_AH

/* =========================================================
   VOLTAGE THRESHOLDS
   ========================================================= */
#define CELL_MAX_VOLTAGE    4.25f
#define CELL_MIN_VOLTAGE    3.00f
#define MAX_CELL_IMBALANCE  0.20f   // estimated only

#define MAX_VOLTAGE  (CELL_MAX_VOLTAGE * NUM_CELLS)
#define MIN_VOLTAGE  (CELL_MIN_VOLTAGE * NUM_CELLS)

#define CHARGE_START_V  (3.85f * NUM_CELLS)
#define CHARGE_STOP_V   (4.1f * NUM_CELLS)

/* =========================================================
   CURRENT THRESHOLDS
   ========================================================= */
#define MAX_CHARGE_CURRENT      30.0f
#define MAX_DISCHARGE_CURRENT   60.0f
#define OVERCURRENT_DURATION_MS 1000

/* =========================================================
   TEMPERATURE THRESHOLDS
   ========================================================= */
#define MAX_CELL_TEMP   60.0f
#define MIN_CELL_TEMP    0.0f
#define MAX_PACK_TEMP   65.0f

#define FAN_ON_TEMP   40.0f
#define FAN_OFF_TEMP  35.0f

/* =========================================================
   ADC
   ========================================================= */
#define ADC_RESOLUTION  4095.0f
#define ADC_VREF         3.3f
#define ADC_SAMPLES      300

/* =========================================================
   VOLTAGE SENSOR MODULE
   ========================================================= */
#define VOLTAGE_SENSOR_MAX_VOLTAGE  25.0f

/* =========================================================
   CURRENT SENSOR (INA219)
   ========================================================= */
/* INA219 is used directly via Adafruit library - no manual mv/A needed */

/* =========================================================
   ACCELEROMETER / IMPACT
   ========================================================= */
#define ENABLE_IMPACT_DETECTION  true
#define ACCEL_SDA  I2C_SDA
#define ACCEL_SCL  I2C_SCL

/* Thresholds are defined internally in accelerometer.cpp */

/* =========================================================
   GPS / GEOFENCE
   ========================================================= */
#define ENABLE_GEOLOCATION  true
#define GEOFENCE_ENABLED    true

#define GEOFENCE_LAT       12.9716f
#define GEOFENCE_LON       77.5946f
#define GEOFENCE_RADIUS_M  100.0f

/* =========================================================
   DISPLAY
   ========================================================= */
#define ENABLE_LOCAL_DISPLAY    true
#define ENABLE_CLOUD_DASHBOARD  true

/* =========================================================
   SOH (STATE OF HEALTH)
   ========================================================= */
#define SOH_MIN_THRESHOLD      60.0f
#define SOH_DEGRADE_PER_FAULT   0.2f
#define SOH_DEGRADE_PER_CYCLE   0.05f
#define SOH_DEGRADE_HIGH_TEMP   0.1f

/* =========================================================
   RUL (REMAINING USEFUL LIFE)
   ========================================================= */
#define RUL_CYCLES_NEW      1000
#define RUL_VOLTAGE_WEIGHT  0.3f
#define RUL_TEMP_WEIGHT     0.3f
#define RUL_CYCLE_WEIGHT    0.4f

/* =========================================================
   EDGE COMPUTING
   ========================================================= */
#define EDGE_PROCESSING_ENABLED  true

/* =========================================================
   CLOUD
   ========================================================= */
#define CLOUD_UPLOAD_INTERVAL_MS  10000

/* =========================================================
   TIMING / SYSTEM
   ========================================================= */
#define DEBOUNCE_TIME_MS         50
#define RELAY_SWITCH_DELAY_MS   100
#define SENSOR_READ_INTERVAL_MS 200
#define WATCHDOG_TIMEOUT_MS   30000

/* =========================================================
   TELEGRAM
   ========================================================= */
#define TELEGRAM_BOT_TOKEN  "8590024470:AAHevONCpIOeMUtcexgg1F_9Ov0kj7SVaIA"
#define TELEGRAM_CHAT_ID    "1680413436"
