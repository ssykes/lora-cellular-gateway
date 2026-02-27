# LoRa Cellular Gateway

**Project:** LoRa Cellular Gateway
**Version:** 1.0
**Date:** February 25, 2026

A **distributed event-triggered network** for remote sensor deployments with cellular backhaul. Designed for artistic installations, environmental monitoring, and actuator control systems where events occur infrequently and latency tolerance is measured in seconds.

**Key Features:**
- 🎯 **Interrupt-driven gateway** - Wakes on LoRa packet (DIO0 interrupt)
- 🔋 **Ultra-low power** - ~6 µA sleep current, 6+ months battery
- ⚡ **MOSFET power control** - Notecard powered only when needed
- 🌐 **Global reach** - Cellular via Blues Notehub (Hawaii → Oregon)
- 🎨 **Artistic latency** - ~5 seconds (feature, not bug!)
- 📡 **Event-driven nodes** - Sleep until triggered (wind, motion, etc.)

---

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    DISTRIBUTED NETWORK                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  SENSOR FIELD (Remote Location)                                 │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                        │
│  │ Node 1  │  │ Node 2  │  │ Node 3  │  ... (1-10 nodes)     │
│  │ Event   │  │ Event   │  │ Event   │                        │
│  │ Driven  │  │ Driven  │  │ Driven  │                        │
│  └────┬────┘  └────┬────┘  └────┬────┘                        │
│       │           │           │                                │
│       └───────────┼───────────┘                                │
│                   ↓                                            │
│          ┌────────────────┐                                    │
│          │ Field Gateway  │                                    │
│          │ + Blues Card   │                                    │
│          │ (08:00-22:00)  │                                    │
│          └───────┬────────┘                                    │
│                  │                                             │
│  📡 Cellular     │ (2-5 seconds)                               │
│                  ↓                                             │
│          ┌────────────────┐                                    │
│          │  Blues Notehub │                                    │
│          │    (Cloud)     │                                    │
│          └───────┬────────┘                                    │
│                  │ (webhook, instant)                          │
│                  ↓                                             │
│  ACTUATOR SYSTEM (Anywhere)                                     │
│          ┌────────────────┐                                    │
│          │  Music Server  │                                    │
│          │  Light Controller │                                 │
│          │  Webhook Handler │                                  │
│          └────────────────┘                                    │
│                                                                 │
│  Total Latency: ~3-7 seconds (feature, not bug)                │
└─────────────────────────────────────────────────────────────────┘
```

---

## Key Design Principles

### 1. **Event-Driven, Not Periodic**

- **Nodes sleep** until triggered (wind, motion, button, sensor threshold)
- **Gateway listens** during active hours (08:00-22:00)
- **Latency tolerance:** Seconds are acceptable (artistic/remote deployment)

### 2. **Battery Longevity**

- **Nodes:** Months to years on battery + solar
- **Gateway:** Weeks on battery, indefinite with solar
- **Deep sleep** is the default state

### 3. **Remote Configuration (Infrequent)**

- **Blues Environment Variables** for runtime config
- **Gateway polls every 6 hours** (low overhead)
- **Nodes receive via LoRa broadcast** (when gateway config changes)
- **Dashboard-driven** (Blues Notehub UI)

### 4. **Actuator-Agnostic**

- Generic sensor data packets (not application-specific)
- Webhook forwards to any system (music, lights, motors, APIs)
- Configuration maps sensors to actuators

### 5. **Global Reach**

- Cellular backhaul (no WiFi required)
- Sensors in Hawaii → Sound in Oregon
- Blues Notehub handles global connectivity

---

## Hardware Architecture

### Gateway (Field Hub)

| Component | Model | Purpose |
|-----------|-------|---------|
| **MCU** | Adafruit Feather nRF52840 | Ultra-low-power processor |
| **LoRa Radio** | RFM95W (SX1276) 915 MHz | Sensor node communication |
| **Cellular** | Blues Note-WBNAN (LTE-M/NB-IoT) | Cloud backhaul |
| **Carrier** | Blues NoteCarrier F | Notecard interface (I2C) |
| **Power** | LiPo battery + solar panel | Field deployment |
| **Antenna** | LoRa whip antenna (915 MHz) | RF transmission |

**Power Consumption:**
- Active (listening): ~6 mA
- Deep sleep: ~6 µA
- Cellular TX: ~150 mA (brief bursts)

### Sensor Nodes (1-10x)

| Component | Model | Purpose |
|-----------|-------|---------|
| **MCU** | Adafruit Feather nRF52840 | Ultra-low-power processor |
| **LoRa Radio** | RFM95W (SX1276) 915 MHz | Gateway communication |
| **Sensors** | Application-specific | Wind, motion, vibration, etc. |
| **Power** | LiPo battery + solar charging | Remote deployment |
| **Antenna** | LoRa whip antenna (915 MHz) | RF transmission |

**Power Consumption:**
- Deep sleep: ~6 µA
- Wake + TX: ~100 mA (~50ms per event)

---

## Network Topology

### Star Network (Single Gateway)

```
        ┌──────────┐
        │ Gateway  │
        │ (Hub)    │
        └────┬─────┘
             │
    ┌────────┼────────┐
    │        │        │
┌───┴───┐ ┌──┴──┐ ┌──┴──┐
│Node 1 │ │Node 2│ │Node 3│
└───────┘ └─────┘ └─────┘
```

**Characteristics:**
- All nodes communicate directly with gateway
- No mesh routing (simpler, lower latency)
- Range: ~2-5 km line of sight (SF9, 17 dBm)

---

## Protocol Design

### Event Packet Structure (15 bytes)

```
┌────────────────────────────────────────────────────────┐
│                  SENSOR EVENT PACKET                   │
├──────────┬──────────┬──────────┬──────────┬───────────┤
│ Type     │ Node ID  │ Hop Count│ RSSI     │ Config Ver│
│ (1 byte) │ (1 byte) │ (1 byte) │ (1 byte) │ (1 byte)  │
├──────────┴──────────┴──────────┴──────────┴───────────┤
│                  Sensor Payload (11 bytes)             │
│  ┌────────────┬────────────┬────────────┬───────────┐ │
│  │ Value 1    │ Value 2    │ Battery mV │ Flags     │ │
│  │ (4 bytes)  │ (4 bytes)  │ (2 bytes)  │ (1 byte)  │ │
│  └────────────┴────────────┴────────────┴───────────┘ │
├────────────────────────────────────────────────────────┤
│                      CRC (2 bytes)                     │
└────────────────────────────────────────────────────────┘
```

**Generic Payload:**
- `Value 1`: Primary sensor (wind speed, vibration, temperature, etc.)
- `Value 2`: Secondary sensor (direction, magnitude, humidity, etc.)
- `Battery mV`: Node battery level
- `Flags`: Status bits (low battery, error, relay mode)

### Config Update Packet (11 bytes)

```
┌──────────┬──────────┬──────────┬──────────┬───────────┐
│ Type     │ Node ID  │ Version  │ Interval │ Threshold │
│ (1 byte) │ (1 byte) │ (1 byte) │ (1 byte) │ (1 byte)  │
├──────────┴──────────┴──────────┴──────────┴───────────┤
│                      CRC (2 bytes)                     │
└────────────────────────────────────────────────────────┘
```

---

## Gateway Behavior

### Listen Window (08:00-22:00)

```cpp
void loop() {
  if (is_listen_window()) {  // 08:00-22:00
    
    // Listen for LoRa events (continuous RX)
    if (packet_received) {
      forward_to_blues();  // ~50ms
    }
    
    // Check Blues Environment Variables (every 6 hours)
    if (millis() - last_env_sync > 6 * 3600 * 1000) {
      check_environment_variables();  // ~2-5 sec
      last_env_sync = millis();
    }
    
  } else {  // 22:00-08:00
    
    // Deep sleep (quiet hours)
    enter_deep_sleep_until(8, 0);  // Wake at 08:00
    
  }
}
```

**Power Budget (24 hours):**
| State | Current | Duration | mAh/day |
|-------|---------|----------|---------|
| Deep sleep | 6 µA | 14 hrs (22:00-08:00) | 0.08 |
| Active listen | 6 mA | 10 hrs (08:00-22:00) | 60 |
| Cellular TX | 150 mA | ~30 sec total | 1.25 |
| Env var sync | 25 mA | ~5 min (4x/day) | 8.3 |
| **Total** | - | - | **~70 mAh/day** |

**Battery life:** ~28 days on 2000 mAh (indefinite with solar)

---

## Node Behavior

### Event-Driven Sleep

```cpp
void setup() {
  // Initialize sensor
  attach_interrupt(SENSOR_PIN, on_event, RISING);
  
  // Initialize LoRa
  init_radio();
  
  // Load config from Flash
  load_node_config();
  
  // First reading, then sleep
  check_sensor();
  
  // Deep sleep until interrupt
  enter_deep_sleep();
}

void loop() {
  // Never reached (event-driven)
}

// Hardware interrupt wakes the node
void IRAM_ATTR on_event() {
  // Wake up (instant, hardware-driven)
  
  // Read sensor
  float value = read_sensor();
  
  // Build and send packet
  send_event_packet(value);
  
  // Return to deep sleep
  enter_deep_sleep();
}
```

**Power Budget (per event):**
| State | Current | Duration | mAh/event |
|-------|---------|----------|-----------|
| Deep sleep | 6 µA | Between events | Base drain |
| Wake + TX | 100 mA | ~50ms | 0.0014 |

**Battery life (50 events/day):** ~15 mAh/day → ~130 days on 2000 mAh

---

## Configuration Management

### Blues Environment Variables

**Set via Blues Dashboard:**
```
Variable                    Value           Description
─────────────────────────────────────────────────────────────────
gateway_listen_start        8               Listen window start
gateway_listen_end          22              Listen window end
gateway_sync_interval       360             Env var sync (minutes)
node_tx_interval            30              Node TX interval
node_threshold              50              Trigger threshold
config_version              5               Increment on change
```

**Gateway reads every 6 hours:**
```cpp
void check_environment_variables() {
  rak_blues.start_req("hub.get");
  rak_blues.add_bool_entry("env", true);
  
  char response[2048];
  if (rak_blues.send_req(response, sizeof(response))) {
    // Parse JSON, apply new config
    // Broadcast to nodes if config changed
  }
}
```

**Node config (broadcast from gateway):**
- Gateway detects config version change
- Broadcasts new config to all nodes via LoRa
- Nodes save to Flash, apply immediately

---

## Blues Notehub Integration

### Webhook Forwarding

**Gateway sends to `events.qo`:**
```json
{
  "file": "events.qo",
  "body": {
    "node_id": 3,
    "value_1": 45.2,
    "value_2": 78.1,
    "battery_mv": 3850,
    "rssi": -72,
    "config_version": 5
  },
  "payload": "base64_encoded_packet"
}
```

**Webhook → Your Actuator System:**
```
Blues Notehub → Webhook → Your Server
                         ↓
                   Music Server
                   Light Controller
                   Motor Driver
                   API Endpoint
```

### Firmware OTA

**Blues handles automatically:**
1. Upload `.bin` to Notehub Dashboard
2. Target device(s)
3. Notecard downloads via cellular
4. Gateway/node updates firmware

**No code changes needed** - Blues infrastructure.

---

## Latency Budget

| Stage | Time | Notes |
|-------|------|-------|
| **Event → Node detect** | <10ms | Hardware interrupt |
| **Node LoRa TX (SF9)** | ~30ms | Airtime |
| **Gateway RX → Blues** | ~50ms | One-shot forward |
| **Cellular → Notehub** | 2-5 sec | Unavoidable |
| **Webhook → Actuator** | <100ms | Your system |
| **Actuator response** | <100ms | Sound/light/motor |
| **TOTAL** | **~3-7 seconds** | **Feature, not bug!** |

---

## Deployment Scenarios

### Artistic Installation (Wind Chime)

```
Hawaii (Sensor Field)          Oregon (Sound Installation)
┌──────────────────┐           ┌──────────────────┐
│ 10x Wind Sensors │  ────→    │  Music Server    │
│ LoRa → Gateway   │  Cellular │  + Speakers      │
│ Blues Notecard   │  Notehub  │                  │
└──────────────────┘           └──────────────────┘

Latency: ~5 seconds (like thunder after lightning)
```

### Environmental Monitoring

```
Remote Forest                   Research Station
┌──────────────────┐           ┌──────────────────┐
│ Motion Sensors   │  ────→    │  Dashboard       │
│ Temperature      │  Cellular │  + Alerts        │
│ Gateway + Blues  │  Notehub  │                  │
└──────────────────┘           └──────────────────┘

Battery: 6+ months (solar charging)
```

### Distributed Instrument

```
Stage (Sensor Controllers)      Sound Booth
┌──────────────────┐           ┌──────────────────┐
│ Performer Nodes  │  ────→    │  DAW / Synth     │
│ Gesture Sensors  │  Cellular │  + Effects       │
│ Gateway + Blues  │  Notehub  │                  │
└──────────────────┘           └──────────────────┘

Latency: ~5 seconds (artistic delay)
```

---

## Files and Structure

```
hummingbird-blues-gateway/
├── src/
│   ├── common/               # Shared code (gateway + nodes)
│   │   ├── protocol.h        # Packet definitions
│   │   ├── protocol.cpp      # CRC, packet builders
│   │   ├── radio_config.h    # LoRa settings (SF9, BW125)
│   │   └── config_types.h    # Config structures (Flash)
│   ├── gateway/              # Gateway-specific code
│   │   └── gateway_main.cpp  # Main loop, Blues integration
│   └── node/                 # Node-specific code (TODO)
│       └── node_main.cpp     # Event-driven sleep
├── platformio.ini            # Build environments
├── README.md                 # This file
├── DESIGN.md                 # Detailed design document
├── QUICK_REFERENCE.md        # Build commands, pin mappings
└── IMPLEMENTATION_STATUS.md  # Progress tracking
```

---

## Build Environments

| Environment | Board | Purpose |
|-------------|-------|---------|
| `gateway_nrf52840` | Adafruit Feather nRF52840 | Gateway (default) |
| `gateway_rak4630` | RAK4630 | Alternative gateway |
| `node_nrf52840` | Adafruit Feather nRF52840 | Sensor node |
| `node_heltec` | HelTec WiFi LoRa 32 V5 | Alternative node |

**Build commands:**
```bash
# Build gateway
pio run -e gateway_nrf52840

# Build node
pio run -e node_nrf52840

# Upload gateway
pio run -e gateway_nrf52840 -t upload

# Monitor serial
pio device monitor -e gateway_nrf52840
```

---

## Success Criteria

- ✅ **Event-driven operation** (nodes sleep until triggered)
- ✅ **Gateway sleep schedule** (08:00-22:00 listen window)
- ✅ **Remote configuration** (Blues Environment Variables)
- ✅ **Actuator-agnostic** (generic sensor packets)
- ✅ **Global reach** (cellular backhaul via Blues)
- ✅ **Battery longevity** (months/years with solar)
- ✅ **Latency tolerance** (~5 seconds acceptable)

---

## License

Open-source hardware and software for artistic installations and remote sensing applications.

---

**End of README**
