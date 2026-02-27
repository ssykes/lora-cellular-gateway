# Documentation Index

**Project:** LoRa Cellular Gateway
**Last Updated:** February 24, 2026

---

## Quick Start (New to Project)

1. **Read this first:** `README.md` (5 min)
2. **Quick reference:** `QUICK_REFERENCE.md` (build commands, settings)
3. **Current status:** `IMPLEMENTATION_STATUS.md` (what's done, what's next)

---

## If Session Is Lost

1. **Read first:** `PROJECT_CONTEXT.md` (critical for resuming)
2. **Then review:** `SESSION_SUMMARY.md` (what we accomplished)
3. **Check progress:** `IMPLEMENTATION_STATUS.md` (where we left off)

---

## Complete File List

### Root Directory

| File | Purpose | When to Read |
|------|---------|--------------|
| [`README.md`](README.md) | Project overview, quick start | First time setup |
| [`QUICK_REFERENCE.md`](QUICK_REFERENCE.md) | Build commands, pin mappings, settings | Daily development |
| [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md) | Progress tracking, task list | Check what's next |
| [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md) | Session recovery, critical context | **If session lost** |
| [`SESSION_SUMMARY.md`](SESSION_SUMMARY.md) | What we accomplished this session | After design phase |
| [`DESIGN.md`](DESIGN.md) | Complete system design | Deep dive, reference |
| `INDEX.md` | This file | Navigation |

### Source Code - Common Headers

| File | Purpose | When to Read |
|------|---------|--------------|
| [`src/common/protocol.h`](src/common/protocol.h) | Packet structures, CRC, protocol functions | Implementing communication |
| [`src/common/radio_config.h`](src/common/radio_config.h) | LoRa settings, pin mappings, timing | Configuring radio |
| [`src/common/config_types.h`](src/common/config_types.h) | Config structures, Flash storage | Implementing config |

### Source Code - Gateway (To Be Created)

| File | Purpose | Status |
|------|---------|--------|
| `src/gateway/gateway_main.cpp` | Main loop, initialization | ⚪ Pending |
| `src/gateway/gateway_radio.cpp` | LoRa RX handling | ⚪ Pending |
| `src/gateway/gateway_blues.cpp` | Blues integration | ⚪ Pending |
| `src/gateway/gateway_scheduler.cpp` | Time window scheduling | ⚪ Pending |
| `src/gateway/gateway_config.cpp` | Config management | ⚪ Pending |

### Source Code - Node (To Be Created)

| File | Purpose | Status |
|------|---------|--------|
| `src/node/node_main.cpp` | Main loop, sensor reading | ⚪ Pending |
| `src/node/node_radio.cpp` | LoRa TX handling | ⚪ Pending |
| `src/node/node_sensor.cpp` | Sensor drivers | ⚪ Pending |
| `src/node/node_config.cpp` | Config handling | ⚪ Pending |

### Build Configuration

| File | Purpose | Status |
|------|---------|--------|
| [`platformio.ini`](platformio.ini) | Build environments, dependencies | ✅ Updated |

---

## Documentation by Topic

### System Design

- [`DESIGN.md`](DESIGN.md) - Complete system architecture
- [`DESIGN.md#system-overview`](DESIGN.md#system-overview) - Block diagram
- [`DESIGN.md#hardware-architecture`](DESIGN.md#hardware-architecture) - Hardware specs
- [`DESIGN.md#key-design-decisions`](DESIGN.md#key-design-decisions) - Why we chose this approach

### Protocol

- [`DESIGN.md#protocol-design`](DESIGN.md#protocol-design) - Packet structure overview
- [`src/common/protocol.h`](src/common/protocol.h) - Actual packet definitions
- [`DESIGN.md#packet-structure`](DESIGN.md#packet-structure) - Detailed field descriptions

### Configuration

- [`DESIGN.md#configuration-management`](DESIGN.md#configuration-management) - Config flow
- [`DESIGN.md#blues-notehub-integration`](DESIGN.md#blues-notehub-integration) - Blues setup
- [`src/common/config_types.h`](src/common/config_types.h) - Config structures

### Radio Settings

- [`DESIGN.md#loRa-radio-settings`](DESIGN.md#loRa-radio-settings) - SF, BW, power
- [`src/common/radio_config.h`](src/common/radio_config.h) - Actual settings
- [`QUICK_REFERENCE.md#radio-settings`](QUICK_REFERENCE.md#radio-settings) - Quick reference

### Implementation

- [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md) - What's done, what's next
- [`IMPLEMENTATION_STATUS.md#detailed-task-list`](IMPLEMENTATION_STATUS.md#detailed-task-list) - Task breakdown
- [`QUICK_REFERENCE.md#build-commands`](QUICK_REFERENCE.md#build-commands) - PlatformIO commands

### Hardware

- [`DESIGN.md#hardware-architecture`](DESIGN.md#hardware-architecture) - Hardware specs
- [`DESIGN.md#appendix-a-pin-mappings`](DESIGN.md#appendix-a-pin-mappings) - Pin connections
- [`QUICK_REFERENCE.md#pin-mappings`](QUICK_REFERENCE.md#pin-mappings) - Quick pin reference

### Testing

- [`IMPLEMENTATION_STATUS.md#testing-checklist`](IMPLEMENTATION_STATUS.md#testing-checklist) - Test plan
- [`DESIGN.md#performance-estimates`](DESIGN.md#performance-estimates) - Expected performance
- [`DESIGN.md#troubleshooting`](DESIGN.md#troubleshooting) - Common issues

---

## Reading Paths

### Path 1: First Time Setup (15 min)

1. `README.md` (5 min)
2. `QUICK_REFERENCE.md` (5 min)
3. `IMPLEMENTATION_STATUS.md` (5 min)

### Path 2: Deep Dive (60 min)

1. `README.md` (5 min)
2. `DESIGN.md` (30 min)
3. `src/common/protocol.h` (10 min)
4. `src/common/radio_config.h` (10 min)
5. `src/common/config_types.h` (5 min)

### Path 3: Session Recovery (10 min)

1. `PROJECT_CONTEXT.md` (5 min)
2. `SESSION_SUMMARY.md` (3 min)
3. `IMPLEMENTATION_STATUS.md` (2 min)

### Path 4: Daily Development (5 min)

1. `QUICK_REFERENCE.md` (build commands)
2. `IMPLEMENTATION_STATUS.md` (what's next)

---

## Key Concepts (Quick Lookup)

| Concept | Description | File |
|---------|-------------|------|
| **Silent ACK** | No response for normal data (saves airtime) | `DESIGN.md#protocol-design` |
| **Config Version** | 1-byte version tracking (update only when changed) | `src/common/protocol.h` |
| **Time Window** | Gateway listens 08:00-22:00 (configurable) | `DESIGN.md#gateway-behavior` |
| **Store-and-Forward Relay** | Simple relay (not full mesh) | `DESIGN.md#network-topology` |
| **Blues Mailbox** | Cloud → Device config queue | `DESIGN.md#blues-notehub-integration` |

---

## Build Environments

| Environment | Hardware | Command |
|-------------|----------|---------|
| `gateway_feather_m0` | Adafruit Feather M0 | `pio run -e gateway_feather_m0` |
| `gateway_rak4630` | RAK4630 (alternative) | `pio run -e gateway_rak4630` |
| `node_nrf52840` | Adafruit nRF52840 | `pio run -e node_nrf52840` |
| `node_heltec` | HelTec WiFi LoRa 32 V5 | `pio run -e node_heltec` |

See: [`platformio.ini`](platformio.ini)

---

## External Resources

| Resource | URL |
|----------|-----|
| Blues Notehub | https://notehub.io |
| Blues Documentation | https://dev.blues.io |
| RadioLib | https://github.com/jgromes/RadioLib |
| PlatformIO | https://platformio.org |
| LoRa Airtime Calculator | https://rfcalculator.nl/lora-airtime-and-range/ |

---

## Contact / Support

| Topic | Where to Look |
|-------|---------------|
| Build errors | `QUICK_REFERENCE.md#build-commands` |
| Radio not working | `DESIGN.md#troubleshooting` |
| Config not applied | `DESIGN.md#configuration-management` |
| Short range | `DESIGN.md#performance-estimates` |
| Blues not connecting | `DESIGN.md#blues-notehub-integration` |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.1.0 | 2026-02-24 | Design phase complete, foundation files created |

---

**End of Documentation Index**
