# Smart Battery Management System (BMS) for Electric Vehicles

## 🚗 Advanced Multi-Cell Monitoring & Analytics Platform

This comprehensive Battery Management System (BMS) provides enterprise-grade monitoring, protection, and analytics for electric vehicle battery packs with support for multi-cell monitoring, edge computing, cloud connectivity, and intelligent fault detection.

---

## 📋 Table of Contents
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [System Architecture](#system-architecture)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## ✨ Features

### Core Battery Monitoring
- ✅ **Multi-Cell Voltage Monitoring**: Individual cell monitoring (4S configuration) + pack voltage
- ✅ **Bidirectional Current Sensing**: Accurate charging and discharging current measurement (ACS712)
- ✅ **Multi-Point Temperature Monitoring**: 4 temperature sensors (2 cell, 1 ambient, 1 pack)
- ✅ **Cell Balancing Detection**: Real-time imbalance monitoring with configurable thresholds
- ✅ **Pack Configuration Support**: Single pack or multiple parallel packs

### Advanced Protection
- 🛡️ **Comprehensive Fault Detection**:
  - Over-voltage / Under-voltage (cell & pack level)
  - Over-current (charge & discharge)
  - Over-temperature / Under-temperature
  - Cell imbalance
  - Thermal runaway risk assessment
  - Sensor failure detection
  
- 🔄 **Active Thermal Management**:
  - Smart cooling fan control with hysteresis
  - Optional heating for cold conditions
  - Multi-zone temperature monitoring
  
- 🔌 **Intelligent Charging Control**:
  - Automatic charge/discharge management
  - Voltage-based charging cutoff
  - Fault-aware charge termination
  - Cycle counting and tracking

### Edge Computing & Analytics
- 🧠 **Local Anomaly Detection**: Real-time edge processing for fault prediction
- 📊 **Moving Average Filtering**: Noise reduction and trend analysis
- 📈 **Predictive Analytics**: Voltage, current, and temperature trend monitoring
- 🎯 **Anomaly Scoring**: 0-100 scale health score calculation

### State of Health (SOH) & Remaining Useful Life (RUL)
- 💚 **Advanced SOH Estimation**:
  - Cycle-based degradation tracking
  - Temperature-based aging models
  - Fault-induced degradation
  - Persistent storage in NVS

- ⏰ **RUL Prediction**:
  - Multi-factor estimation (voltage, temperature, cycles)
  - Weighted combination algorithm
  - Replacement date prediction
  - Remaining cycles & hours estimation

### Cloud Connectivity & Remote Monitoring
- ☁️ **Supabase Integration**: Real-time cloud database upload
- 📱 **WhatsApp Alerts**: Event-based notifications via API
  - Fault alerts with severity levels
  - Geo-fence violations
  - Impact detection
  - Maintenance reminders

- 📲 **SMS Notifications**: GSM module for critical alerts
- 🌐 **WiFi Auto-Reconnect**: Resilient cloud connectivity
- 📊 **Time-Series Data Storage**: Historical data logging

### Location & Security
- 📍 **GPS Tracking** (Optional):
  - Real-time location monitoring
  - Geo-fence support for theft detection
  - Distance from home calculation
  - Coordinate logging to cloud

- 🚨 **Geo-Fence Alerts**:
  - Configurable radius
  - Automatic theft detection
  - WhatsApp/SMS notifications

### Impact & Vibration Detection
- 💥 **Accelerometer Integration** (MPU6050):
  - Impact detection (configurable G-force threshold)
  - Severe shock detection
  - Vibration monitoring (RMS calculation)
  - Tilt angle measurement
  - Event counting

### Data Logging & Maintenance
- 💾 **Non-Volatile Storage (NVS)**:
  - SOH persistence
  - Cycle count tracking
  - Fault history logging
  - Configuration storage

- 🔧 **Maintenance Tracking**:
  - Usage hours tracking
  - Warranty period monitoring
  - Service interval reminders
  - Installation timestamp

### User Interface
- 📺 **LCD Display** (16x2 I2C):
  - Real-time voltage, current, temperature
  - Fault status indication
  - Compact dashboard view

- 💡 **Status LEDs** (Optional):
  - Normal operation (Green)
  - Warning state (Yellow)
  - Fault condition (Red)

- 🖥️ **Serial Console**:
  - Detailed telemetry logging
  - System diagnostics
  - Debug information

---

## 🛠️ Hardware Requirements

### Core Components
| Component | Model | Quantity | Notes |
|-----------|-------|----------|-------|
| Microcontroller | ESP32 DevKit | 1 | WiFi + Bluetooth capable |
| Voltage Sensor | Resistor Divider | 5 | Pack + 4 cells |
| Current Sensor | ACS712-30A | 1 | Bidirectional, Hall effect |
| Temperature Sensor | DHT11 | 4 | Cell1, Cell2, Ambient, Pack |
| GSM Module | SIM800L | 1 | For SMS alerts |
| LCD Display | 16x2 I2C | 1 | Optional but recommended |

### Optional Components
| Component | Model | Quantity | Purpose |
|-----------|-------|----------|---------|
| GPS Module | NEO-6M | 1 | Location tracking |
| Accelerometer | MPU6050 | 1 | Impact detection |
| Relay Module | 5V 4-Channel | 1 | Charge, discharge, fan, heater |

### Pin Connections

```
ESP32 Pinout:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Voltage Sensors:
  - Pack Voltage:   GPIO 34 (ADC)
  - Cell 1:         GPIO 35 (ADC)
  - Cell 2:         GPIO 32 (ADC)
  - Cell 3:         GPIO 33 (ADC)
  - Cell 4:         GPIO 25 (ADC)

Current Sensor:
  - ACS712 Output:  GPIO 39 (ADC)

Temperature Sensors (DHT11):
  - Cell 1:         GPIO 4
  - Cell 2:         GPIO 5
  - Ambient:        GPIO 18
  - Pack:           GPIO 19

GPS Module:
  - TX:             GPIO 26
  - RX:             GPIO 27

Accelerometer (I2C):
  - SDA:            GPIO 21
  - SCL:            GPIO 22
  - INT:            GPIO 23

GSM Module:
  - TX:             GPIO 16
  - RX:             GPIO 17

LCD Display (I2C):
  - SDA:            GPIO 21 (shared)
  - SCL:            GPIO 22 (shared)

Relay Controls:
  - Charging:       GPIO 13
  - Discharge:      GPIO 12
  - Cooling Fan:    GPIO 14
  - Heater:         GPIO 15

Status LEDs:
  - Normal (Green): GPIO 2
  - Warning (Yel):  GPIO 17
  - Fault (Red):    GPIO 16
```

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32 BMS Controller                 │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │   Voltage    │  │   Current    │  │ Temperature  │ │
│  │  Monitoring  │  │   Sensing    │  │  Monitoring  │ │
│  │  (5 points)  │  │ (ACS712-30A) │  │  (4 DHT11)   │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
│         │                 │                  │          │
│         └─────────────────┴──────────────────┘          │
│                           │                             │
│                    ┌──────▼──────┐                      │
│                    │    Fault    │                      │
│                    │  Detection  │◄─────────┐           │
│                    │   & Edge    │          │           │
│                    │  Computing  │          │           │
│                    └──────┬──────┘          │           │
│                           │                 │           │
│         ┌─────────────────┼─────────────────┤           │
│         │                 │                 │           │
│    ┌────▼────┐      ┌─────▼─────┐    ┌─────▼─────┐    │
│    │ Thermal │      │ Charging  │    │   SOH &   │    │
│    │ Control │      │  Control  │    │    RUL    │    │
│    └─────────┘      └───────────┘    └───────────┘    │
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │     GPS      │  │ Accelero-    │  │  GSM/WiFi    │ │
│  │   Tracking   │  │    meter     │  │    Comms     │ │
│  │  (Optional)  │  │  (Optional)  │  │              │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
│         │                 │                  │          │
│         └─────────────────┴──────────────────┘          │
│                           │                             │
│                    ┌──────▼──────┐                      │
│                    │  Alert &    │                      │
│                    │   Cloud     │                      │
│                    │  Reporting  │                      │
│                    └─────────────┘                      │
│                                                         │
└─────────────────────────────────────────────────────────┘
            │                                   │
            ▼                                   ▼
    ┌───────────────┐                  ┌──────────────┐
    │   Supabase    │                  │   WhatsApp   │
    │  Time-Series  │                  │     API      │
    │   Database    │                  │   Alerts     │
    └───────────────┘                  └──────────────┘
```

---

## 📦 Installation

### 1. Arduino IDE Setup
```bash
1. Install Arduino IDE (1.8.19 or later)
2. Add ESP32 board support:
   - File > Preferences > Additional Board URLs
   - Add: https://dl.espressif.com/dl/package_esp32_index.json
3. Install ESP32 board from Board Manager
```

### 2. Required Libraries
```cpp
// Install via Arduino Library Manager:
- WiFi (built-in)
- HTTPClient (built-in)
- Preferences (built-in)
- DHT sensor library by Adafruit
- hd44780 by Bill Perry (for LCD)
- TinyGPS++ by Mikal Hart (for GPS)
- Adafruit MPU6050 (for accelerometer)
- Adafruit Unified Sensor
```

### 3. Upload Firmware
```bash
1. Open BMS_Firmware.ino in Arduino IDE
2. Select Board: "ESP32 Dev Module"
3. Select correct COM port
4. Upload Speed: 115200
5. Click Upload
```

---

## ⚙️ Configuration

### Edit `config.h` for your setup:

```cpp
// WiFi Credentials
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASS "YourPassword"

// Battery Configuration
#define NUM_CELLS 4                // Number of cells in series
#define NUM_PACKS 1                // Number of parallel packs
#define CELL_CAPACITY_AH 50.0      // Capacity per cell

// Thresholds (adjust for your battery chemistry)
#define CELL_MAX_VOLTAGE 4.2       // Li-ion: 4.2V, LiFePO4: 3.65V
#define CELL_MIN_VOLTAGE 3.0       // Li-ion: 3.0V, LiFePO4: 2.5V
#define MAX_CELL_TEMP 55.0         // Maximum safe temperature
#define MAX_CHARGE_CURRENT 30.0    // Maximum charging current

// Feature Flags
#define ENABLE_WHATSAPP_ALERTS true
#define ENABLE_GEOLOCATION true
#define ENABLE_IMPACT_DETECTION true
#define ENABLE_EDGE_PROCESSING true

// WhatsApp API (get free API key from callmebot.com)
#define WHATSAPP_PHONE "+1234567890"
#define WHATSAPP_API_KEY "your_api_key"

// Geo-fence (set your home location)
#define GEOFENCE_LAT 17.385044
#define GEOFENCE_LON 78.486671
#define GEOFENCE_RADIUS_M 1000     // Alert if beyond 1km
```

---

## 🚀 Usage

### Initial Setup
1. **Power up the system** with battery disconnected
2. **Open Serial Monitor** (115200 baud)
3. **Wait for "BMS SYSTEM READY"** message
4. **Connect battery** to voltage dividers
5. **Calibrate current sensor** (ensure no load)

### Normal Operation
The system will automatically:
- Monitor all battery parameters every 200ms
- Upload to cloud every 10 seconds
- Send alerts on fault conditions
- Control charging based on voltage
- Activate cooling when needed

### Serial Console Commands
View real-time telemetry in Serial Monitor:
```
╔═══════════════ TELEMETRY ═══════════════╗
║ Pack: 14.82V  Cells: [3.71, 3.70, 3.71, 3.70]
║ Imbalance: 0.010V
║ Current: -2.34A (CHARGING)
║ Power: 34.7W
║ Temps: Cell1=32.1°C Cell2=31.8°C Ambient=28.5°C
║ SOH: 98.5%  RUL: 950 cycles (950 days)
║ Status: NORMAL  Charging: ON
╚═════════════════════════════════════════╝
```

### Cloud Dashboard
Access your Supabase dashboard to view:
- Real-time telemetry graphs
- Historical data trends
- Fault event logs
- SOH/RUL predictions
- GPS location tracking

---

## 📚 API Reference

### Key Functions

```cpp
// Voltage Monitoring
CellVoltageData readAllVoltages();  // Read all cells + pack
float readCellVoltage(uint8_t cellNumber);  // Read specific cell
float calculateImbalance(const CellVoltageData& data);

// Current Monitoring
CurrentData readCurrentData();  // Full current data with direction
bool isCharging();  // Check if battery charging
bool isDischarging();  // Check if battery discharging

// Temperature
TemperatureData readAllTemperatures();  // All temp sensors
bool needsCooling(const TemperatureData& data);
uint8_t getThermalRunawayRisk(const TemperatureData& data);

// Fault Management
void evaluateSystemFaults(...);  // Comprehensive fault check
bool isFaulted();  // Check fault status
FaultData getFaultData();  // Get detailed fault info
void clearFaults();  // Manual fault reset

// SOH & RUL
float getSOH();  // Current state of health
int estimateRUL();  // Remaining useful life (cycles)
bool needsReplacement();  // Check if replacement needed

// GPS (if enabled)
GPSData getGPSData();  // Current location
bool isGeofenceViolated();  // Check geo-fence

// Accelerometer (if enabled)
AccelData readAccelerometer();  // Read acceleration
bool checkImpact();  // Check for impact
uint32_t getImpactCount();  // Total impacts
```

---

## 🔧 Troubleshooting

### Common Issues

**Problem: Current reading always shows 0A**
```
Solution:
1. Check ACS712 connections (VCC, GND, OUT)
2. Verify CURRENT_PIN configuration
3. Re-run calibrateCurrent() with no load
4. Check if sensor is reversed (swap wires)
```

**Problem: WiFi won't connect**
```
Solution:
1. Verify SSID and password in config.h
2. Check WiFi signal strength
3. Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
4. Check Serial Monitor for connection errors
```

**Problem: Temperature sensors return -999°C**
```
Solution:
1. Check DHT11 connections (VCC, GND, DATA)
2. Ensure 2-second delay between reads
3. Verify correct pins in config.h
4. Test each sensor individually
```

**Problem: Voltage readings seem incorrect**
```
Solution:
1. Verify voltage divider resistor values
2. Adjust DIVIDER_RATIO in voltage.cpp
3. Calibrate using known voltage source
4. Check ADC reference voltage (should be 3.3V)
```

**Problem: Fault continuously triggers**
```
Solution:
1. Check threshold values in config.h
2. Review Serial Monitor for fault reason
3. Verify sensor readings are accurate
4. Use clearFaults() to reset if needed
```

---

## 📊 Performance Specifications

| Parameter | Value |
|-----------|-------|
| Voltage Accuracy | ±50mV |
| Current Accuracy | ±0.5A |
| Temperature Accuracy | ±2°C |
| Sampling Rate | 10 Hz (10 samples/sec) |
| Cloud Upload Rate | 0.1 Hz (every 10 sec) |
| Edge Processing | Real-time |
| Power Consumption | ~500mA @ 5V |
| Operating Temp Range | -10°C to 60°C |
| Max Battery Voltage | 20V (configurable) |
| Max Current | ±30A (ACS712-30A) |

---

## 🎯 Roadmap

- [ ] Machine learning-based SOH prediction
- [ ] Mobile app for real-time monitoring
- [ ] CAN bus integration for automotive systems
- [ ] Advanced cell balancing control
- [ ] Energy efficiency analytics
- [ ] Predictive maintenance scheduling
- [ ] Multi-battery pack support
- [ ] Web-based configuration interface

---

## 📄 License

This project is released under the MIT License.

---

## 👥 Contributing

Contributions welcome! Please submit pull requests or open issues for bugs/features.

---

## 📞 Support

For questions or support:
- Open an issue on GitHub
- Email: support@yourproject.com
- Documentation: https://docs.yourproject.com

---

## 🙏 Acknowledgments

- Anthropic Claude for development assistance
- ESP32 community for hardware support
- Open-source library contributors

---

**Built with ❤️ for safer, smarter EV battery management**
