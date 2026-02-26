# 🔋 **EV Battery Management System (BMS) Firmware**

**A production-grade Battery Management System firmware for Electric Vehicles running on ESP32**

---

## 📋 **Table of Contents**

- [Overview](#-overview)
- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Quick Start](#-quick-start)
- [Project Structure](#-project-structure)
- [Configuration Guide](#-configuration-guide)
- [System Architecture](#-system-architecture)
- [Core Modules](#-core-modules)
- [Installation & Setup](#-installation--setup)
- [Usage Examples](#-usage-examples)
- [Troubleshooting](#-troubleshooting)
- [Cloud Integration](#-cloud-integration)
- [Security Note](#-security-note)
- [Contributing](#-contributing)

---

## 📖 **Overview**

This **Battery Management System (BMS)** firmware is designed for **Electric Vehicles** using an **ESP32 microcontroller**. It provides comprehensive real-time battery monitoring, intelligent protection logic, cloud connectivity, and advanced analytics.

### Key Capabilities:
- ✅ **Real-time Sensing** - Voltage, current, temperature monitoring at 100ms intervals
- ✅ **Battery Intelligence** - SOC, SOH, RUL calculations
- ✅ **Smart Protection** - Over-voltage, over-current, over-temperature handling
- ✅ **Cloud Connectivity** - Supabase cloud integration with WiFi upload
- ✅ **Geolocation** - WiFi-based or hardware GPS tracking
- ✅ **Impact Detection** - MPU6050 accelerometer for collision detection
- ✅ **SMS Alerts** - GSM module for SMS notifications
- ✅ **Telegram Bot** - Real-time alerts via Telegram
- ✅ **LCD Display** - Local I2C display with live telemetry

---

## ✨ **Features**

### 🔌 **Core Monitoring**
- Real-time **pack voltage** measurement (0-25V range)
- **Current sensing** via Adafruit INA219 (bidirectional)
- **Temperature monitoring** with dual threshold control
- **3S LiPo battery** configuration (9V-12.75V nominal)
- **Anomaly detection** using edge computing
- **Cell imbalance** estimation

### ⚠️ **Protection & Safety**
| Feature | Specification |
|---------|---------------|
| **Over-Voltage Protection** | 12.75V (4.25V/cell) |
| **Under-Voltage Protection** | 9.0V (3.0V/cell) |
| **Over-Current (Charge)** | 30A max, 1s duration |
| **Over-Current (Discharge)** | 60A max |
| **Thermal Management** | 60°C max, fan at 40°C |
| **Auto-Recovery** | Fault auto-clears when condition resolved |

### 📡 **Connectivity & Logging**
- **WiFi Cloud** - Supabase integration (10s upload interval)
- **GPS Geolocation** - WiFi-based (BeaconDB) or hardware GPS
- **GSM/SMS** - Alert notifications to phone
- **Telegram Bot** - Real-time anomaly alerts
- **NVS Storage** - Non-volatile logging on ESP32
- **Edge Analytics** - On-device anomaly scoring

### 📺 **User Interface**
- **16x2 LCD I2C Display** - Live voltage, current, SOC, status
- **Serial Telemetry** - 115200 baud UART output (every 2s)
- **Telegram Integration** - Bot commands for monitoring

---

## 🔧 **Hardware Requirements**

### **Required Components**

| Component | Pin | Specification | Purpose |
|-----------|-----|---------------|---------|
| **ESP32** | - | DevKit v1 | Main processor |
| **Voltage Sensor** | GPIO34 (ADC) | 0-25V range | Pack voltage |
| **INA219** | GPIO21/22 (I2C) | Adafruit library | Current sense |
| **DHT22** | GPIO4 | Digital temp | Temperature |
| **LCD16x2** | GPIO21/22 (I2C) | hd44780 library | Display |
| **Charging Relay** | GPIO25 | 5V relay module | Charge control |
| **Motor Relay** | GPIO33 | 5V relay module | Load control |
| **Fan/Heater** | GPIO27 | PWM-controlled | Thermal mgmt |

### **Optional Components**

| Component | Pin | Purpose |
|-----------|-----|---------|
| **GPS Module (Neo-6M)** | GPIO18/19 (UART) | Hardware geolocation |
| **Accelerometer (MPU6050)** | GPIO21/22 (I2C) | Impact detection |
| **GSM Module (SIM800L)** | GPIO16/17 (UART) | SMS alerts |

### **Wiring Diagram**

```
┌─────────────────────────────────────────────────┐
│                   ESP32                         │
│  ┌──────────────────────────────────────────┐   │
│  │ GPIO34  ◄──── Voltage Divider           │   │
│  │ GPIO4   ◄──── DHT22 Data                │   │
│  │ GPIO21/22 ◄─ I2C (SDA/SCL)             │   │
│  │   ├─ INA219 (current)                  │   │
│  │   ├─ LCD16x2 (display)                 │   │
│  │   └─ MPU6050 (accel)                   │   │
│  │ GPIO25  ──── Charge Relay              │   │
│  │ GPIO33  ──── Motor Relay               │   │
│  │ GPIO27  ──── Fan PWM                   │   │
│  │ GPIO16/17 ◄─ GSM Module (UART2)       │   │
│  │ GPIO18/19 ◄─ GPS Module (UART1)       │   │
│  └──────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

---

## 🚀 **Quick Start**

### **1. Install Arduino IDE & ESP32 Board**

```bash
# Download Arduino IDE: https://www.arduino.cc/en/software
# Add ESP32 Board Manager URL:
# https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

### **2. Install Required Libraries**

Open **Arduino IDE → Sketch → Include Library → Manage Libraries**, then install:

```
✓ Adafruit INA219          (for current sensor)
✓ DHT sensor library       (for temperature)
✓ ArduinoJson              (for WiFi geolocation JSON)
✓ TinyGPS++                (optional – for hardware GPS)
✓ hd44780                  (for LCD I2C)
```

### **3. Clone the Repository**

```bash
git clone https://github.com/Hari-r31/BMS_Firmware.git
cd BMS_Firmware
```

### **4. Configure Hardware & WiFi**

Edit **`config.h`**:

```cpp
// WiFi Credentials
static const char* WIFI_SSID = "Your_SSID";
static const char* WIFI_PASS = "Your_Password";

// Hardware Pins (adjust if needed)
#define VOLTAGE_PACK_PIN    34
#define TEMP_PACK_PIN        4
#define CHARGE_RELAY_PIN    25
#define LOAD_MOTOR_RELAY_PIN 33

// Features
#define ENABLE_GEOLOCATION      true
#define ENABLE_IMPACT_DETECTION true
#define ENABLE_LOCAL_DISPLAY    true
#define ENABLE_CLOUD_DASHBOARD  true
```

### **5. Upload to ESP32**

```
1. Connect ESP32 via USB
2. Select Board: Tools → Board → ESP32 Dev Module
3. Select Port: Tools → Port → COM/dev/tty...
4. Click Upload (Ctrl+U)
```

---

## 📁 **Project Structure**

```
BMS_Firmware/
├── BMS_Firmware.ino          # Main entry point & loop (100ms cadence)
├── config.h                  # Hardware pins & parameters ⚙️
├── system.h/cpp              # System init & diagnostics
├── voltage.h/cpp             # Voltage measurement (ADC)
├── current.h/cpp             # Current sensing (INA219)
├── temperature.h/cpp         # Temperature monitoring (DHT22)
├── soc.h/cpp                 # State of Charge (coulomb counting)
├── soh.h/cpp                 # State of Health (cycle & aging)
├── rul.h/cpp                 # Remaining Useful Life estimation
├── fault_manager.h/cpp       # Fault detection & auto-recovery
├── lcd.h/cpp                 # 16x2 LCD I2C display control
├── nvs_logger.h/cpp          # Non-volatile storage (NVS)
├── wifi_cloud.h/cpp          # Supabase cloud upload
├── gps.h/cpp                 # WiFi/hardware GPS geolocation
├── accelerometer.h/cpp       # MPU6050 impact detection
├── gsm_sms.h/cpp             # GSM/SMS module
├── telegram.h/cpp            # Telegram bot integration
├── statistics.h              # Moving averages & math
├── events.h                  # System events & fault codes
└── README.md                 # This file
```

---

## ⚙️ **Configuration Guide**

### **Critical Parameters in `config.h`**

#### **Battery Configuration**
```cpp
#define NUM_CELLS               3              // 3S LiPo
#define NOMINAL_CELL_VOLTAGE    3.75f          // 3.75V nominal
#define CELL_CAPACITY_AH        50.0f          // 50Ah pack
```

#### **Voltage Thresholds**
```cpp
#define CELL_MAX_VOLTAGE        4.25f          // Max per-cell
#define CELL_MIN_VOLTAGE        3.00f          // Min per-cell
#define MAX_VOLTAGE             (12.75f)       // Max pack (4.25 × 3)
#define MIN_VOLTAGE             (9.0f)         // Min pack (3.0 × 3)
#define CHARGE_START_V          (11.55f)       // Charge threshold
#define CHARGE_STOP_V           (12.3f)        // Charge complete
```

#### **Current Thresholds**
```cpp
#define MAX_CHARGE_CURRENT      30.0f          // 30A max charge
#define MAX_DISCHARGE_CURRENT   60.0f          // 60A max discharge
#define OVERCURRENT_DURATION_MS 1000           // 1s trip time
```

#### **Temperature Control**
```cpp
#define MAX_CELL_TEMP           60.0f          // Max operating
#define MIN_CELL_TEMP            0.0f          // Min operating
#define FAN_ON_TEMP             40.0f          // Fan activation
#define FAN_OFF_TEMP            35.0f          // Fan deactivation
```

#### **Cloud Settings**
```cpp
static const char* SUPABASE_URL = "https://omfcwxwtblbhuucqhljf.supabase.co/rest/v1/bms_telemetry";
#define CLOUD_UPLOAD_INTERVAL_MS 10000         // 10s upload
```

#### **Geolocation (Geofence)**
```cpp
#define ENABLE_GEOLOCATION      true
#define GEOFENCE_LAT            12.9716f       // Latitude
#define GEOFENCE_LON            77.5946f       // Longitude
#define GEOFENCE_RADIUS_M       100.0f         // 100m radius
```

---

## 🏗️ **System Architecture**

### **Main Loop Execution (100ms cadence)**

```
┌──────────────────────────────────────────┐
│  MAIN LOOP (every 100ms)                 │
└──────────────────────────────────────────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
    STEP 1      STEP 2      STEP 3
  SENSING     EDGE PROC   BATTERY
                         INTELLIGENCE
    Voltage      Anomaly      SOC
    Current      Detection    SOH
    Temperature  Moving Avg   RUL
                    │
                    ▼
              ┌─────────────┐
              │ STEP 4      │
              │ PROTECTION │
              │ LOGIC       │
              └─────────────┘
                    │
        ┌───────────┴───────────┐
        ▼                       ▼
    FAULT CHECK          AUTO RECOVERY
    - Over-Voltage      - Clear if safe
    - Over-Current      - Log history
    - Over-Temp         
    - Impact            
                    │
                    ▼
              ┌─────────────┐
              │ STEP 5      │
              │ RELAY       │
              │ CONTROL     │
              └─────────────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
    CHARGING    MOTOR      THERMAL
    INTERLOCK   RELAY      MANAGEMENT
                    │
                    ▼
              ┌─────────────┐
              │ STEP 6      │
              │ LCD UPDATE  │
              └─────────────┘
                    │
                    ▼
              ┌─────────────┐
              │ STEP 7      │
              │ TELEMETRY   │
              │ (every 2s)  │
              └─────────────┘
                    │
                    ▼
              ┌─────────────┐
              │ STEP 8      │
              │ CLOUD       │
              │ UPLOAD      │
              │ (10s)       │
              └─────────────┘
```

---

## 🔌 **Core Modules**

### **voltage.cpp** - Pack Voltage Monitoring
- Reads ADC on GPIO34 (34-point moving average)
- Applies calibration scaling
- Detects over/under voltage faults
- Supports 0-25V sensing range

**Key Functions:**
```cpp
void initVoltage()              // Initialize ADC
float readPackVoltage()         // Get voltage reading
void checkVoltageProtection()   // Trigger OV/UV faults
```

### **current.cpp** - Current Sensing (INA219)
- I2C interface to Adafruit INA219
- Bidirectional current measurement
- Over-current fault detection (1s debounce)
- Motor relay control based on live current

**Key Functions:**
```cpp
void initCurrent()                    // Initialize INA219
CurrentData readCurrentData()          // Get I, mW
bool isOverCurrent()                  // Check OC condition
```

### **temperature.cpp** - Thermal Management
- DHT22 digital temperature sensor
- Dual-threshold fan control (40°C on, 35°C off)
- Over-temperature protection (60°C)
- Hysteresis debouncing

**Key Functions:**
```cpp
void initTemperature()          // Initialize DHT22
float readPackTemperature()     // Get temperature (°C)
bool isThermalFault()           // Check high-temp
```

### **soc.cpp** - State of Charge (Coulomb Counting)
- Coulomb counter algorithm
- Voltage-based initial estimate
- Depth-of-discharge (DoD) tracking
- Charge/discharge cycle detection

**Key Functions:**
```cpp
float getSOC()                  // Current SOC (%)
void updateSOC(float current)   // Add coulombs
void calibrateSOC()             // OCV-based cal
```

### **soh.cpp** - State of Health
- Cycle count analysis
- Capacity fade tracking
- Temperature-based degradation
- End-of-life (EOL) threshold: 60%

**Key Functions:**
```cpp
float getSOH()                  // Current SOH (%)
void updateSOH()                // Increment cycles
bool needsReplacement()         // EOL check
```

### **rul.cpp** - Remaining Useful Life
- Multi-factor estimation (voltage 30%, temp 30%, cycles 40%)
- Extrapolation based on current trajectory
- Linear degradation model

**Key Functions:**
```cpp
float estimateRULDays()         // RUL in days
float estimateRULMonths()       // RUL in months
```

### **fault_manager.cpp** - Protection Logic
Detects and manages 10+ fault types:
- `FAULT_OVERVOLTAGE`
- `FAULT_UNDERVOLTAGE`
- `FAULT_OVERCURRENT_DISCHARGE`
- `FAULT_OVERCURRENT_CHARGE`
- `FAULT_OVERHEAT`
- `FAULT_BATTERY_AGING`
- `FAULT_IMPACT_DETECTED`
- `FAULT_GEOFENCE_BREACH`
- etc.

**Key Functions:**
```cpp
void evaluateSystemFaults(...)  // Evaluate all faults
void autoCheckFaultRecovery(..) // Auto-recover if safe
bool isFaulted()                // Is any fault active?
const char* faultReason()       // Human-readable fault
```

### **lcd.cpp** - 16x2 I2C Display
Displays live telemetry on 2 rows:
```
Row 1: 11.10V 2.5A 28.5°C
Row 2: SOC:85% SOH:92% RUL:18m
```

**Key Functions:**
```cpp
void lcdInitialize()            // Init I2C LCD
void lcdUpdate(...)             // Refresh display
void lcdShowFault(fault)        // Display fault message
```

### **wifi_cloud.cpp** - Supabase Integration
- WiFi connection management
- JSON telemetry formatting
- 10s throttled cloud upload
- Error retry logic

**Key Functions:**
```cpp
void wifiEnsure()               // Reconnect if needed
void uploadSystemData(...)      // POST to Supabase
```

### **gps.cpp** - Geolocation
- WiFi-based: Scans nearby SSIDs, queries BeaconDB
- Hardware GPS: UBX protocol on UART1 (optional)
- Geofence breach detection

**Key Functions:**
```cpp
void initGPS()                  // Initialize
void updateLocation()           // Get lat/lon
bool isInGeofence()            // Breach check
```

### **accelerometer.cpp** - Impact Detection (MPU6050)
- I2C accelerometer interface
- Collision threshold: 2.0G
- Event logging on impact

**Key Functions:**
```cpp
void initAccelerometer()        // Initialize MPU6050
bool isImpactDetected()         // Check collision
```

### **telegram.cpp** - Bot Alerts
- Sends alerts via Telegram bot
- Fault notifications
- Periodic health summary

**Key Functions:**
```cpp
void sendTelegramAlert(msg)     // Send message
void notifyFault(fault)         // Fault alert
```

---

## 📝 **Installation & Setup**

### **Step-by-Step Setup**

**1. Install Dependencies (5 min)**

```bash
# Windows/Mac: Use Arduino IDE Library Manager
# Search and install:
#  - Adafruit INA219
#  - DHT sensor library  
#  - ArduinoJson
#  - hd44780 (optional for LCD)
#  - TinyGPS++ (optional for GPS)
```

**2. Clone Repository (2 min)**

```bash
git clone https://github.com/Hari-r31/BMS_Firmware.git
cd BMS_Firmware
```

**3. Configure WiFi & Cloud (3 min)**

Edit `config.h`:
```cpp
// WiFi
static const char* WIFI_SSID = "Your_Network";
static const char* WIFI_PASS = "Your_Password";

// Supabase (get from supabase.com)
static const char* SUPABASE_URL = "your_url";
static const char* SUPABASE_KEY = "your_key";

// Telegram (get from @BotFather on Telegram)
#define TELEGRAM_BOT_TOKEN "your_token"
#define TELEGRAM_CHAT_ID   "your_chat_id"
```

**4. Hardware Assembly (30-60 min)**
- Mount sensors on battery pack
- Connect relays and fan
- Mount LCD on enclosure
- Verify I2C pull-ups (5.6kΩ recommended)

**5. Upload Firmware (5 min)**

```
Arduino IDE:
1. File → Open → BMS_Firmware.ino
2. Tools → Board → ESP32 Dev Module
3. Tools → Port → Select COM port
4. Click Upload
5. Open Serial Monitor (115200 baud)
```

**6. Verify Operation (5 min)**

Watch Serial Monitor for boot messages:
```
[BOOT] ======= EV BMS FIRMWARE INITIALIZED =======
[BOOT] Pack voltage at startup: 11.10 V
[INIT] Voltage calibration: OK
[INIT] Current sensor: READY
[INIT] Temperature: 28.5°C
[LOOP] V=11.10V I=2.5A T=28.5°C SOC=85% SOH=92%
[CLOUD] Data uploaded successfully
```

---

## 💡 **Usage Examples**

### **Monitor via Serial**

```cpp
// Every 2 seconds you'll see:
[LOOP] V=11.10V I=2.5A T=28.5°C SOC=85% SOH=92%
[LOOP] Motor running, Charging disabled
[EDGE] Edge analytics: anomaly score=3
[LOOP] Cycles: 125, RUL: 18 months
```

### **Check LCD Display**

```
┌──────────────────┐
│11.10V 2.5A 28°C  │
│SOC:85% SOH:92%   │
└──────────────────┘
```

### **Telegram Bot Commands**

Send message to bot:
- `status` - Get current battery status
- `soc` - Battery percentage
- `soh` - Battery health
- `location` - GPS coordinates

### **Cloud Dashboard**

View real-time data on Supabase:
```
https://supabase.io/dashboard → Tables → bms_telemetry
```

---

## 🔍 **Troubleshooting**

| Problem | Cause | Solution |
|---------|-------|----------|
| **Voltage reads 0V** | ADC not initialized | Check `VOLTAGE_PACK_PIN` in config.h |
| **INA219 not found** | I2C address mismatch | Run I2C scanner, check SDA/SCL |
| **Temperature -1°C** | DHT22 disconnected | Verify GPIO4 wiring, check data line |
| **LCD blank** | Wrong I2C address | Use I2C scanner to find address (usually 0x27) |
| **WiFi won't connect** | Wrong credentials | Edit WIFI_SSID/WIFI_PASS in config.h |
| **Relay stuck ON** | GPIO issue or timeout | Check relay pin configuration |
| **High current spike** | Motor inrush (normal) | System has 500ms blanking period |
| **Cloud upload failing** | Supabase auth error | Verify SUPABASE_KEY & SUPABASE_URL |
| **Telegram not sending** | Bot token invalid | Get new token from @BotFather |

### **Enable Debug Mode**

Add to code for extra logging:
```cpp
#define DEBUG_LEVEL 2  // 0=off, 1=errors, 2=debug, 3=verbose
```

---

## ☁️ **Cloud Integration (Supabase)**

### **Setup Supabase Account**

1. Go to [supabase.io](https://supabase.io)
2. Create new project
3. Create table: `bms_telemetry`
   ```sql
   CREATE TABLE bms_telemetry (
     id BIGINT PRIMARY KEY GENERATED BY DEFAULT AS IDENTITY,
     device_id TEXT,
     timestamp TIMESTAMP DEFAULT now(),
     voltage FLOAT,
     current FLOAT,
     temperature FLOAT,
     soc FLOAT,
     soh FLOAT,
     latitude FLOAT,
     longitude FLOAT,
     fault_active BOOLEAN
   );
   ```

### **Get API Keys**

1. Go to Project Settings → API
2. Copy `anon` key → paste into `config.h` as `SUPABASE_KEY`
3. Copy project URL → paste as `SUPABASE_URL`

### **Cloud Data Format**

JSON sent every 10 seconds:
```json
{
  "device_id": "BMS_EV_001",
  "voltage": 11.10,
  "current": 2.5,
  "temperature": 28.5,
  "soc": 85.0,
  "soh": 92.0,
  "latitude": 12.9716,
  "longitude": 77.5946,
  "fault_active": false,
  "rul_days": 540
}
```

---

## 🔐 **Security Note**

⚠️ **IMPORTANT**: This repository contains sensitive credentials in `config.h`:

- WiFi passwords
- Supabase API keys
- Telegram bot tokens
- GSM phone numbers

### **Before Pushing to GitHub:**

```bash
# Remove config.h from Git tracking
git rm --cached config.h

# Create config.h.example
cp config.h config.h.example

# Edit config.h.example with dummy values
nano config.h.example

# Add to .gitignore
echo "config.h" >> .gitignore

# Commit
git add .gitignore config.h.example
git commit -m "Add config template, remove secrets"
```

---

## 🤝 **Contributing**

Contributions are welcome! To contribute:

1. **Fork** the repository
2. **Create** a feature branch:
   ```bash
   git checkout -b feature/amazing-feature
   ```
3. **Commit** your changes:
   ```bash
   git commit -m 'Add amazing feature'
   ```
4. **Push** to branch:
   ```bash
   git push origin feature/amazing-feature
   ```
5. **Open** a Pull Request

---

## 📚 **Useful Resources**

- [ESP32 Arduino Documentation](https://docs.espressif.com/projects/arduino-esp32/)
- [Adafruit INA219 Guide](https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout)
- [Battery Management Systems](https://en.wikipedia.org/wiki/Battery_management_system)
- [Supabase Documentation](https://supabase.com/docs)
- [Telegram Bot API](https://core.telegram.org/bots)

---

## 📞 **Support**

- **Author**: [Hari-r31](https://github.com/Hari-r31)
- **Repository**: https://github.com/Hari-r31/BMS_Firmware
- **Issues**: [Report a bug](https://github.com/Hari-r31/BMS_Firmware/issues)

---

## 📄 **License**

This project is open source and available under the **MIT License**. See [LICENSE](LICENSE) file for details.

---

**Last Updated**: February 26, 2026  
**Firmware Version**: 1.0.0  
**Device**: ESP32 (Arduino core)  
**Battery Config**: 3S LiPo, 50Ah pack