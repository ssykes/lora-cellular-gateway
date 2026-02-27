# Project Context - LoRa Cellular Gateway

**Last Updated:** February 24, 2026
**Session ID:** Design Phase Complete
**Next Phase:** Implementation

---

## Quick Resume

This project builds a **low-power LoRa sensor network** with **Blues cellular backhaul**. The existing codebase (`lora-cellular-gateway`) is being **modified** (not replaced) to support:

1. **5-10 battery-powered sensor nodes** (nRF52840 or HelTec ESP32)
2. **Simple store-and-forward relay** (not full mesh)
3. **Time-based gateway listening** (08:00-22:00, configurable)
4. **Embedded config requests** in every sensor transmission
5. **Blues Notehub mailbox** for remote configuration from web app

---

## Critical Design Decisions (DO NOT CHANGE WITHOUT DISCUSSION)

| Decision | Rationale | Status |
|----------|-----------|--------|
| **Modify existing project** | Blues integration already exists, saves 60% work | ✅ Agreed |
| **Star topology with optional relay** | User wants range extension, not complex mesh | ✅ Agreed |
| **No encryption** | DIY project, trusted network, reduces overhead | ✅ Agreed |
| **Single frequency (915 MHz)** | Simplifies hardware/firmware | ✅ Agreed |
| **Config embedded in sensor TX** | Zero overhead when unchanged, simplest approach | ✅ Agreed |
| **Silent ACK for normal data** | Reduces airtime, nodes don't need confirmation | ✅ Agreed |
| **Gateway duty-cycled (08:00-22:00)** | User requirement for power/cost management | ✅ Agreed |
| **Nodes independent schedule** | No sync required, simpler firmware | ✅ Agreed |
| **RadioLib for LoRa** | Supports RFM95W (SX127x), clean API | ✅ Agreed |
| **Blues for backhaul (not MQTT)** | User's hardware choice | ✅ Agreed |

---

## Hardware Commitments (User Already Owns)

### Gateway
- **Adafruit Feather M0** (SAMD21) + **RFM95W** LoRa
- **Blues Note-WBNAN** (LTE-M/NB-IoT) in **NoteCarrier F**
- Power: USB or 5V DC (always powered)

### Nodes (5-10x)
- **Option A:** Adafruit nRF52840 + RFM95W (preferred for battery)
- **Option B:** HelTec WiFi LoRa 32 V5 (ESP32, for relay nodes)
- Power: LiPo + solar charging

---

## Current Project State

### Existing Codebase Analysis

**Location:** `e:\vsCode\workspaces\hummingbird-blues-gateway`

**What to KEEP:**
- ✅ Blues Notecard integration (`blues.cpp`, `blues_parse_send.cpp`)
- ✅ WisBlock-API-V2 power management concepts
- ✅ User AT command framework (`user_at_cmd.cpp`)
- ✅ OLED display support (`RAK1921_oled.cpp`)
- ✅ Battery monitoring
- ✅ PlatformIO structure

**What to REPLACE:**
- ❌ WisBlock-API-V2 (too much overhead, RAK-specific)
- ❌ LoRaWAN stack (not needed for P2P)
- ❌ Cayenne LPP encoding (use simple binary protocol)
- ❌ RAK-specific board support (replace with Adafruit/HelTec)

**What to ADD:**
- ➕ RadioLib (for RFM95W/SX127x control)
- ➕ Simple binary protocol (node ID, config version, sensor data)
- ➕ Config version tracking (global + per-node)
- ➕ Time-based scheduler (08:00-22:00 listen window)
- ➕ Relay logic (optional, hop count)
- ➕ Board support for Adafruit Feather + HelTec

---

## File Structure (Planned)

```
hummingbird-blues-gateway/
├── DESIGN.md                    # ✅ Created - Full design document
├── PROJECT_CONTEXT.md           # ✅ This file - Context for session recovery
├── platformio.ini               # ⚠️ Modify - Add environments for gateway/nodes
├── src/
│   ├── gateway/
│   │   ├── main.cpp             # ⚠️ Modify - Gateway main loop
│   │   ├── gateway_config.cpp   # ➕ New - Config management
│   │   ├── gateway_radio.cpp    # ➕ New - LoRa RX handling
│   │   ├── gateway_blues.cpp    # ➕ New - Blues integration (keep existing blues.cpp)
│   │   └── gateway_scheduler.cpp# ➕ New - Time window scheduling
│   │
│   ├── node/
│   │   ├── node_main.cpp        # ➕ New - Node firmware (separate build)
│   │   ├── node_sensor.cpp      # ➕ New - Sensor reading
│   │   ├── node_radio.cpp       # ➕ New - LoRa TX handling
│   │   └── node_config.cpp      # ➕ New - Config storage
│   │
│   ├── common/
│   │   ├── protocol.h           # ➕ New - Packet structures
│   │   ├── radio_config.h       # ➕ New - LoRa settings
│   │   └── config_types.h       # ➕ New - Config data structures
│   │
│   ├── blues.cpp                # ✅ Keep - Blues Notecard driver
│   ├── blues.h                  # ✅ Keep
│   ├── main.h                   # ⚠️ Modify - Update includes/defines
│   └── user_at_cmd.cpp          # ✅ Keep - AT command framework
│
├── lib/
│   └── RadioLib/                # ➕ Add - RadioLib dependency
│
├── include/
│   └── (shared headers)
│
└── test/
    └── (unit tests)
```

---

## Protocol Specification (Final)

### Packet Structure

**Sensor Data Packet (0x01)** - Node → Gateway
```
Offset  Size  Field           Description
0       1     Type            0x01 = Sensor Data
1       1     Node ID         1-255 (0 = gateway)
2       1     Hop Count       0 = direct, 1-3 = relayed
3       1     RSSI            Added by receiver
4       1     Config Version  Node's current config version
5-8     4     Sensor Data     Temperature (float) or custom payload
9-10    2     CRC             16-bit CRC
```

**Config Update Packet (0x02)** - Gateway → Node
```
Offset  Size  Field           Description
0       1     Type            0x02 = Config Update
1       1     Node ID         Target node
2       1     Flags           Bit 0 = relay enabled
3       1     Config Version  New config version
4-5     2     Interval        Transmission interval (minutes)
6       1     Relay           Relay mode (0/1)
7-8     2     CRC             16-bit CRC
```

### Packet Type Constants

```cpp
#define PKT_TYPE_SENSOR_DATA    0x01
#define PKT_TYPE_CONFIG_UPDATE  0x02
#define PKT_TYPE_ACK          0x03
#define PKT_TYPE_RELAY        0x04
```

---

## Blues Notehub Setup (Required)

### Mailbox Configuration

**File:** `config.qo` (gateway reads this)

```json
{
  "gateway": {
    "listen_start_hour": 8,
    "listen_end_hour": 22,
    "timezone": "America/New_York",
    "wake_interval_minutes": 15,
    "listen_window_seconds": 30
  },
  "global_config_version": 1,
  "nodes": [
    {"id": 1, "interval_minutes": 30, "relay_enabled": false},
    {"id": 2, "interval_minutes": 30, "relay_enabled": true}
  ]
}
```

### Sensor Data Forwarding

**File:** `sensors.qo` (gateway writes this)

```json
{
  "node_id": 3,
  "timestamp": "2026-02-24T14:30:00Z",
  "temperature_c": 23.5,
  "humidity_pct": 65.2,
  "battery_mv": 3850,
  "rssi_dbm": -87,
  "hops": 0
}
```

### Webhook Setup

Configure in Blues Notehub console:
- **Trigger:** When `sensors.qo` is created
- **Action:** POST to `https://your-server.com/api/sensor-data`
- **Method:** POST with JSON body

---

## Radio Configuration (Final)

```cpp
// LoRa Settings - SF10 for balance of range/airtime
#define LORA_FREQUENCY        915.0f      // MHz (North America)
#define LORA_BANDWIDTH        125.0f      // kHz
#define LORA_SF               10          // Spreading factor
#define LORA_CODING_RATE      5           // 4/5
#define LORA_TX_POWER         14          // dBm (FCC compliant)
#define LORA_PREAMBLE_LEN     8           // Symbols
#define LORA_CRC_ENABLED      true

// Timing
#define GATEWAY_LISTEN_START  8           // 08:00 (24-hour format)
#define GATEWAY_LISTEN_END    22          // 22:00
#define GATEWAY_WAKE_INTERVAL  15          // Minutes (check Blues)
#define GATEWAY_LISTEN_WINDOW 30          // Seconds (RX duration)
#define NODE_TX_INTERVAL      30          // Minutes (default)
#define NODE_CONFIG_LISTEN    3000        // ms (wait for config response)
```

---

## Implementation Checklist

### Phase 1: Setup (Week 1)
- [ ] Update `platformio.ini` with gateway + node environments
- [ ] Add RadioLib dependency
- [ ] Create board definitions (Adafruit Feather M0, nRF52840, HelTec)
- [ ] Create `protocol.h` with packet structures
- [ ] Create `radio_config.h` with LoRa settings
- [ ] Test basic LoRa TX/RX between two devices

### Phase 2: Gateway Core (Week 2)
- [ ] Implement Blues mailbox reader
- [ ] Implement time scheduler (RTC + listen window)
- [ ] Implement LoRa RX handler
- [ ] Implement config version tracking
- [ ] Test gateway receiving sensor packets

### Phase 3: Node Core (Week 3)
- [ ] Implement sensor reading (start with mock data)
- [ ] Implement LoRa TX with config version
- [ ] Implement config update listener
- [ ] Implement deep sleep
- [ ] Test node TX → gateway RX

### Phase 4: Configuration System (Week 4)
- [ ] Implement config storage (Flash)
- [ ] Implement config update responder (gateway)
- [ ] Implement config apply (node)
- [ ] Test config update flow (web → Blues → gateway → node)

### Phase 5: Relay & Polish (Week 5)
- [ ] Implement relay mode (optional)
- [ ] Add hop count logic
- [ ] Add RSSI tracking
- [ ] Range testing
- [ ] Battery life testing

### Phase 6: Integration (Week 6)
- [ ] Blues webhook testing
- [ ] Web app integration test
- [ ] Multi-node stress test (5-10 nodes)
- [ ] Documentation cleanup
- [ ] Release v1.0

---

## Key Functions to Implement

### Gateway

```cpp
// Main loop
void gateway_loop() {
    check_blues_mailbox();      // Every 15 min
    if (within_listen_window()) {
        enable_lora_rx();
        process_packets();
        disable_lora_rx();
    }
    forward_to_blues();
    sleep_until_next_wake();
}

// Packet handling
void on_sensor_data(Packet* pkt) {
    blues_forward(pkt);
    if (pkt->config_version != global_config_version) {
        send_config_update(pkt->node_id);
    }
}
```

### Node

```cpp
// Main loop
void node_loop() {
    read_sensors();
    send_sensor_data();
    listen_for_config(3000);
    deep_sleep(interval_minutes * 60 * 1000);
}

// Config handling
void on_config_update(Packet* pkt) {
    if (pkt->node_id == MY_NODE_ID) {
        apply_config(pkt->config);
        save_config_version(pkt->config_version);
    }
}
```

---

## Testing Plan

### Unit Tests
- [ ] Packet CRC calculation
- [ ] Config version comparison
- [ ] Time window calculation
- [ ] Blues mailbox parsing

### Integration Tests
- [ ] Single node → gateway communication
- [ ] Config update flow
- [ ] Relay mode (2-hop)
- [ ] Multi-node (5 nodes simultaneous)

### Field Tests
- [ ] Range test (line of sight)
- [ ] Range test (obstructed)
- [ ] Battery life (1 week monitoring)
- [ ] Gateway duty cycle verification

---

## Known Issues / Open Questions

| Issue | Status | Notes |
|-------|--------|-------|
| RTC backup battery | ⚠️ Verify | Feather M0 has RTC but needs backup power |
| Blues time sync | ⚠️ Implement | Get time from cellular network |
| Flash storage location | ⚠️ Decide | Internal Flash or external EEPROM? |
| Node ID assignment | ⚠️ Document | Compile-time or runtime configuration? |
| Firmware update mechanism | 🔴 Future | OTA not in v1.0 |
| Encryption | 🔴 Won't implement | User confirmed not needed |

---

## Dependencies (platformio.ini)

```ini
[env:gateway]
platform = atmelsam
board = adafruit_feather_m0
framework = arduino
lib_deps =
    beegee-tokyo/Blues-Minimal-I2C @ ^0.0.4
    jgromes/RadioLib @ ^6.0.0

[env:node_nrf52840]
platform = nordicnrf52
board = adafruit_feather_nrf52840
framework = arduino
lib_deps =
    jgromes/RadioLib @ ^6.0.0

[env:node_heltec]
platform = espressif32
board = heltec_wifi_lora_32_V5
framework = arduino
lib_deps =
    jgromes/RadioLib @ ^6.0.0
```

---

## Session Recovery Instructions

If this session is lost and you need to resume:

1. **Read this file first** (`PROJECT_CONTEXT.md`)
2. **Then read** `DESIGN.md` for full specifications
3. **Check current implementation state:**
   ```bash
   # See what files exist
   ls -la src/
   
   # Check platformio.ini for environments
   cat platformio.ini
   
   # See what's been implemented
   git log --oneline  # If using git
   ```
4. **Resume from Implementation Checklist** - find last completed item
5. **Key contact:** User has hardware commitments as specified above

---

## Contact / Decision Log

| Date | Decision | Participants |
|------|----------|--------------|
| 2026-02-24 | Modify existing project (not start fresh) | User + Assistant |
| 2026-02-24 | Star topology with simple relay (not full mesh) | User + Assistant |
| 2026-02-24 | Config embedded in sensor TX (not separate beacon) | User + Assistant |
| 2026-02-24 | Gateway listen window 08:00-22:00 (user requirement) | User |
| 2026-02-24 | No encryption (DIY project) | User |
| 2026-02-24 | RadioLib for LoRa control | Assistant recommendation |
| 2026-02-24 | Blues for backhaul (user hardware) | User |

---

## Quick Reference Commands

```bash
# Build gateway
pio run -e gateway

# Build node firmware
pio run -e node_nrf52840
pio run -e node_heltec

# Upload to gateway
pio run -e gateway -t upload

# Upload to node
pio run -e node_nrf52840 -t upload

# Serial monitor
pio device monitor -p /dev/ttyUSB0 -b 115200

# Clean build
pio run --clean
```

---

## Files Created This Session

| File | Purpose | Status |
|------|---------|--------|
| `DESIGN.md` | Complete system design | ✅ Created |
| `PROJECT_CONTEXT.md` | This file - session recovery | ✅ Created |

---

**END OF PROJECT CONTEXT**

*Save this file and refer to it if the session is lost.*
