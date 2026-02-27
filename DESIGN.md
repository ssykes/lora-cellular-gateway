# Design Document: LoRa Cellular Gateway

**Project:** LoRa Cellular Gateway
**Version:** 1.0
**Date:** February 25, 2026

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Hardware Architecture](#hardware-architecture)
3. [Key Design Decisions](#key-design-decisions)
4. [Network Topology](#network-topology)
5. [Protocol Design](#protocol-design)
6. [Gateway Behavior](#gateway-behavior)
7. [Node Behavior](#node-behavior)
8. [Configuration Management](#configuration-management)
9. [Blues Notehub Integration](#blues-notehub-integration)
10. [Power Budget](#power-budget)
11. [Latency Analysis](#latency-analysis)

---

## System Overview

This project implements a **distributed event-triggered sensor network** with cellular backhaul via Blues Notecard. The system is designed for artistic installations, environmental monitoring, and remote actuator control where:

- **Events occur infrequently** (wind, motion, button presses, sensor thresholds)
- **Latency tolerance is seconds** (not milliseconds)
- **Battery longevity is critical** (remote deployments, solar charging)
- **Global reach required** (sensors in one location, actuators elsewhere)

### Primary Goals

- **Event-driven operation** - Nodes sleep until triggered
- **Battery efficiency** - Months to years on battery + solar
- **Remote configuration** - Blues Environment Variables (infrequent updates)
- **Actuator-agnostic** - Generic sensor data, webhook-driven outputs
- **Artistic latency** - ~5 second delay is a feature, not a bug

### Communication Flow

```
Sensor Nodes (Event-Driven)
     │
     │ LoRa P2P (915 MHz, SF9)
     │ Event-triggered TX
     ▼
Field Gateway (08:00-22:00 Active)
     │
     │ Cellular (LTE-M/NB-IoT)
     │ Blues Notecard
     │ ~2-5 seconds
     ▼
Blues Cloud (Notehub.io)
     │
     │ Webhooks (instant)
     ▼
Your Actuator System
     │
     │ Music, Lights, Motors, APIs
     ▼
Sound / Light / Motion / etc.
```

---

## Hardware Architecture

### Gateway Hardware

| Component | Model | Purpose |
|-----------|-------|---------|
| **MCU** | Adafruit Feather nRF52840 | Ultra-low-power processor |
| **LoRa Radio** | RFM95W (SX1276) 915 MHz | Sensor node communication |
| **Cellular** | Blues Note-WBNAN (LTE-M/NB-IoT) | Cloud backhaul |
| **Carrier** | Blues NoteCarrier F | Notecard interface (I2C) |
| **Power** | LiPo battery (2000 mAh) + solar panel | Field deployment |
| **Antenna** | LoRa whip antenna (915 MHz) | RF transmission |

**Pin Mappings (nRF52840 + RFM95W):**
```cpp
// RFM95W LoRa radio (SPI)
#define RADIO_CS_PIN      10        // P0.06
#define RADIO_RST_PIN     8         // P0.08
#define RADIO_DIO0_PIN    4         // P0.04 (RX/TX interrupt)
#define RADIO_DIO1_PIN    7         // P0.27 (optional)

// Blues Notecard (I2C)
#define NOTECARD_SDA      PIN_WIRE_SDA  // D20/A4
#define NOTECARD_SCL      PIN_WIRE_SCL  // D21/A5
```

### Node Hardware

| Component | Model | Purpose |
|-----------|-------|---------|
| **MCU** | Adafruit Feather nRF52840 | Ultra-low-power processor |
| **LoRa Radio** | RFM95W (SX1276) 915 MHz | Gateway communication |
| **Sensors** | Application-specific | Wind, motion, vibration, etc. |
| **Power** | LiPo battery + solar charging | Remote deployment |
| **Antenna** | LoRa whip antenna (915 MHz) | RF transmission |

**Sensor Interfaces:**
- **Analog:** `A0-A5` (wind speed, vibration magnitude, etc.)
- **Digital:** `D2-D13` (button, PIR motion, reed switch)
- **I2C:** `SDA/SCL` (environmental sensors, accelerometers)

---

## Key Design Decisions

### 1. Event-Driven vs. Periodic

**Decision:** Event-driven (nodes sleep until triggered)

**Rationale:**
- Battery longevity (months/years vs. days/weeks)
- Natural fit for wind chime / artistic installations
- Latency tolerance allows deep sleep between events

**Trade-offs:**
- ❌ No heartbeat (can't detect offline nodes easily)
- ✅ Years of battery life
- ✅ Natural rhythm (events arrive when they happen)

### 2. Gateway Sleep Schedule

**Decision:** 08:00-22:00 active, 22:00-08:00 deep sleep

**Rationale:**
- Artistic choice (quiet hours for sound installations)
- Battery conservation (14 hours sleep = 70% power savings)
- Configurable via Blues Environment Variables

**Trade-offs:**
- ❌ 14 hours offline daily
- ✅ 3x longer battery life
- ✅ Respectful of neighbors / environment

### 3. Blues Notehub (Not Direct MQTT)

**Decision:** Use Blues Notehub (not direct MQTT)

**Rationale:**
- Global reach (Hawaii → Oregon)
- No infrastructure required (cellular everywhere)
- Built-in firmware OTA
- Dashboard-driven configuration
- Free tier sufficient (500 notes/day)

**Trade-offs:**
- ❌ 2-5 second latency (cellular)
- ✅ No cellular infrastructure to manage
- ✅ Remote monitoring + alerts

### 4. Environment Variables (Not Mailbox Polling)

**Decision:** Blues Environment Variables (poll every 6 hours)

**Rationale:**
- Low overhead (4 checks/day vs. 96/day)
- Dashboard-driven (Blues UI)
- Infrequent config changes (once optimized)
- Version tracking (detect changes automatically)

**Trade-offs:**
- ❌ Up to 6-hour config propagation
- ✅ 96% reduction in polling overhead
- ✅ Simple workflow (edit dashboard → save → deploy)

### 5. LoRa Settings (SF9, BW125)

**Decision:** SF9, BW125, 17 dBm

**Rationale:**
- Balance range (2-5 km) vs. airtime (~30ms)
- SF7 too fast but short range (~500m)
- SF10 too slow (~100ms) for marginal range gain
- 17 dBm max power (field deployment)

**Trade-offs:**
- ⚖️ Compromise between speed and range
- ✅ 30ms airtime acceptable (latency dominated by cellular)
- ✅ 2-5 km range sufficient for field deployments

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
- No mesh routing (simpler, lower latency, less battery drain)
- Range: ~2-5 km line of sight (SF9, 17 dBm)
- Supports 1-10 nodes (scalable via node ID)

**Why not mesh?**
- Mesh adds complexity (routing tables, discovery)
- Mesh adds latency (multiple hops)
- Mesh drains battery (relay nodes always listening)
- Star topology sufficient for field deployments (~2 km range)

---

## Protocol Design

### Packet Type Definitions

```cpp
#define PKT_TYPE_SENSOR_DATA    0x01    // Node → Gateway (event)
#define PKT_TYPE_CONFIG_UPDATE  0x02    // Gateway → Node (config)
#define PKT_TYPE_ACK            0x03    // Optional acknowledgment
#define PKT_TYPE_BROADCAST      0x00    // Gateway → All nodes
```

### Sensor Event Packet (15 bytes)

```cpp
typedef struct {
    uint8_t type;             // PKT_TYPE_SENSOR_DATA (0x01)
    uint8_t node_id;          // Source node ID (1-255)
    uint8_t hop_count;        // Number of relays (0 = direct)
    uint8_t rssi;             // RSSI in dBm (added by gateway)
    uint8_t config_version;   // Node's current config version
    float value_1;            // Primary sensor value
    float value_2;            // Secondary sensor value
    uint16_t battery_mv;      // Battery voltage in mV
    uint8_t sensor_flags;     // Bit flags for status
    uint16_t crc;             // 16-bit CRC
} __attribute__((packed)) sensor_packet_t;
```

**Generic Payload Interpretation:**
| Application | `value_1` | `value_2` |
|-------------|-----------|-----------|
| Wind chime | Wind speed | Vibration magnitude |
| Environmental | Temperature (°C) | Humidity (%) |
| Motion detection | Motion intensity | Direction (0-360) |
| Musical instrument | Pitch (MIDI note) | Velocity (0-127) |
| Button/switch | Button ID | Press duration |

### Config Update Packet (11 bytes)

```cpp
typedef struct {
    uint8_t type;             // PKT_TYPE_CONFIG_UPDATE (0x02)
    uint8_t node_id;          // Target node ID (0 = broadcast)
    uint8_t config_version;   // New config version
    uint8_t tx_interval;      // TX interval (if periodic)
    uint8_t threshold;        // Trigger threshold
    uint8_t tx_power_dbm;     // TX power (2-17)
    uint16_t crc;             // 16-bit CRC
} __attribute__((packed)) config_packet_t;
```

### CRC-16-CCITT

- Polynomial: `0x1021`
- Initial value: `0xFFFF`
- Covers all packet bytes except CRC field

---

## Gateway Behavior

### State Machine

```
┌─────────────────────────────────────────────────────────┐
│                  GATEWAY STATE MACHINE                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐                                        │
│  │ BOOT        │                                        │
│  │ - Init HW   │                                        │
│  │ - Check Env │                                        │
│  └──────┬──────┘                                        │
│         │                                               │
│         ▼                                               │
│  ┌─────────────┐     22:00      ┌─────────────┐        │
│  │ ACTIVE      │───────────────→│ SLEEP       │        │
│  │ (08:00)     │                │ (22:00-08:00│        │
│  │             │←───────────────│             │        │
│  │ - Listen    │     08:00      │ - Deep sleep│        │
│  │ - Forward   │                │ - RTC alarm │        │
│  │ - Env sync  │                └─────────────┘        │
│  └─────────────┘                                        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Active Mode (08:00-22:00)

```cpp
void loop() {
  // Continuous LoRa RX (duty-cycled internally by radio)
  if (packet_received) {
    if (verify_crc()) {
      forward_to_blues();  // Immediate forward (~50ms)
    }
  }
  
  // Environment variable sync (every 6 hours)
  if (millis() - last_env_sync > 6 * 3600 * 1000) {
    check_environment_variables();  // ~2-5 sec
    last_env_sync = millis();
  }
  
  // Optional: Heartbeat to Notehub (every 30 min)
  if (millis() - last_heartbeat > 30 * 60 * 1000) {
    send_heartbeat();
    last_heartbeat = millis();
  }
}
```

### Sleep Mode (22:00-08:00)

```cpp
void enter_deep_sleep_until(uint8_t wake_hour) {
  // Turn off peripherals
  radio.sleep();
  digitalWrite(LED, LOW);
  
  // Set RTC alarm for wake hour
  rtc_set_alarm(wake_hour, 0, 0);
  
  // Enter deep sleep (~6 µA)
  sd_app_evt_wait();  // nRF52840 deep sleep
  
  // Wake via RTC alarm (or external interrupt)
  // Reinitialize and return to loop()
}
```

---

## Node Behavior

### Event-Driven State Machine

```
┌─────────────────────────────────────────────────────────┐
│                    NODE STATE MACHINE                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐                                        │
│  │ BOOT        │                                        │
│  │ - Init HW   │                                        │
│  │ - Load Cfg  │                                        │
│  └──────┬──────┘                                        │
│         │                                               │
│         ▼                                               │
│  ┌─────────────┐     Event       ┌─────────────┐        │
│  │ DEEP SLEEP  │───────────────→│ WAKE        │        │
│  │ (~6 µA)     │  (Interrupt)   │ (~100 mA)   │        │
│  │             │                │ - Read Snsr │        │
│  │             │◄───────────────│ - TX Packet │        │
│  │             │  TX Done       │ - Sleep     │        │
│  └─────────────┘                └─────────────┘        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Interrupt-Driven Wake

```cpp
void setup() {
  // Initialize sensor
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(SENSOR_PIN, on_event, RISING);
  
  // Initialize LoRa
  init_radio();
  
  // Load config from Flash
  load_node_config();
  
  // First reading (optional)
  check_sensor();
  
  // Deep sleep until interrupt
  enter_deep_sleep();
}

void loop() {
  // Never reached (event-driven)
}

// Hardware interrupt handler
void IRAM_ATTR on_event() {
  // Wake up (instant, no delay)
  
  // Read sensor
  float value = read_sensor();
  
  // Check threshold (debounce)
  if (abs(value - last_value) > THRESHOLD) {
    send_event_packet(value);
    last_value = value;
  }
  
  // Return to deep sleep
  enter_deep_sleep();
}
```

---

## Configuration Management

### Blues Environment Variables

**Set via Blues Dashboard:**
```
https://notehub.io → Products → [Your Product] → Devices → Environment
```

**Variables:**
| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `lora_frequency` | float | 915.0 | LoRa frequency in MHz (902-927 FCC, 868 ETSI) |
| `gateway_listen_start` | int | 8 | Listen window start (08:00) |
| `gateway_listen_end` | int | 22 | Listen window end (22:00) |
| `gateway_sync_interval` | int | 360 | Env var sync (minutes) |
| `node_tx_interval` | int | 30 | Node TX interval (if periodic) |
| `node_threshold` | int | 50 | Trigger threshold (0-100) |
| `node_tx_power` | int | 17 | TX power (2-17 dBm) |
| `config_version` | int | 1 | Increment on change |

**Frequency Configuration:**
- **FCC (North America):** 902.0 - 927.0 MHz (default: 915.0)
- **ETSI (Europe):** 863.0 - 870.0 MHz (default: 868.0)
- **Predefined channels:** 903, 905, 907, 909, 915, 917, 921, 923 MHz

**Use cases for frequency changes:**
1. Avoid interference from other LoRa devices
2. Multi-gateway deployments (different gateways on different channels)
3. Regional deployment (same firmware, different frequencies)

**Gateway reads every 6 hours:**
```cpp
void check_environment_variables() {
  rak_blues.start_req("hub.get");
  rak_blues.add_bool_entry("env", true);
  
  char response[2048];
  if (rak_blues.send_req(response, sizeof(response))) {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, response);
    
    JsonObject env = doc["env"];
    
    // Check version (did config change?)
    int new_version = env["config_version"] | 0;
    if (new_version == g_config_version && g_config_version != 0) {
      return;  // No change
    }
    
    // Apply new config
    g_gateway_config.listen_start_hour = env["gateway_listen_start"] | 8;
    g_gateway_config.listen_end_hour = env["gateway_listen_end"] | 22;
    g_gateway_config.sync_interval_minutes = env["gateway_sync_interval"] | 360;
    
    // Broadcast to nodes
    broadcast_config_update();
    
    // Save to Flash
    save_gateway_config();
    
    g_config_version = new_version;
  }
}
```

### Flash Storage

**Gateway config (last 64 KB of Flash):**
```cpp
#define FLASH_CONFIG_ADDR_GATEWAY   0x000FC000  // nRF52840

typedef struct {
    uint32_t magic;               // 0x47574154 ("GWAT")
    uint8_t version;              // Config structure version
    uint8_t listen_start_hour;    // 0-23
    uint8_t listen_end_hour;      // 0-23
    uint16_t sync_interval_minutes;
    uint8_t node_tx_interval;
    uint8_t node_threshold;
    uint8_t node_tx_power;
    uint16_t crc;
} gateway_config_t;
```

**Node config (last 64 KB of Flash):**
```cpp
#define FLASH_CONFIG_ADDR_NODE    0x000FC000  // nRF52840

typedef struct {
    uint32_t magic;               // 0x4E4F4445 ("NODE")
    uint8_t version;              // Config structure version
    uint8_t node_id;              // 1-255
    uint8_t tx_interval;          // Minutes (if periodic)
    uint8_t threshold;            // 0-100
    uint8_t tx_power;             // 2-17 dBm
    uint8_t network_config_version;
    uint16_t crc;
} node_config_t;
```

---

## Blues Notehub Integration

### Webhook Configuration

**Forward `events.qo` to your server:**
```
Notehub Dashboard → Products → [Your Product] → Routes → New Route

Type: Webhook
File: events.qo
URL: https://your-server.com/webhook
Method: POST
Headers:
  Authorization: Bearer your-token
  Content-Type: application/json
```

**Webhook payload:**
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
  "payload": "base64_encoded_packet",
  "received": "2026-02-25T14:30:00Z"
}
```

### Firmware OTA

**Upload firmware via Notehub:**
```
Notehub Dashboard → Products → [Your Product] → Firmware → Upload

File: firmware.bin
Version: 1.0.1
Target: gateway_nrf52840 (or all devices)
```

**Gateway receives update:**
- Notecard downloads via cellular (~30 sec)
- Gateway validates checksum
- Gateway applies update (or schedules for next reboot)
- Rollback on failure (automatic)

---

## Power Budget

### Gateway (24 hours)

| State | Current | Duration | mAh/day |
|-------|---------|----------|---------|
| Deep sleep | 6 µA | 14 hrs (22:00-08:00) | 0.08 |
| Active listen | 6 mA | 10 hrs (08:00-22:00) | 60 |
| Cellular TX | 150 mA | ~30 sec total | 1.25 |
| Env var sync | 25 mA | ~5 min (4x/day) | 8.3 |
| **Total** | - | - | **~70 mAh/day** |

**Battery life:**
- 2000 mAh: ~28 days
- 2000 mAh + solar: indefinite

### Node (per event)

| State | Current | Duration | mAh/event |
|-------|---------|----------|-----------|
| Deep sleep | 6 µA | Between events | Base drain |
| Wake + TX | 100 mA | ~50ms | 0.0014 |

**Battery life (50 events/day):**
- Daily drain: ~15 mAh/day
- 2000 mAh: ~130 days (4+ months)
- 2000 mAh + solar: indefinite

---

## Latency Analysis

### Event → Actuator Path

| Stage | Time | Notes |
|-------|------|-------|
| **Event → Node detect** | <10ms | Hardware interrupt |
| **Node LoRa TX (SF9)** | ~30ms | Airtime |
| **Gateway RX → Blues** | ~50ms | One-shot forward |
| **Cellular → Notehub** | 2-5 sec | Unavoidable (cellular) |
| **Webhook → Actuator** | <100ms | Your system |
| **Actuator response** | <100ms | Sound/light/motor |
| **TOTAL** | **~3-7 seconds** | **Feature, not bug!** |

### Artistic Latency

**Why 5 seconds is acceptable (even desirable):**

1. **Natural delay** - Like thunder after lightning, waves after wind
2. **Distributed installation** - Sensors in Hawaii, sound in Oregon
3. **Artistic rhythm** - Events arrive when they happen, not on schedule
4. **Anticipation** - Delay builds expectation

**Compare to:**
- Wired MIDI: ~10ms (stage performance)
- Bluetooth MIDI: 20-100ms (wireless instruments)
- **Your LoRa network: ~5 seconds (global art installation)** ✅

---

## Deployment Checklist

### Gateway Deployment

- [ ] Set Blues Environment Variables (dashboard)
- [ ] Configure webhook (your server URL)
- [ ] Test LoRa range (walk test with node)
- [ ] Verify cellular signal (Blues dashboard)
- [ ] Set listen window (08:00-22:00 or custom)
- [ ] Install antenna + solar panel
- [ ] Monitor first 24 hours (dashboard)

### Node Deployment

- [ ] Set node ID (Flash or DIP switches)
- [ ] Calibrate sensor threshold (field test)
- [ ] Test LoRa TX (verify gateway receives)
- [ ] Install antenna + solar panel
- [ ] Deploy in field location
- [ ] Monitor battery (dashboard alerts)

---

**End of Design Document**
