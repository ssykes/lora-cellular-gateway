# PlatformIO Build Commands Reference

**Project:** LoRa Cellular Gateway  
**Date:** February 27, 2026

---

## Quick Start

### Build and Upload Node
```bash
pio run -e node_feather_m0 -t upload
pio device monitor -e node_feather_m0 -b 115200
```

### Build and Upload Gateway
```bash
pio run -e gateway_nrf52840 -t upload
pio device monitor -e gateway_nrf52840 -b 115200
```

---

## Build Commands

### Compile (Build) Only

```bash
# Build specific environment
pio run -e node_feather_m0

# Build all environments
pio run

# Build with verbose output
pio run -e node_nrf52840 -v

# Clean build and rebuild
pio run -e node_feather_m0 -t clean
pio run -e node_feather_m0
```

---

### Upload (Compile + Flash)

```bash
# Build and upload to specific board
pio run -e node_feather_m0 -t upload

# Upload without recompiling (if already built)
pio run -e node_nrf52840 -t program

# Force rebuild and upload
pio run -e node_heltec_v4 -t clean -t upload
```

---

### Serial Monitor

```bash
# Open serial monitor (uses monitor_speed from platformio.ini)
pio device monitor -e node_feather_m0

# Open serial monitor with specific baud rate
pio device monitor -e node_nrf52840 -b 115200

# List available serial ports
pio device list
```

**Exit monitor:** Press `Ctrl+C`, then type `exit`

---

## Board-Specific Commands

### Feather M0 RFM9x (Node)
```bash
# Build and upload
pio run -e node_feather_m0 -t upload

# Monitor
pio device monitor -e node_feather_m0 -b 115200
```

**⚠️ Bootloader Mode:** Double-press the reset button (red LED pulses) before upload

---

### nRF52840 + FeatherWing (Node)
```bash
# Build and upload
pio run -e node_nrf52840 -t upload

# Monitor
pio device monitor -e node_nrf52840 -b 115200
```

**Note:** Board should auto-reset into bootloader

---

### HelTec WiFi LoRa 32 V3 (Node)
```bash
# Build and upload
pio run -e node_heltec_v4 -t upload

# Monitor
pio device monitor -e node_heltec_v4 -b 115200
```

**⚠️ If upload fails:** Hold BOOT button while connecting USB, then release

---

### Adafruit ESP32 Feather V2 (Node)
```bash
# Build and upload
pio run -e node_esp32_feather -t upload

# Monitor
pio device monitor -e node_esp32_feather -b 115200
```

**⚠️ If upload fails:** Hold BOOT button while connecting USB, then release

---

### Gateway (nRF52840)
```bash
# Build and upload
pio run -e gateway_nrf52840 -t upload

# Monitor
pio device monitor -e gateway_nrf52840 -b 115200
```

---

## One-Liner Workflows

### Build + Upload + Monitor
```bash
# Node: Build, upload, then open monitor
pio run -e node_feather_m0 -t upload && pio device monitor -e node_feather_m0 -b 115200

# Gateway: Build, upload, then open monitor
pio run -e gateway_nrf52840 -t upload && pio device monitor -e gateway_nrf52840 -b 115200
```

### Test All Nodes
```bash
# Build all node environments
pio run -e node_feather_m0 -e node_nrf52840 -e node_heltec_v4 -e node_esp32_feather
```

---

## Other Useful Commands

```bash
# Show project information
pio project info

# List all configured environments
pio run --list-targets

# Check for library updates
pio lib outdated

# Update libraries
pio lib update

# Clean all builds
pio run -t clean

# Show memory usage
pio run -e node_feather_m0 --target inspect
```

---

## Environment Reference

| Environment | Board | Upload Speed | Monitor Speed |
|-------------|-------|--------------|---------------|
| `node_feather_m0` | Adafruit Feather M0 RFM9x | 115200 | 115200 |
| `node_nrf52840` | Adafruit nRF52840 Express | 115200 | 115200 |
| `node_heltec_v4` | HelTec WiFi LoRa 32 V3 | 921600 | 115200 |
| `node_esp32_feather` | Adafruit ESP32 Feather V2 | 921600 | 115200 |
| `gateway_nrf52840` | Adafruit nRF52840 Gateway | 115200 | 115200 |
| `gateway_feather_m0` | Adafruit Feather M0 Gateway | 115200 | 115200 |
| `gateway_rak4630` | RAK4630 Gateway | 115200 | 115200 |

---

## Troubleshooting

### Upload Fails - Device Not Found
```bash
# Check if device is connected
pio device list

# Specify upload port explicitly
pio run -e node_feather_m0 -t upload --upload-port COM3
```

### Serial Monitor Shows Garbage
```bash
# Check baud rate matches
pio device monitor -e node_feather_m0 -b 115200
```

### Build Fails - Stale Build Files
```bash
# Clean and rebuild
pio run -e node_feather_m0 -t clean
pio run -e node_feather_m0
```

### ESP32 Upload Fails
```bash
# Hold BOOT button while connecting USB
# Release BOOT button after connection
pio run -e node_heltec_v4 -t upload
```

### Feather M0 Upload Fails
```bash
# Double-press reset button (red LED pulses)
# Board enters bootloader mode
pio run -e node_feather_m0 -t upload
```

---

## See Also

- [PlatformIO CLI Documentation](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html)
- [Hello World Test Guide](HELLO_WORLD_TEST.md)
- [Design Document](DESIGN.md)
