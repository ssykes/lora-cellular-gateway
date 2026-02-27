# Quick Reference

**Project:** LoRa Cellular Gateway
**Version:** 1.0
**Date:** February 25, 2026

---

## Build Commands

```bash
# Build gateway (default)
pio run -e gateway_nrf52840

# Build node
pio run -e node_nrf52840

# Upload to gateway
pio run -e gateway_nrf52840 -t upload

# Upload to node
pio run -e node_nrf52840 -t upload

# Serial monitor (gateway)
pio device monitor -e gateway_nrf52840 -b 115200

# Serial monitor (node)
pio device monitor -e node_nrf52840 -b 115200

# Clean build
pio run -t clean

# Build + upload + monitor
pio run -e gateway_nrf52840 -t upload && pio device monitor -e gateway_nrf52840 -b 115200
```

---

## Build Environments

| Environment | Board | Purpose | Default |
|-------------|-------|---------|---------|
| `gateway_nrf52840` | Adafruit Feather nRF52840 | Gateway (nRF52840 + RFM95W + Blues) | ✅ Yes |
| `gateway_rak4630` | RAK4630 | Alternative gateway | ❌ No |
| `node_nrf52840` | Adafruit Feather nRF52840 | Sensor node | ❌ No |
| `node_heltec` | HelTec WiFi LoRa 32 V5 | Alternative node | ❌ No |

---

## Pin Mappings

### Gateway (Adafruit Feather nRF52840 + RFM95W + Blues)

```cpp
// RFM95W LoRa Radio (SPI)
#define RADIO_CS_PIN      10        // P0.06 (SPI SS)
#define RADIO_RST_PIN     8         // P0.08
#define RADIO_DIO0_PIN    4         // P0.04 (RX/TX interrupt)
#define RADIO_DIO1_PIN    7         // P0.27 (optional)

// Blues Notecard (I2C)
#define NOTECARD_SDA      PIN_WIRE_SDA  // D20/A4
#define NOTECARD_SCL      PIN_WIRE_SCL  // D21/A5

// LEDs (on Feather nRF52840)
#define LED_GREEN         PIN_LED1    // Red LED
#define LED_BLUE          PIN_LED2    // Blue LED
```

### Node (Adafruit Feather nRF52840 + RFM95W)

```cpp
// RFM95W LoRa Radio (SPI) - Same as gateway
#define RADIO_CS_PIN      10
#define RADIO_RST_PIN     8
#define RADIO_DIO0_PIN    4
#define RADIO_DIO1_PIN    7

// Sensor Interfaces
#define SENSOR_ANALOG     A0        // Analog sensor input
#define SENSOR_DIGITAL    D2        // Digital interrupt input
#define SENSOR_I2C_SDA    PIN_WIRE_SDA  // I2C sensors
#define SENSOR_I2C_SCL    PIN_WIRE_SCL
```

---

## LoRa Radio Settings

### Current Configuration (SF9 - Balanced)

```cpp
#define LORA_FREQUENCY          915.0f      // MHz (North America)
#define LORA_BANDWIDTH          125.0f      // kHz
#define LORA_SPREADING_FACTOR   9           // SF7-SF12 (SF9 = balance)
#define LORA_CODING_RATE        5           // 5-8 (4/5 coding)
#define LORA_PREAMBLE_LENGTH    8           // Symbols
#define LORA_SYNC_WORD          0x12        // Private network
#define LORA_TX_POWER           17          // dBm (max for RFM95W)
#define LORA_CRC_ENABLED        true
```

### Alternative Configurations

**Fast (short range, ~500m):**
```cpp
#define LORA_SPREADING_FACTOR   7
#define LORA_BANDWIDTH          500.0f
// Airtime: ~5ms, Range: ~500m
```

**Long Range (slow, ~5+ km):**
```cpp
#define LORA_SPREADING_FACTOR   12
#define LORA_BANDWIDTH          125.0f
// Airtime: ~200ms, Range: ~5+ km
```

---

## Packet Structures

### Sensor Event Packet (15 bytes)

```
┌──────────┬──────────┬──────────┬──────────┬───────────┐
│ Type     │ Node ID  │ Hop Count│ RSSI     │ Config Ver│
│ 0x01     │ 1-255    │ 0        │ -dBm     │ 1-255     │
├──────────┴──────────┴──────────┴──────────┴───────────┤
│ Value 1 (float, 4 bytes) - Primary sensor             │
├───────────────────────────────────────────────────────┤
│ Value 2 (float, 4 bytes) - Secondary sensor           │
├───────────────────────────────────────────────────────┤
│ Battery mV (uint16, 2 bytes)                          │
├───────────────────────────────────────────────────────┤
│ Sensor Flags (uint8, 1 byte)                          │
├───────────────────────────────────────────────────────┤
│ CRC (uint16, 2 bytes)                                 │
└───────────────────────────────────────────────────────┘
```

### Config Update Packet (11 bytes)

```
┌──────────┬──────────┬──────────┬──────────┬───────────┐
│ Type     │ Node ID  │ Version  │ Interval │ Threshold │
│ 0x02     │ 0-255    │ 1-255    │ minutes  │ 0-100     │
├──────────┴──────────┴──────────┴──────────┴───────────┤
│ TX Power (dBm)                                        │
├───────────────────────────────────────────────────────┤
│ CRC (uint16, 2 bytes)                                 │
└───────────────────────────────────────────────────────┘
```

---

## Blues Environment Variables

### Set via Blues Dashboard

```
https://notehub.io → Products → [Your Product] → Devices → Environment
```

### Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `gateway_listen_start` | int | 8 | Listen window start (08:00) |
| `gateway_listen_end` | int | 22 | Listen window end (22:00) |
| `gateway_sync_interval` | int | 360 | Env var sync (minutes) |
| `node_tx_interval` | int | 30 | Node TX interval (if periodic) |
| `node_threshold` | int | 50 | Trigger threshold (0-100) |
| `node_tx_power` | int | 17 | TX power (2-17 dBm) |
| `config_version` | int | 1 | Increment on change |

### Example Configuration

**Wind Chime Installation:**
```
gateway_listen_start: 8
gateway_listen_end: 22
gateway_sync_interval: 360
node_threshold: 45
node_tx_power: 17
config_version: 1
```

**Environmental Monitoring:**
```
gateway_listen_start: 0
gateway_listen_end: 24
gateway_sync_interval: 60
node_tx_interval: 30
node_threshold: 30
node_tx_power: 14
config_version: 2
```

---

## Power Budget

### Gateway (24 hours)

| State | Current | Duration | mAh/day |
|-------|---------|----------|---------|
| Deep sleep | 6 µA | 14 hrs (22:00-08:00) | 0.08 |
| Active listen | 6 mA | 10 hrs (08:00-22:00) | 60 |
| Cellular TX | 150 mA | ~30 sec | 1.25 |
| Env var sync | 25 mA | ~5 min (4x) | 8.3 |
| **Total** | - | - | **~70 mAh/day** |

**Battery life:** 2000 mAh → ~28 days (indefinite with solar)

### Node (per event)

| State | Current | Duration | mAh/event |
|-------|---------|----------|-----------|
| Deep sleep | 6 µA | Between events | Base |
| Wake + TX | 100 mA | ~50ms | 0.0014 |

**Battery life:** 50 events/day → ~130 days on 2000 mAh

---

## Serial Commands

### Gateway Console

```
# Force config sync (for testing)
SYNC_CONFIG

# Show current config
SHOW_CONFIG

# Show Blues status
BLUES_STATUS

# Reboot gateway
REBOOT
```

### Node Console

```
# Show current config
SHOW_CONFIG

# Force sensor read
READ_SENSOR

# Send test packet
TEST_TX

# Reboot node
REBOOT
```

---

## Troubleshooting

### Gateway Won't Boot

```bash
# Check board detection
pio device list

# Check build errors
pio run -e gateway_nrf52840 -v

# Try manual upload
pio run -e gateway_nrf52840 -t upload -f
```

### Blues Notecard Not Found

```
1. Check I2C connections (SDA/SCL)
2. Verify 3.3V power to Notecard
3. Check product UID in code
4. Monitor serial output for error messages
```

### LoRa RX Not Working

```
1. Check SPI connections (CS, MOSI, MISO, SCK)
2. Verify RFM95W reset pin
3. Check antenna connection
4. Test with known-good TX node
5. Monitor RSSI values (should be -50 to -100 dBm)
```

### High Battery Drain

```
1. Check deep sleep current (should be ~6 µA)
2. Verify all peripherals powered down
3. Check for stuck interrupts
4. Reduce env var sync interval
5. Shorten listen window
```

---

## File Locations

```
hummingbird-blues-gateway/
├── src/
│   ├── common/
│   │   ├── protocol.h          # Packet definitions
│   │   ├── protocol.cpp        # CRC, builders
│   │   ├── radio_config.h      # LoRa settings
│   │   └── config_types.h      # Config structures
│   ├── gateway/
│   │   └── gateway_main.cpp    # Gateway main loop
│   └── node/
│       └── node_main.cpp       # Node main loop (TODO)
├── platformio.ini              # Build configuration
├── README.md                   # Project overview
├── DESIGN.md                   # Detailed design
├── QUICK_REFERENCE.md          # This file
└── IMPLEMENTATION_STATUS.md    # Progress tracking
```

---

## Useful Links

- **Blues Notehub Dashboard:** https://notehub.io
- **Blues Documentation:** https://dev.blues.io
- **RadioLib Documentation:** https://github.com/jgromes/RadioLib
- **nRF52840 Datasheet:** https://infocenter.nordicsemi.com/topic/ps_nrf52840

---

**End of Quick Reference**
