# RAK4630 Gateway - Build & Upload Guide

## Hardware Required

- **RAK4630** or **RAK4631** WisBlock Core
- **RAK19003** or **RAK19007** WisBlock Base Board (for USB)
- **Blues Notecard** (NOTE-WBNAN or NOTE-NB-IoT)
- **LoRa Antenna** (915 MHz)

## Setup

### 1. Install RAK4630 Board Support

The RAK4630 board files are already included in this project in the `rakwireless/` folder.

### 2. Connect Hardware

1. **Mount RAK4630** on WisBlock base board
2. **Insert Notecard** into Notecarrier-F, connect to RAK4630:
   - **SDA** → **D20/A4** (P0.30)
   - **SCL** → **D21/A5** (P0.31)
   - **VCC** → **3.3V** (or switched via MOSFET)
   - **GND** → **GND**
3. **Connect LoRa antenna** to RAK4630 (u.FL connector)
4. **Connect USB** to base board

### 3. Build Gateway

```bash
# Build for RAK4630
pio run -e gateway_rak4630
```

### 4. Upload Gateway

```bash
# Upload (RAK4630 should be in bootloader mode)
pio run -e gateway_rak4630 -t upload

# Or specify COM port manually
pio run -e gateway_rak4630 -t upload --upload-port COMxx
```

### 5. Enter Bootloader Mode (if needed)

If upload fails, put RAK4630 in bootloader mode:

1. **Disconnect USB**
2. **Hold the boot button** (or connect BOOT pin to GND)
3. **Reconnect USB**
4. **Release boot button**
5. A drive named **ARDUINO** should appear
6. Try upload again

### 6. Monitor Serial

```bash
# Open serial monitor at 115200 baud
pio device monitor -e gateway_rak4630 -b 115200

# Or use Python directly
python -m serial.tools.miniterm COMxx 115200
```

## Expected Output

```
*** LoRa-to-Blues Gateway Online ***
Board: RAK4630
Interrupt-driven gateway

Powering on Notecard...
Initializing Blues Notecard...
Blues Notecard found
Blues Notecard initialized

Checking environment variables...
Gateway config loaded (version 1, frequency 915.0 MHz)

Radio initialized successfully
  Frequency: 915.0 MHz
  Spreading Factor: 10
  Bandwidth: 125 kHz
  TX Power: 14 dBm

Powering off Notecard (sleep mode)
*** Gateway ready - sleeping until packet ***
```

## When Node Transmits

```
Packet received - processing
Valid sensor packet from node 1 (RSSI: -48 dBm)
Forwarding to Blues...
Blues send successful
```

## Check Blues Notehub

1. Go to https://notehub.io
2. Select your project
3. Look for `events.qo` files
4. Should contain sensor data from node

## Troubleshooting

### Upload Fails

- **Double-tap reset** or use boot button to enter bootloader
- Check COM port in Device Manager
- Try different USB cable

### Notecard Not Found

- Check I2C wiring (SDA/SCL)
- Verify Notecard is seated properly in carrier
- Check power (3.3V to Notecard)

### No Packets Received

- Verify node is transmitting (check node Serial)
- Check antenna connections
- Verify both boards use same frequency (915 MHz)
- Check sync word (0x12 on both)

### Blues Send Fails

- Check Notecard has cellular signal
- Verify product ID is set correctly
- Check Notecard power (no brown-outs)

## Project Structure

```
src/
├── common/           # Shared code (protocol, config types)
│   ├── protocol.cpp
│   ├── protocol.h
│   └── config_types.h
├── gateway/          # Gateway code
│   └── gateway_main.cpp
└── node/             # Node code (Feather M0, etc.)
    └── node_main_feather_m0.cpp
```

## Build Environments

| Environment | Board | Purpose |
|-------------|-------|---------|
| `gateway_rak4630` | RAK4630 | Gateway with Blues (recommended) |
| `gateway_nrf52840` | Feather nRF52840 | Gateway (I2C issues) |
| `node_feather_m0` | Feather M0 RFM9x | Sensor node (working!) |
| `node_nrf52840` | Feather nRF52840 | Alternative node |

## Power Consumption

**RAK4630 Gateway:**
- Active (listening): ~5 mA
- Deep sleep: ~6 µA
- Cellular TX: ~150 mA (brief)

**Battery Life:** ~2-4 weeks on 2000 mAh (with solar: indefinite)

---

**Ready to deploy!** 🚀
