# 🚀 Quick Start Guide - BMS Setup in 10 Minutes

This guide will get your BMS up and running quickly!

---

## ⚡ Prerequisites Checklist

Before starting, make sure you have:

- [ ] ESP32 DevKit board
- [ ] ACS712-30A current sensor
- [ ] 4x DHT11 temperature sensors
- [ ] Resistors for voltage dividers (47kΩ and 10kΩ)
- [ ] Relay module (4-channel)
- [ ] 16x2 I2C LCD display
- [ ] SIM800L GSM module
- [ ] USB cable for programming
- [ ] Arduino IDE installed

**Optional:**
- [ ] NEO-6M GPS module
- [ ] MPU6050 accelerometer

---

## 📝 Step 1: Install Arduino IDE & Libraries (5 min)

### 1.1 Install ESP32 Board Support
```
1. Open Arduino IDE
2. File → Preferences
3. Add to "Additional Board URLs":
   https://dl.espressif.com/dl/package_esp32_index.json
4. Tools → Board → Board Manager
5. Search "ESP32" and install
```

### 1.2 Install Required Libraries
```
Go to: Tools → Manage Libraries

Install these:
✅ DHT sensor library (by Adafruit)
✅ hd44780 (by Bill Perry)
✅ TinyGPS++ (by Mikal Hart) - if using GPS
✅ Adafruit MPU6050 - if using accelerometer
✅ Adafruit Unified Sensor
```

---

## 🔌 Step 2: Hardware Connections (10 min)

### Minimal Setup (Core BMS Only)

```
ESP32          →  Component
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
GPIO 34        →  Pack Voltage (voltage divider)
GPIO 35        →  Cell 1 Voltage (voltage divider)
GPIO 39        →  ACS712 Output
GPIO 4         →  DHT11 Data (Cell temp)
GPIO 13        →  Charge Relay IN
GPIO 14        →  Fan Relay IN
GPIO 21        →  LCD SDA
GPIO 22        →  LCD SCL
5V             →  All VCC connections
GND            →  All GND connections
```

### Voltage Divider Circuit (Per Cell)
```
Battery (+) ────┬──── 47kΩ ────┬──── ESP32 ADC Pin
                │               │
                │              10kΩ
                │               │
Battery (-) ────┴───────────────┴──── ESP32 GND

Formula: Vout = Vin × (10kΩ / (47kΩ + 10kΩ))
```

### Current Sensor (ACS712)
```
ACS712 VCC  →  5V
ACS712 GND  →  GND
ACS712 OUT  →  GPIO 39
ACS712 IP+  →  Battery (+) or Load
ACS712 IP-  →  Battery (-) or Load
```

---

## ⚙️ Step 3: Configure Software (3 min)

### 3.1 Edit config.h

Open `config.h` and update:

```cpp
// YOUR WIFI CREDENTIALS
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASS "YourPassword"

// YOUR PHONE NUMBER FOR ALERTS
#define GSM_ALERT_NUMBER "+1234567890"

// BATTERY CONFIGURATION
#define NUM_CELLS 4              // Change if different
#define CELL_CAPACITY_AH 50.0    // Your battery capacity

// VOLTAGE LIMITS (IMPORTANT!)
#define CELL_MAX_VOLTAGE 4.2     // Li-ion: 4.2V, LiFePO4: 3.65V
#define CELL_MIN_VOLTAGE 3.0     // Li-ion: 3.0V, LiFePO4: 2.5V

// ENABLE/DISABLE FEATURES
#define ENABLE_WHATSAPP_ALERTS false  // Set true when configured
#define ENABLE_GEOLOCATION false      // Set true if GPS connected
#define ENABLE_IMPACT_DETECTION false // Set true if accelerometer connected
```

### 3.2 Supabase Setup (Optional but Recommended)

1. Go to [supabase.com](https://supabase.com)
2. Create free account
3. Create new project
4. Create table "telemetry" with columns:
   ```sql
   CREATE TABLE telemetry (
     id BIGSERIAL PRIMARY KEY,
     created_at TIMESTAMPTZ DEFAULT NOW(),
     device_id TEXT,
     pack_voltage REAL,
     cell1_voltage REAL,
     cell2_voltage REAL,
     cell3_voltage REAL,
     cell4_voltage REAL,
     current REAL,
     power REAL,
     temp_cell1 REAL,
     temp_cell2 REAL,
     temp_ambient REAL,
     temp_pack REAL,
     soh REAL,
     rul INTEGER,
     fault BOOLEAN,
     fault_message TEXT
   );
   ```
5. Copy your API URL and Key to `config.h`

---

## 📤 Step 4: Upload Firmware (2 min)

```
1. Open BMS_Firmware.ino in Arduino IDE
2. Tools → Board → ESP32 Dev Module
3. Tools → Port → Select your COM port
4. Tools → Upload Speed → 115200
5. Click Upload button (→)
6. Wait for "Done uploading"
```

---

## 🧪 Step 5: Initial Testing (5 min)

### 5.1 Open Serial Monitor
```
Tools → Serial Monitor
Set baud rate: 115200
```

### 5.2 Watch Boot Sequence
You should see:
```
╔════════════════════════════════════════════════════════╗
║  SMART BATTERY MANAGEMENT SYSTEM (BMS) FOR EV         ║
╚════════════════════════════════════════════════════════╝

[LCD] LCD initialized
[WIFI] Initializing WiFi...
[WIFI] ✓ Connected successfully
[VOLTAGE] Multi-cell voltage monitoring initialized
[CURRENT] Calibrating current sensor...
[CURRENT] ✓ Calibration successful
[TEMP] Initializing temperature monitoring...
[FAULT] Fault management initialized

--- SYSTEM DIAGNOSTICS ---
✓ Voltage sensors: OK
✓ Current sensor: OK
✓ Temperature sensors: OK
✓ WiFi: Connected
--- DIAGNOSTICS COMPLETE ---

=================================
✓ BMS SYSTEM READY
=================================
```

### 5.3 Check Telemetry Output
Every 2 seconds you should see:
```
╔═══════════════ TELEMETRY ═══════════════╗
║ Pack: 12.60V  Cells: [3.15, 3.15, 3.15, 3.15]
║ Imbalance: 0.000V
║ Current: 0.00A (IDLE)
║ Power: 0.0W
║ Temps: Cell1=25.0°C Cell2=25.0°C Ambient=24.5°C
║ SOH: 100.0%  RUL: 1000 cycles (1000 days)
║ Status: NORMAL  Charging: OFF
╚═════════════════════════════════════════╝
```

---

## ✅ Step 6: Calibration & Testing

### 6.1 Current Sensor Calibration
**IMPORTANT: Do this with NO LOAD!**
```
1. Disconnect battery load
2. Reset ESP32
3. Wait for calibration message
4. Verify current reads ~0.0A
```

### 6.2 Voltage Verification
```
1. Measure actual battery voltage with multimeter
2. Compare with BMS reading
3. If different, adjust calibration in voltage.cpp:
   calibrationFactors[0] = actualVoltage / measuredVoltage;
```

### 6.3 Temperature Check
```
1. Check all DHT11 sensors show reasonable temps
2. Should be near room temperature (20-30°C)
3. If showing -999°C, check wiring
```

### 6.4 Relay Testing
```
1. Manually connect battery above CHARGE_START_V
2. Watch for "CHARGER ON" message
3. Verify relay clicks
4. Check voltage on relay output
```

---

## 🎯 Common First-Time Issues

### Issue: WiFi won't connect
```
✅ Solution:
   - Check SSID/password spelling
   - Ensure 2.4GHz network (not 5GHz)
   - Move closer to router
   - Check router allows new devices
```

### Issue: Current always 0A
```
✅ Solution:
   - Check ACS712 VCC = 5V
   - Verify OUT pin connected to GPIO 39
   - Run calibration with NO load
   - Check current flowing through IP+/IP- terminals
```

### Issue: Voltage readings wrong
```
✅ Solution:
   - Verify resistor values (47kΩ and 10kΩ)
   - Check voltage divider connections
   - Measure divider output with multimeter
   - Adjust calibration factor if needed
```

### Issue: Temperature shows -999
```
✅ Solution:
   - Check DHT11 VCC = 5V (or 3.3V)
   - Verify data pin connection
   - Add 10kΩ pull-up resistor to data line
   - Try different DHT11 (could be faulty)
```

### Issue: Continuous fault triggering
```
✅ Solution:
   - Check threshold values in config.h
   - Ensure sensor readings are accurate
   - Use clearFaults() function in code
   - Review Serial Monitor for fault reason
```

---

## 🔧 Advanced Configuration (Optional)

### Enable WhatsApp Alerts
```
1. Visit callmebot.com
2. Add their bot to WhatsApp: +34 644 51 55 24
3. Send message: "I allow callmebot to send me messages"
4. You'll receive your API key
5. Update config.h with your number and API key
6. Set ENABLE_WHATSAPP_ALERTS = true
```

### Enable GPS Tracking
```
1. Connect NEO-6M GPS to GPIO 26/27
2. Set ENABLE_GEOLOCATION = true
3. Set your home coordinates in config.h:
   GEOFENCE_LAT = your_latitude
   GEOFENCE_LON = your_longitude
4. Set geo-fence radius (meters)
```

### Enable Impact Detection
```
1. Connect MPU6050 to I2C (GPIO 21/22)
2. Connect INT pin to GPIO 23
3. Set ENABLE_IMPACT_DETECTION = true
4. Adjust thresholds:
   IMPACT_THRESHOLD_G = 3.0 (normal impact)
   SHOCK_THRESHOLD_G = 5.0 (severe shock)
```

---

## 📊 Monitoring Your BMS

### Via Serial Monitor
- Real-time telemetry every 2 seconds
- Detailed fault messages
- System status updates

### Via LCD Display
- Line 1: Voltage, Current
- Line 2: Temperature, Status
- Fault indication when active

### Via Cloud Dashboard
- Log into Supabase
- View "telemetry" table
- Create graphs in dashboard
- Set up email alerts

### Via Smartphone
- WhatsApp alerts for faults
- SMS alerts via GSM
- Location tracking (if GPS enabled)

---

## 🎉 You're Done!

Your BMS is now:
- ✅ Monitoring all battery parameters
- ✅ Protecting against faults
- ✅ Logging to cloud
- ✅ Sending alerts when needed
- ✅ Estimating SOH and RUL

---

## 📚 Next Steps

1. **Fine-tune thresholds** for your specific battery
2. **Set up cloud dashboard** for remote monitoring  
3. **Enable optional features** (GPS, accelerometer)
4. **Configure WhatsApp alerts** for critical events
5. **Install in your EV** and start monitoring!

---

## 🆘 Need Help?

Check these resources:
- 📖 README.md - Full documentation
- 📝 IMPROVEMENTS.md - Detailed feature list
- 💻 Serial Monitor - Real-time diagnostics
- 🔍 GitHub Issues - Community support

---

**Happy Monitoring! Your battery is in safe hands! 🔋🛡️**
