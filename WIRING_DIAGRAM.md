# 🔌 Complete BMS Wiring Diagram

## 📐 System Overview

This document provides detailed wiring diagrams for the complete Battery Management System.

---

## 🎨 Color Code Legend

```
Red     = Power (+5V or +3.3V)
Black   = Ground (GND)
Yellow  = Signal/Data
Blue    = Analog Input
Green   = Digital I/O
Orange  = Communication (TX/RX)
Purple  = I2C (SDA/SCL)
```

---

## 📊 Complete System Wiring Diagram

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                               ESP32 DevKit Board                                    │
│                                                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │   3.3V   │  │    5V    │  │   GND    │  │   GND    │  │   GND    │           │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘           │
│       │             │             │             │             │                    │
│       │             │             │             │             │                    │
│  GPIO 34 ◄──────────────────────────────────────────── Pack Voltage               │
│  GPIO 35 ◄──────────────────────────────────────────── Cell 1 Voltage             │
│  GPIO 32 ◄──────────────────────────────────────────── Cell 2 Voltage             │
│  GPIO 33 ◄──────────────────────────────────────────── Cell 3 Voltage             │
│  GPIO 25 ◄──────────────────────────────────────────── Cell 4 Voltage             │
│                                                                                     │
│  GPIO 39 ◄──────────────────────────────────────────── ACS712 Output              │
│                                                                                     │
│  GPIO 4  ──────────────────────────────────────────► DHT11 Cell 1                 │
│  GPIO 5  ──────────────────────────────────────────► DHT11 Cell 2                 │
│  GPIO 18 ──────────────────────────────────────────► DHT11 Ambient                │
│  GPIO 19 ──────────────────────────────────────────► DHT11 Pack                   │
│                                                                                     │
│  GPIO 13 ──────────────────────────────────────────► Charge Relay IN              │
│  GPIO 12 ──────────────────────────────────────────► Discharge Relay IN           │
│  GPIO 14 ──────────────────────────────────────────► Fan Relay IN                 │
│  GPIO 15 ──────────────────────────────────────────► Heater Relay IN              │
│                                                                                     │
│  GPIO 21 ◄─────────────────────────────────────────► I2C SDA (LCD/Accel)          │
│  GPIO 22 ◄─────────────────────────────────────────► I2C SCL (LCD/Accel)          │
│  GPIO 23 ◄──────────────────────────────────────────── Accelerometer INT          │
│                                                                                     │
│  GPIO 16 ──────────────────────────────────────────► GSM RX                       │
│  GPIO 17 ◄──────────────────────────────────────────── GSM TX                     │
│                                                                                     │
│  GPIO 26 ◄──────────────────────────────────────────── GPS TX                     │
│  GPIO 27 ──────────────────────────────────────────► GPS RX                       │
│                                                                                     │
│  GPIO 2  ──────────────────────────────────────────► LED Normal (Green)           │
│  GPIO 17 ──────────────────────────────────────────► LED Warning (Yellow)         │
│  GPIO 16 ──────────────────────────────────────────► LED Fault (Red)              │
│                                                                                     │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔋 1. Voltage Divider Circuit (Per Cell/Pack)

### Circuit Diagram

```
                    Battery Cell/Pack
                           │
                           │ Positive Terminal
                           │
                       ┌───┴───┐
                       │       │
                       │ 47kΩ  │  R1 (Precision 1% resistor)
                       │       │
                       └───┬───┘
                           │
                           ├──────────► To ESP32 ADC Pin
                           │             (GPIO 34/35/32/33/25)
                       ┌───┴───┐
                       │       │
                       │ 10kΩ  │  R2 (Precision 1% resistor)
                       │       │
                       └───┬───┘
                           │
                           │ Ground
                           ▼
                          GND

Calculation:
Vout = Vin × (R2 / (R1 + R2))
Vout = Vin × (10kΩ / 57kΩ)
Vout = Vin × 0.175

For 4.2V cell: Vout = 0.735V ✓ (Safe for ESP32 ADC)
For 16.8V pack: Vout = 2.94V ✓ (Safe for ESP32 ADC)
```

### Wiring Table

| Battery Point | R1 (47kΩ) | Junction | R2 (10kΩ) | ESP32 Pin | GND |
|--------------|-----------|----------|-----------|-----------|-----|
| Pack (+)     | ─────────►│◄─────── | ────────► | GPIO 34   | ──► |
| Cell 1 (+)   | ─────────►│◄─────── | ────────► | GPIO 35   | ──► |
| Cell 2 (+)   | ─────────►│◄─────── | ────────► | GPIO 32   | ──► |
| Cell 3 (+)   | ─────────►│◄─────── | ────────► | GPIO 33   | ──► |
| Cell 4 (+)   | ─────────►│◄─────── | ────────► | GPIO 25   | ──► |

### ⚠️ Important Notes

1. **Use 1% tolerance resistors** for accuracy
2. **Add 0.1µF capacitor** across R2 for noise filtering
3. **Keep wires short** to minimize noise pickup
4. **Use shielded cable** if wires are long (>20cm)

---

## ⚡ 2. Current Sensor (ACS712-30A)

### Circuit Diagram

```
                    Battery System
                         │
        ┌────────────────┼────────────────┐
        │                │                │
        │            ┌───┴───┐            │
        │            │       │            │
        │            │ LOAD  │            │
        │            │   OR  │            │
        │            │CHARGER│            │
        │            │       │            │
        │            └───┬───┘            │
        │                │                │
   ┌────┴────┐      ┌────┴────┐     ┌────┴────┐
   │ Battery │      │ ACS712  │     │ Battery │
   │   (+)   │─────►│   IP+   │     │   (-)   │
   └─────────┘      │         │     └────┬────┘
                    │   IP-   │◄─────────┘
                    │         │
                    │   VCC   │◄───── 5V (ESP32)
                    │   GND   │◄───── GND
                    │   OUT   │─────► GPIO 39 (ESP32)
                    └─────────┘

Current Flow Direction:
• Charging:    Current flows Battery ← Charger (negative reading)
• Discharging: Current flows Battery → Load (positive reading)
```

### Wiring Connections

| ACS712 Pin | Connection | Wire Color | Notes |
|------------|------------|------------|-------|
| VCC        | ESP32 5V   | Red        | 5V power supply |
| GND        | ESP32 GND  | Black      | Common ground |
| OUT        | GPIO 39    | Blue       | Analog signal (2.5V ± 0.066V/A) |
| IP+        | Battery +  | Red        | Current input (positive) |
| IP-        | Load/Charger | Black    | Current output (negative) |

### 📊 ACS712 Output Characteristics

```
Current (A)  │  Output Voltage (V)
─────────────┼──────────────────────
    -30      │      0.52
    -20      │      1.18
    -10      │      1.84
      0      │      2.50  ← Zero current (calibration point)
    +10      │      3.16
    +20      │      3.82
    +30      │      4.46
```

---

## 🌡️ 3. Temperature Sensors (DHT11) x4

### Circuit Diagram (Each DHT11)

```
        ESP32                          DHT11
      ┌────────┐                    ┌─────────┐
      │  5V    │───────Red─────────►│ VCC (1) │
      │        │                    │         │
      │ GPIO X │◄──────Yellow──────►│ DATA(2) │◄──┐
      │        │                    │         │   │
      │  GND   │───────Black───────►│ NC  (3) │   │
      │        │                    │         │   │
      │  GND   │───────Black───────►│ GND (4) │   │
      └────────┘                    └─────────┘   │
                                          │       │
                                        ┌─┴─┐     │
                                        │   │     │
                                        │10kΩ│────┘ Pull-up resistor
                                        │   │
                                        └─┬─┘
                                          │
                                          5V
```

### Wiring Table (All 4 Sensors)

| Sensor | Purpose | VCC | DATA Pin | GND | Pull-up |
|--------|---------|-----|----------|-----|---------|
| DHT11 #1 | Cell 1 Temp | 5V | GPIO 4 | GND | 10kΩ to 5V |
| DHT11 #2 | Cell 2 Temp | 5V | GPIO 5 | GND | 10kΩ to 5V |
| DHT11 #3 | Ambient | 5V | GPIO 18 | GND | 10kΩ to 5V |
| DHT11 #4 | Pack Temp | 5V | GPIO 19 | GND | 10kΩ to 5V |

### Mounting Recommendations

```
┌─────────────────────────────────────┐
│        Battery Pack (Top View)      │
│                                     │
│  [DHT11 #1]              [DHT11 #2]│  ← On cells
│                                     │
│    Cell 1   Cell 2   Cell 3   Cell 4│
│    ═══╤═══ ═══╤═══ ═══╤═══ ═══╤═══ │
│       └───────┴───────┴───────┘     │
│                                     │
│  [DHT11 #4]                         │  ← Pack temperature
│                                     │
└─────────────────────────────────────┘

[DHT11 #3] ← Mounted outside pack (ambient)
```

---

## 🔄 4. Relay Module (4-Channel)

### Circuit Diagram

```
        ESP32                    Relay Module                   Devices
     ┌─────────┐              ┌──────────────┐
     │         │              │              │
     │ GPIO 13 │─────────────►│ IN1  COM1 ◄──┼──── Charger (+)
     │         │              │      NO1  ───┼────► Battery (+)
     │ GPIO 12 │─────────────►│ IN2  COM2 ◄──┼──── Battery (+)
     │         │              │      NO2  ───┼────► Load (+)
     │ GPIO 14 │─────────────►│ IN3  COM3 ◄──┼──── Fan (+)
     │         │              │      NO3  ───┼────► 12V Supply
     │ GPIO 15 │─────────────►│ IN4  COM4 ◄──┼──── Heater (+)
     │         │              │      NO4  ───┼────► 12V Supply
     │   5V    │─────────────►│ VCC          │
     │   GND   │─────────────►│ GND          │
     └─────────┘              └──────────────┘

Relay Contact Types:
• COM = Common terminal
• NO  = Normally Open (closes when relay activated)
• NC  = Normally Closed (opens when relay activated)
```

### Relay Wiring Details

| Relay | Function | IN Pin | COM Connection | NO Connection | Load |
|-------|----------|--------|----------------|---------------|------|
| 1 | Charging | GPIO 13 | Charger (+) | Battery (+) | 30A |
| 2 | Discharge | GPIO 12 | Battery (+) | Load (+) | 30A |
| 3 | Cooling Fan | GPIO 14 | Fan (+) | 12V Supply | 2A |
| 4 | Heater | GPIO 15 | Heater (+) | 12V Supply | 5A |

### 🔌 Complete Relay Connections

```
Relay 1 (Charging):
┌────────┐
│Charger │ (+) ───► COM1
└────────┘          NO1 ───► Battery (+)
                    
Relay 2 (Discharge):  
┌────────┐
│Battery │ (+) ───► COM2
└────────┘          NO2 ───► Load (+)

Relay 3 (Fan):
┌────────┐
│12V Sup │ (+) ───► COM3
└────────┘          NO3 ───► Fan (+)
                             Fan (-) ───► GND

Relay 4 (Heater):
┌────────┐
│12V Sup │ (+) ───► COM4
└────────┘          NO4 ───► Heater (+)
                             Heater (-) ───► GND
```

---

## 📡 5. GSM Module (SIM800L)

### Circuit Diagram

```
    ESP32                           SIM800L
 ┌─────────┐                     ┌──────────┐
 │         │                     │          │
 │ GPIO 16 │────Orange (TX)─────►│ RX       │
 │         │                     │          │
 │ GPIO 17 │◄───Orange (RX)──────│ TX       │
 │         │                     │          │
 │         │                     │ VCC  ────┼───► 4.2V (Li-ion)
 │         │                     │          │     or 3.7-4.2V
 │         │                     │          │
 │   GND   │────Black (GND)─────►│ GND      │
 └─────────┘                     │          │
                                 │ RST      │  Not connected
                                 │          │
                                 │ SIM Card │◄── Insert SIM
                                 └──────────┘
                                      │││
                                   Antenna

⚠️ CRITICAL: SIM800L needs 3.7-4.2V, NOT 5V!
Use dedicated Li-ion battery or buck converter from 5V
```

### Power Supply Options

**Option 1: Dedicated Li-ion Battery (Recommended)**
```
Li-ion Battery (3.7V) ───► SIM800L VCC
                      └──► SIM800L GND
```

**Option 2: Buck Converter from 5V**
```
ESP32 5V ───► Buck Converter IN+ ───► 4V ───► SIM800L VCC
ESP32 GND ──► Buck Converter IN- ──► GND ──► SIM800L GND
```

### Wiring Table

| SIM800L Pin | ESP32 Pin | Wire Color | Notes |
|-------------|-----------|------------|-------|
| RX | GPIO 16 (TX2) | Orange | ESP32 transmits to GSM |
| TX | GPIO 17 (RX2) | Orange | ESP32 receives from GSM |
| VCC | 4.2V source | Red | **Not 5V!** Use Li-ion or converter |
| GND | GND | Black | Common ground |
| RST | Not connected | - | Optional reset |

---

## 📍 6. GPS Module (NEO-6M) - Optional

### Circuit Diagram

```
    ESP32                          NEO-6M GPS
 ┌─────────┐                     ┌──────────┐
 │         │                     │          │
 │ GPIO 27 │────Orange (TX)─────►│ RX       │
 │         │                     │          │
 │ GPIO 26 │◄───Orange (RX)──────│ TX       │
 │         │                     │          │
 │   5V    │────Red (VCC)───────►│ VCC      │
 │         │                     │          │
 │   GND   │────Black (GND)─────►│ GND      │
 └─────────┘                     │          │
                                 │ PPS      │  Not connected
                                 └──────────┘
                                      │
                                   Antenna (Built-in ceramic or external)
```

### Wiring Table

| NEO-6M Pin | ESP32 Pin | Wire Color | Notes |
|------------|-----------|------------|-------|
| RX | GPIO 27 (TX1) | Orange | ESP32 transmits to GPS |
| TX | GPIO 26 (RX1) | Orange | ESP32 receives from GPS |
| VCC | 5V | Red | 5V power |
| GND | GND | Black | Common ground |
| PPS | Not used | - | Optional pulse per second |

### Antenna Placement

```
Mount GPS with clear sky view:
        │
        │ GPS Antenna
        ▼
    [========]  ← NEO-6M Module
        ║
        ║ Cable
        ▼
    ESP32 Board

⚠️ Keep away from metal/interference
✓ Mount with antenna facing up
✓ Needs clear view of sky for fix
```

---

## 📊 7. Accelerometer (MPU6050) - Optional

### Circuit Diagram

```
    ESP32                          MPU6050
 ┌─────────┐                    ┌──────────┐
 │         │                    │          │
 │ GPIO 21 │◄────Purple (SDA)──►│ SDA      │
 │         │                    │          │
 │ GPIO 22 │◄────Purple (SCL)──►│ SCL      │
 │         │                    │          │
 │ GPIO 23 │◄────Green (INT)────│ INT      │
 │         │                    │          │
 │  3.3V   │────Red (VCC)───────►│ VCC      │
 │         │                    │          │
 │   GND   │────Black (GND)─────►│ GND      │
 │         │                    │          │
 │         │                    │ XDA      │  Not used
 │         │                    │ XCL      │  Not used
 │         │                    │ AD0  ────┼──► GND (Address 0x68)
 └─────────┘                    └──────────┘
```

### Wiring Table

| MPU6050 Pin | ESP32 Pin | Wire Color | Notes |
|-------------|-----------|------------|-------|
| SDA | GPIO 21 | Purple | I2C data (shared with LCD) |
| SCL | GPIO 22 | Purple | I2C clock (shared with LCD) |
| INT | GPIO 23 | Green | Interrupt for motion detection |
| VCC | 3.3V | Red | **Use 3.3V, not 5V!** |
| GND | GND | Black | Common ground |
| AD0 | GND | Black | Sets I2C address to 0x68 |

### Mounting Position

```
Mount accelerometer on battery pack:

        ┌────────────────────┐
        │   Battery Pack     │
        │                    │
        │  X →               │
        │  Y ↓   [MPU6050]   │  ← Securely mounted
        │  Z ⊙ (into page)   │
        │                    │
        └────────────────────┘

Axes orientation:
X = Forward/Backward
Y = Left/Right
Z = Up/Down
```

---

## 📺 8. LCD Display (16x2 I2C)

### Circuit Diagram

```
    ESP32                          LCD Display
 ┌─────────┐                    ┌──────────┐
 │         │                    │  16x2    │
 │ GPIO 21 │◄────Purple (SDA)──►│ SDA      │
 │         │                    │          │
 │ GPIO 22 │◄────Purple (SCL)──►│ SCL      │
 │         │                    │          │
 │   5V    │────Red (VCC)───────►│ VCC      │
 │         │                    │          │
 │   GND   │────Black (GND)─────►│ GND      │
 └─────────┘                    └──────────┘

I2C Address: Usually 0x27 or 0x3F
```

### Wiring Table

| LCD Pin | ESP32 Pin | Wire Color | Notes |
|---------|-----------|------------|-------|
| SDA | GPIO 21 | Purple | I2C data (shared with MPU6050) |
| SCL | GPIO 22 | Purple | I2C clock (shared with MPU6050) |
| VCC | 5V | Red | 5V power for backlight |
| GND | GND | Black | Common ground |

### I2C Address Finding

```cpp
// Run I2C scanner to find LCD address
#include <Wire.h>

void setup() {
  Wire.begin(21, 22);
  Serial.begin(115200);
  Serial.println("Scanning I2C...");
  
  for(byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(i, HEX);
    }
  }
}
```

---

## 💡 9. Status LEDs - Optional

### Circuit Diagram

```
    ESP32              Resistor         LED
 ┌─────────┐          ┌──────┐      ┌──────┐
 │         │          │      │      │  ╱▲  │
 │ GPIO 2  │─────────►│ 220Ω │─────►│ ╱  ▲ │──── GND  (Green - Normal)
 │         │          └──────┘      │   ║  │
 │         │                        └──────┘
 │         │          ┌──────┐      ┌──────┐
 │ GPIO 17 │─────────►│ 220Ω │─────►│ ╱  ▲ │──── GND  (Yellow - Warning)
 │         │          └──────┘      │   ║  │
 │         │                        └──────┘
 │         │          ┌──────┐      ┌──────┐
 │ GPIO 16 │─────────►│ 220Ω │─────►│ ╱  ▲ │──── GND  (Red - Fault)
 │         │          └──────┘      │   ║  │
 └─────────┘                        └──────┘

⚠️ Always use current-limiting resistor!
```

### LED Wiring Table

| LED | Color | ESP32 Pin | Resistor | Indicates |
|-----|-------|-----------|----------|-----------|
| LED1 | Green | GPIO 2 | 220Ω | Normal operation |
| LED2 | Yellow | GPIO 17 | 220Ω | Warning state |
| LED3 | Red | GPIO 16 | 220Ω | Fault condition |

---

## 🔋 10. Power Supply Diagram

### Complete Power Distribution

```
                        Main Power Supply
                               │
                         ┌─────┴─────┐
                         │  12V DC   │  From battery or adapter
                         │  Power    │
                         └─────┬─────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
           ┌────▼────┐    ┌────▼────┐   ┌────▼────┐
           │ Buck to │    │ Buck to │   │  Direct │
           │   5V    │    │  3.3V   │   │   12V   │
           └────┬────┘    └────┬────┘   └────┬────┘
                │              │              │
        ┌───────┼───────┐      │         ┌────┼────┐
        │       │       │      │         │         │
        ▼       ▼       ▼      ▼         ▼         ▼
      ESP32   Relay  DHT11  MPU6050    Fan     Heater
              Module  (×4)
              
                    ┌────┐
                    │4.2V│◄──── Separate Li-ion for SIM800L
                    └──┬─┘
                       │
                       ▼
                    SIM800L
```

### Power Requirements

| Component | Voltage | Current | Power | Notes |
|-----------|---------|---------|-------|-------|
| ESP32 | 5V | 500mA | 2.5W | Via USB or VIN |
| Relay Module | 5V | 200mA | 1W | 4-channel |
| DHT11 (×4) | 5V | 10mA | 0.2W | Total for 4 sensors |
| LCD 16x2 | 5V | 50mA | 0.25W | With backlight |
| MPU6050 | 3.3V | 5mA | 0.017W | Low power |
| NEO-6M GPS | 5V | 50mA | 0.25W | During acquisition |
| SIM800L | 3.7-4.2V | 2A (peak) | 8W | Use separate battery! |
| Cooling Fan | 12V | 200mA | 2.4W | When active |
| Heater | 12V | 500mA | 6W | When active |
| **TOTAL** | - | **~3.5A** | **~20W** | All components active |

---

## 🛠️ Assembly Tips

### 1. PCB Layout Recommendation

```
┌─────────────────────────────────────────────┐
│  ┌──────────┐                               │
│  │  ESP32   │  ← Microcontroller            │
│  │ DevKit   │                               │
│  └──────────┘                               │
│       ║║                                     │
│  ┌────╨╨─────────────────────┐              │
│  │  Voltage Dividers (×5)    │              │
│  │  [47kΩ] [10kΩ] [0.1µF]   │              │
│  └───────────────────────────┘              │
│                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ ACS712   │  │ Relay    │  │  LCD     │  │
│  │ Current  │  │ Module   │  │ 16x2 I2C │  │
│  └──────────┘  └──────────┘  └──────────┘  │
│                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ SIM800L  │  │ NEO-6M   │  │ MPU6050  │  │
│  │   GSM    │  │   GPS    │  │  Accel   │  │
│  └──────────┘  └──────────┘  └──────────┘  │
│                                              │
│  [Screw Terminals for Battery Connections]  │
└─────────────────────────────────────────────┘
```

### 2. Wire Management

```
Color Coding:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Power Rails:
  • 5V     = Thick Red
  • 3.3V   = Thin Red
  • GND    = Black
  
Signals:
  • Analog    = Blue
  • Digital   = Green
  • I2C       = Purple (twisted pair)
  • UART/SPI  = Orange (twisted pair)
  
High Current:
  • Battery   = Red (12-14 AWG)
  • Ground    = Black (12-14 AWG)
  • Relays    = Yellow (14-16 AWG)
```

### 3. Grounding Strategy

```
                    Battery (-)
                         │
                    Main Ground
                         │
         ┌───────────────┼───────────────┐
         │               │               │
      ESP32 GND     Relay GND      Sensor GND
         │               │               │
         └───────────────┴───────────────┘
              Single Point Ground
              
⚠️ Connect all grounds to single point
   to avoid ground loops!
```

---

## ✅ Pre-Connection Checklist

Before powering on:

- [ ] All voltage dividers use correct resistor values (47kΩ, 10kΩ)
- [ ] ACS712 is correctly oriented (IP+ to battery, IP- to load)
- [ ] SIM800L has separate 3.7-4.2V power supply
- [ ] All DHT11 sensors have 10kΩ pull-up resistors
- [ ] I2C devices share same SDA/SCL but have different addresses
- [ ] Relay module control signals are connected to correct GPIOs
- [ ] All grounds are connected to single point
- [ ] No short circuits between power rails
- [ ] Polarity is correct on all components
- [ ] Battery fuse is installed (recommended: 40A fuse)

---

## 🧪 Testing Procedure

### Step 1: Power Test (No Battery)
```
1. Connect only ESP32 to USB
2. Verify 5V and 3.3V on pins with multimeter
3. Check no components are hot
```

### Step 2: Sensor Test
```
1. Connect sensors one at a time
2. Upload test sketch
3. Verify readings in Serial Monitor
4. Check each sensor individually
```

### Step 3: Full System Test
```
1. Connect battery through fuse
2. Monitor voltage readings
3. Test current sensor (apply load)
4. Verify relay switching
5. Check cloud connectivity
```

---

## 🆘 Troubleshooting Guide

### No Power
- Check USB cable
- Verify power supply voltage
- Check for short circuits

### Wrong Voltage Readings
- Verify resistor values with multimeter
- Check voltage divider connections
- Recalibrate using known voltage

### Current Always 0A
- Check ACS712 VCC = 5V
- Verify current flows through IP+/IP-
- Recalibrate with no load

### Temperature Sensors Not Working
- Check pull-up resistors
- Verify VCC is 5V
- Try different DHT11 unit
- Check wiring order (VCC, DATA, NC, GND)

### I2C Devices Not Found
- Run I2C scanner
- Check SDA/SCL not swapped
- Verify pull-up resistors on I2C lines
- Ensure unique addresses

---

**Your wiring is complete! Proceed to software upload and testing.** 🎉
