# Hello World Test Guide

**Goal:** Send sensor data from node → Gateway → Blues Dashboard

---

## Quick Start

### 1. Build and Upload Node

```bash
# Build for Feather M0 RFM9x (simple, USB powered)
pio run -e node_feather_m0
pio run -e node_feather_m0 -t upload

# OR for nRF52840 + FeatherWing (deep sleep, battery powered)
pio run -e node_nrf52840
pio run -e node_nrf52840 -t upload
```

**Bootloader mode (Feather M0):** Double-press the reset button

### 2. Build and Upload Gateway

```bash
pio run -e gateway_nrf52840
pio run -e gateway_nrf52840 -t upload
```

### 3. Configure Blues Notehub

1. Go to https://notehub.io
2. Create a product (or use existing)
3. Note your **Product UID**

### 4. Watch Data Arrive

1. Power up both node and gateway
2. Open Serial Monitor:
   ```bash
   # Node
   pio device monitor -e node_feather_m0 -b 115200
   
   # Gateway
   pio device monitor -e gateway_nrf52840 -b 115200
   ```
3. Watch Blues Dashboard → Devices → [Your Gateway] → events.qo

---

## Expected Output

### Node Serial (Feather M0)
```
*** Node Hello World ***
Board: Adafruit Feather M0 RFM9x
Node ID: 1
TX Interval: 30 seconds

Radio initialized successfully
  Frequency: 915.0 MHz
  Spreading Factor: 10

*** Node ready - sending first packet ***

[TX 1] Sending sensor data...
Sensors: temp=22.5C, humidity=45.0%, battery=3850mV
Transmitting packet...
TX successful!
```

### Gateway Serial
```
*** LoRa-to-Blues Gateway Online ***
Board: Adafruit Feather nRF52840

Packet received - processing
Valid sensor packet from node 1 (RSSI: -72 dBm)
Forwarding to Blues...
Blues send successful
```

### Blues Dashboard Data

In **events.qo**:
```json
{
  "node_id": 1,
  "value_1": 22.5,
  "value_2": 45.0,
  "battery_mv": 3850,
  "rssi": -72,
  "config_version": 1
}
```

---

## Board Comparison

| Board | Power Strategy | Current (sleep) | Best For |
|-------|--------------|-----------------|----------|
| **Feather M0 RFM9x** | Simple delay | ~15 mA | USB powered, testing |
| **nRF52840 + FeatherWing** | Deep sleep | ~6 µA | Battery/solar, field |
| **HelTec V3** | Light sleep | ~100 µA | OLED display, WiFi |
| **ESP32 Feather** | Light sleep | ~50 µA | WiFi, more RAM |

---

## File Structure

```
src/
├── common/
│   ├── pins.h              # Board pin definitions
│   ├── protocol.h/cpp      # Packet format
│   └── radio_config.h      # Radio settings
│
├── gateway/
│   └── gateway_main.cpp    # Gateway firmware
│
└── node/
    ├── node_main_feather_m0.cpp    # Feather M0 (simple)
    ├── node_main_nrf52840.cpp      # nRF52840 (deep sleep)
    ├── node_main_heltec_v4.cpp     # HelTec V3 (OLED)
    └── node_main_esp32_feather.cpp # ESP32 Feather
```

---

## Troubleshooting

### Node won't transmit
- Check battery (> 3.5V)
- Verify antenna connected
- Check serial for errors

### Gateway doesn't receive
- Same frequency on both boards
- Distance < 100m for testing
- RSSI should be -50 to -90 dBm

### Blues doesn't receive
- Check cellular signal in dashboard
- Verify Product UID

---

**Next:** See DESIGN.md for protocol details
