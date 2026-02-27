# Hardware Reference

**Project:** LoRa Cellular Gateway
**Version:** 1.0
**Date:** February 25, 2026

---

## Gateway Hardware

### Core Components

| Component | Part Number | Qty | Supplier | Unit Cost |
|-----------|-------------|-----|----------|-----------|
| **MCU Board** | Adafruit Feather nRF52840 Express | 1 | Adafruit | $35 |
| **LoRa Radio** | RFM95W (SX1276) 915 MHz | 1 | Adafruit/DigiKey | $10 |
| **Cellular Module** | Blues Note-WBNAN (LTE-M/NB-IoT) | 1 | Blues | $50 |
| **Carrier Board** | Blues NoteCarrier F | 1 | Blues | $20 |
| **MOSFET Switch** | Pololu High-Side MOSFET (SKU: 2679) | 1 | Pololu | $5 |
| **Antenna** | LoRa Whip 915 MHz (u.FL to SMA) | 1 | Adafruit | $5 |
| **Battery** | LiPo 3.7V 2000 mAh (JST-PH 2mm) | 1 | Adafruit/SparkFun | $10 |
| **Solar Panel** | 5V 1W (optional) | 1 | Adafruit/SparkFun | $15 |
| **Enclosure** | IP65 Waterproof Box (100x60x40mm) | 1 | Amazon/McMaster | $10 |
| **Capacitors** | See below | 2-3 | DigiKey/Mouser | $0.50 |

**Total:** ~$145 (without solar), ~$160 (with solar)

---

## Power Circuit

### Schematic

```
┌─────────────────────────────────────────────────────────────────┐
│                  GATEWAY POWER SCHEMATIC                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  LiPo Battery (3.7V, 2000 mAh)                                  │
│       │                                                         │
│      ┌┴┐                                                        │
│      │ │ 10 µF 6.3V ceramic (C1)                               │
│      └┬┘   (optional, close to battery)                         │
│       │                                                         │
│       ├─── VIN ─────────────────────┐                          │
│       │                             │                          │
│       │    Pololu High-Side         │                          │
│       │    MOSFET Switch            │                          │
│       │    ┌─────────────┐          │                          │
│       └───→│ VIN      GND│──────────┼── GND                   │
│            │             │          │                          │
│  nRF52840  │             │          │                          │
│  D5 (GPIO)─→│ CTRL   VOUT│────┬─────┴────→ NoteCarrier-F VCC  │
│            │             │    │                                 │
│  GND ──────→│ GND    GND│────┼─────────→ NoteCarrier-F GND    │
│            └─────────────┘    │                                 │
│                              │                                 │
│  NoteCarrier-F               │                                 │
│  ┌──────────────────┐       ┌┴┐                                │
│  │ VCC    GND       │       │ │ 47-100 µF 6.3V (C2)           │
│  │ SDA    SCL       │       │ │ tantalum (optional)           │
│  │ TX     RX        │       └┬┘                                │
│  │ RST    EN        │        │                                 │
│  │ Notecard Module  │       ┌┴┐                                │
│  │ [Cellular]       │       │ │ 0.1 µF 16V ceramic (C3)       │
│  │                  │       │ │ ceramic (optional, close)     │
│  └──────────────────┘       └┬┘                                │
│       │                      │                                 │
│       └──────────────────────┴──────── GND                     │
│                                                                 │
│  Capacitor Summary:                                             │
│  ┌────┬──────────────┬─────────────┬──────────────────────┐   │
│  │ Ref│ Value        │ Type        │ Notes                │   │
│  ├────┼──────────────┼─────────────┼──────────────────────┤   │
│  │ C1 │ 10 µF 6.3V   │ Ceramic     │ Battery bypass (opt) │   │
│  │ C2 │ 47-100 µF    │ Tantalum    │ Bulk capacitance*    │   │
│  │ C3 │ 0.1 µF 16V   │ Ceramic     │ HF bypass (opt)      │   │
│  └────┴──────────────┴─────────────┴──────────────────────┘   │
│                                                                 │
│  * C2 recommended if:                                          │
│    - Battery leads > 10cm (4 inches)                           │
│    - Cold weather operation (<0°C)                             │
│    - Maximum reliability desired                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### NoteCarrier-F Onboard Capacitance

**NoteCarrier-F includes ~32 µF onboard:**
- 22 µF input filtering
- 10 µF Notecard decoupling

**Is external capacitance needed?**

| Condition | Onboard Only | With External C2 |
|-----------|--------------|------------------|
| Short battery leads (<10cm) | ✅ OK | ✅ Better |
| Long battery leads (>10cm) | ⚠️ May brownout | ✅ Recommended |
| Cold weather (<0°C) | ⚠️ Risk | ✅ Recommended |
| Low battery (<3.5V) | ⚠️ Risk | ✅ Recommended |
| Prototyping | ✅ OK | ✅ Good practice |

**Recommendation:** Add 47-100 µF tantalum (C2) for ~$0.20 insurance against brownouts.

---

## Pin Connections

### nRF52840 to RFM95W (LoRa)

| nRF52840 | RFM95W | Function |
|----------|--------|----------|
| D10 (P0.06) | NSS | SPI Chip Select |
| D11 (P0.08) | MOSI | SPI Data Out |
| D12 (P0.10) | MISO | SPI Data In |
| D13 (P0.12) | SCK | SPI Clock |
| D5 (P0.29) | RST | Reset (optional) |
| D4 (P0.04) | DIO0 | RX/TX Interrupt |
| D7 (P0.27) | DIO1 | Optional (CAD) |
| 3.3V | 3.3V | Power |
| GND | GND | Ground |

### nRF52840 to NoteCarrier-F (Blues)

| nRF52840 | NoteCarrier-F | Function |
|----------|---------------|----------|
| D20/A4 (P0.30) | SDA | I2C Data |
| D21/A5 (P0.31) | SCL | I2C Clock |
| D5 (P0.29) | EN | Enable (optional, via MOSFET) |
| 3.3V | VCC | Power (via MOSFET switch) |
| GND | GND | Ground |

### nRF52840 to Pololu MOSFET Switch

| nRF52840 | Pololu | Function |
|----------|--------|----------|
| D5 (P0.29) | CTRL | MOSFET Gate Control |
| GND | GND | Ground |
| (from battery) | VIN | Power Input |
| (to Notecard) | VOUT | Switched Power Output |

---

## Assembly Guide

### Step 1: Prepare Power Circuit

1. **Solder Pololu MOSFET switch:**
   - VIN: Connect to battery positive (red)
   - GND: Connect to battery negative (black)
   - VOUT: Will connect to NoteCarrier-F VCC
   - CTRL: Will connect to nRF52840 D5

2. **Add bulk capacitor (C2, optional but recommended):**
   - Solder 47-100 µF tantalum between VOUT and GND
   - Place close to NoteCarrier-F VCC pin
   - Observe polarity (+ to VOUT, - to GND)

3. **Add high-frequency bypass (C3, optional):**
   - Solder 0.1 µF ceramic directly on NoteCarrier-F VCC/GND pins
   - As close as possible to the module

### Step 2: Connect RFM95W to nRF52840

1. **Solder headers to RFM95W** (if not pre-soldered)
2. **Connect SPI wires:**
   - Use 22-26 AWG stranded wire
   - Keep wires short (<15cm)
   - Twist MOSI/MISO/GND together (reduces noise)
3. **Connect antenna:**
   - Attach u.FL connector to RFM95W
   - Connect SMA pigtail to external antenna

### Step 3: Connect NoteCarrier-F

1. **Mount Notecard on NoteCarrier-F:**
   - Align notches
   - Press firmly until clips engage
2. **Connect I2C wires:**
   - SDA (D20/A4) → SDA
   - SCL (D21/A5) → SCL
   - Keep I2C wires short (<10cm)
3. **Connect power:**
   - VCC → Pololu VOUT (with capacitor)
   - GND → Pololu GND (common ground)

### Step 4: Connect Control Signals

1. **MOSFET control:**
   - D5 (P0.29) → Pololu CTRL
   - Use 22-26 AWG wire
2. **LoRa interrupt:**
   - D4 (P0.04) → RFM95W DIO0
   - Critical for wake-on-packet!

### Step 5: Test Before Enclosure

1. **Power test:**
   - Connect battery
   - Measure VOUT on Pololu (should be ~3.7V)
   - Set D5 HIGH, verify Notecard powers on
2. **Serial test:**
   - Upload test sketch
   - Open serial monitor (115200 baud)
   - Verify "Gateway ready" message
3. **LoRa test:**
   - Run loopback test (TX → RX)
   - Verify interrupt fires on packet received
4. **Blues test:**
   - Run Blues test sketch
   - Verify Notecard responds
   - Check signal strength

### Step 6: Install in Enclosure

1. **Mount components:**
   - Use standoffs for PCBs
   - Secure battery with foam tape
   - Route antenna cable through grommet
2. **Waterproofing:**
   - Apply silicone to cable entries
   - Use IP65-rated enclosure
   - Test seal before deployment
3. **Solar connection (optional):**
   - Connect solar panel to LiPo charging circuit
   - Verify charging current (<500 mA for small panels)

---

## Node Hardware

### Core Components

| Component | Part Number | Qty | Supplier | Unit Cost |
|-----------|-------------|-----|----------|-----------|
| **MCU Board** | Adafruit Feather nRF52840 Express | 1 | Adafruit | $35 |
| **LoRa Radio** | RFM95W (SX1276) 915 MHz | 1 | Adafruit/DigiKey | $10 |
| **Sensor** | Application-specific | 1 | Various | $5-20 |
| **Antenna** | LoRa Whip 915 MHz (u.FL to SMA) | 1 | Adafruit | $5 |
| **Battery** | LiPo 3.7V 2000 mAh | 1 | Adafruit/SparkFun | $10 |
| **Solar Panel** | 5V 0.5W (optional) | 1 | Adafruit/SparkFun | $10 |
| **Enclosure** | IP65 Waterproof Box | 1 | Amazon/McMaster | $10 |

**Total:** ~$75-90 per node (without solar), ~$85-100 (with solar)

### Sensor Interfaces

| Sensor Type | Interface | Example Parts |
|-------------|-----------|---------------|
| **Wind/Vibration** | Analog (A0) | Piezo + rectifier |
| **Motion (PIR)** | Digital (D2) | HC-SR501 |
| **Temperature/Humidity** | I2C | BME280, SHT31 |
| **Accelerometer** | I2C | ADXL343, LIS3DH |
| **Button/Switch** | Digital (D2) | Any momentary switch |
| **Light** | Analog (A0) | Photoresistor + resistor |

---

## Tools Required

| Tool | Purpose |
|------|---------|
| **Soldering iron** (fine tip) | Component assembly |
| **Solder** (rosin core, 60/40) | Joints |
| **Wire** (22-26 AWG, stranded) | Connections |
| **Heat shrink tubing** | Insulation |
| **Multimeter** | Voltage/current testing |
| **Oscilloscope** (optional) | Signal debugging |
| **Hot glue gun** | Strain relief |
| **Silicone sealant** | Waterproofing |

---

## Troubleshooting

### Gateway Won't Boot

| Symptom | Check | Fix |
|---------|-------|-----|
| No serial output | Battery voltage | Charge/replace battery |
| Brownout on TX | Capacitor C2 | Add 47-100 µF |
| Notecard not found | I2C connections | Check SDA/SCL wiring |
| LoRa not receiving | DIO0 connection | Verify interrupt pin |

### High Battery Drain

| Symptom | Check | Fix |
|---------|-------|-----|
| Battery dies in days | Notecard always on | Add MOSFET switch |
| Current > 1 mA in sleep | Peripherals powered | Check all enable pins |
| Voltage droop | Long battery leads | Add bulk capacitance |

### Communication Failures

| Symptom | Check | Fix |
|---------|-------|-----|
| No packets received | Antenna connection | Verify u.FL seated |
| Weak signal | RSSI values | Check antenna placement |
| Intermittent | Wire connections | Re-solder joints |

---

## Safety Notes

⚠️ **LiPo Battery Safety:**
- Never discharge below 3.0V
- Never charge above 4.2V
- Use LiPo charging circuit
- Store in fireproof container
- Dispose properly (recycling center)

⚠️ **RF Safety:**
- Don't transmit with antenna disconnected
- Keep antenna away from metal (detunes)
- Maintain FCC power limits (17 dBm max)

⚠️ **Cellular Regulations:**
- Use certified modules (Blues Notecard is FCC certified)
- Don't modify RF circuitry
- Register with carrier if required

---

## Supplier Links

### Primary Suppliers

- **Adafruit:** https://adafruit.com
- **DigiKey:** https://digikey.com
- **Mouser:** https://mouser.com
- **Pololu:** https://pololu.com
- **Blues:** https://blues.io

### Alternative (Budget)

- **AliExpress:** Cheaper, longer shipping
- **Amazon:** Fast, but verify authenticity

---

**End of Hardware Reference**
