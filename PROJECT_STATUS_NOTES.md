# Project Status Notes
**Project:** LoRa Cellular Gateway
**Last Updated:** February 25, 2026
**Session:** Design + Gateway Core Implementation

---

## Project Overview

**Goal:** Distributed event-triggered sensor network with cellular backhaul  
**Use Case:** Artistic installations (e.g., wind chime - Hawaii sensors → Oregon sound)  
**Key Requirements:**
- Event-driven (not periodic)
- Battery-powered (months/years with solar)
- Global reach (cellular via Blues)
- Latency tolerance: ~5 seconds (artistic feature)
- Quiet hours: 22:00-08:00 (gateway + nodes sleep)

---

## Architecture Summary

### Gateway (Interrupt-Driven)

```
Deep Sleep (~6 µA)
    ↓
LoRa packet received (DIO0 interrupt)
    ↓
Wake → Read packet → Power ON Notecard → Forward to Blues → Power OFF Notecard
    ↓
Check quiet hours → Deep sleep or resume listening
```

**Hardware:**
- Adafruit Feather nRF52840
- RFM95W LoRa radio (SPI)
- Blues Note-WBNAN + NoteCarrier F
- Pololu high-side MOSFET (Notecard power control)
- 47-100 µF bulk capacitance (recommended)
- 3.7V 2000 mAh LiPo + solar

**Power Budget:**
- Deep sleep: 6 µA (interrupt waiting)
- Active: 6 mA (RX + processing)
- Cellular TX: 150 mA (brief bursts)
- Notecard: Powered only when needed (MOSFET control)
- **Target:** ~10.7 mAh/day → ~187 days on 2000 mAh

### Nodes (Event-Driven)

```
Deep Sleep (~6 µA)
    ↓
Sensor event (hardware interrupt)
    ↓
Wake → Read sensor → TX packet → Deep sleep
    ↓
(Repeat)
```

**Hardware:**
- Adafruit Feather nRF52840
- RFM95W LoRa radio (SPI)
- Application-specific sensor
- 3.7V 2000 mAh LiPo + solar

**Power Budget:**
- Deep sleep: 6 µA
- Wake + TX: 100 mA (~50ms per event)
- **Target:** ~0.3 mAh/day (50 events) → ~18 years on 2000 mAh

---

## What's Implemented ✅

### 1. Build System ✅
- `platformio.ini` with 4 environments:
  - `gateway_nrf52840` (default, interrupt-driven)
  - `gateway_rak4630` (alternative)
  - `node_nrf52840` (ready for node code)
  - `node_heltec` (alternative)
- Dependencies: RadioLib, Blues-Minimal-I2C, Adafruit SSD1306
- Build filters (excludes legacy WisBlock code)

### 2. Protocol Stack ✅
- `src/common/protocol.h` - Packet definitions
- `src/common/protocol.cpp` - CRC-16-CCITT + packet builders
- `src/common/radio_config.h` - LoRa settings (SF9, BW125, 17 dBm)
- `src/common/config_types.h` - Config structures (Flash storage)

### 3. Gateway Main Code ✅
- `src/gateway/gateway_main.cpp` - Interrupt-driven main loop
  - DIO0 interrupt handler (`onLoRaInterrupt()`)
  - MOSFET power control (`notecard_power_on/off()`)
  - Blues initialization (one-shot at boot)
  - Environment variable check (every 6 hours)
  - Quiet hours logic (22:00-08:00)
  - Event forwarding to Blues (`events.qo`)

### 4. Documentation ✅
- `README.md` - Project overview with key features
- `DESIGN.md` - Complete system design (900+ lines)
- `HARDWARE.md` - BOM, schematics, assembly guide (NEW!)
- `QUICK_REFERENCE.md` - Build commands, pin mappings, env vars
- `IMPLEMENTATION_STATUS.md` - Progress tracking
- `PROJECT_STATUS_NOTES.md` - This file (session recovery)

---

## What's NOT Implemented ❌

### Critical (Blocks Deployment)

#### 1. Deep Sleep Implementation ❌
**File:** `src/gateway/gateway_main.cpp`  
**Functions:** `enter_deep_sleep()`, `enter_deep_sleep_until()`  
**Current Status:** Placeholders (don't actually sleep!)

```cpp
// CURRENT (placeholder - DOESN'T SLEEP!)
void enter_deep_sleep(void) {
  // TODO: Implement nRF52840 System OFF
  // delay(1000);  // Don't actually sleep yet
}

// NEEDS TO BE:
void enter_deep_sleep(void) {
  // Configure GPIO wake (DIO0)
  nrf_gpio_cfg_sense_input(RADIO_DIO0_PIN, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
  
  // Enter System OFF mode
  NRF_POWER->SYSTEMOFF = 1;
  
  // Code stops here - MCU is OFF!
  // Wake via DIO0 interrupt resumes at loop()
}
```

**Impact:** Gateway consumes ~6 mA instead of 6 µA → **4 days battery instead of 187 days!**

#### 2. RTC Timekeeping ❌
**File:** `src/gateway/gateway_main.cpp`  
**Function:** `get_current_hour()`  
**Current Status:** Always returns 12 (noon)

```cpp
// CURRENT (always returns noon)
uint8_t get_current_hour(void) {
  // TODO: Read from nRF52840 RTC
  return 12u;  // Noon (for testing)
}

// NEEDS TO BE:
uint8_t get_current_hour(void) {
  // Read nRF52840 RTC
  // Convert to hours (0-23)
  // Apply timezone offset
  return actual_hour;
}
```

**Impact:** Quiet hours never trigger! Gateway never sleeps 22:00-08:00.

#### 3. Flash Storage ❌
**File:** `src/gateway/gateway_main.cpp`  
**Functions:** `load_gateway_config()`, `save_gateway_config()`  
**Current Status:** Uses defaults, config lost on reboot

```cpp
// CURRENT (no persistence)
void load_gateway_config(void) {
  // TODO: Implement Flash read
  
  // For now, use defaults
  g_gateway_config.listen_start_hour = 8u;
  // ...
}

// NEEDS TO BE:
void load_gateway_config(void) {
  // Read from nRF52840 Flash (last 64 KB)
  // Verify magic number and CRC
  // Apply to g_gateway_config
}
```

**Impact:** Config lost on every reboot. Can't persist env var changes.

#### 4. Node Code ❌
**File:** `src/node/node_main.cpp`  
**Current Status:** **Does not exist!**

**What's needed:**
```cpp
// src/node/node_main.cpp
void setup() {
  init_sensor();
  init_radio();
  attach_interrupt(SENSOR_PIN, on_event, RISING);
  enter_deep_sleep();
}

void on_event() {
  // Wake from interrupt
  // Read sensor
  // Send LoRa packet
  // Back to deep sleep
}
```

**Impact:** No nodes to send packets! Gateway has nothing to receive.

### Important (Needed for Full Functionality)

#### 5. Environment Variable Parsing ⚠️
**File:** `src/gateway/gateway_main.cpp`  
**Function:** `check_environment_variables()`  
**Current Status:** Reads but doesn't parse JSON

```cpp
// CURRENT (logs but doesn't apply)
void check_environment_variables(void) {
  // TODO: Parse JSON and apply config
  // For now, just log it
}

// NEEDS TO BE:
void check_environment_variables(void) {
  // Read env vars from Blues
  // Parse JSON (ArduinoJson)
  // Extract: listen_start, listen_end, thresholds, etc.
  // Check version (did config change?)
  // Apply new config
  // Save to Flash
  // Broadcast to nodes (if changed)
}
```

#### 6. Blues Webhook Testing ⚠️
**Status:** Code exists but untested

**What's needed:**
1. Create Blues Notehub account
2. Set product UID: `com.hummingbird.gateway:main`
3. Configure webhook for `events.qo`
4. Test packet forwarding

---

## Build Status

```bash
# Gateway build
$ pio run -e gateway_nrf52840
========================= [SUCCESS] Took 2.58 seconds =========================
RAM:   3.8% (used 9464 bytes from 248832 bytes)
Flash: 12.4% (used 101196 bytes from 815104 bytes)
```

✅ **Compiles successfully**  
⚠️ **Runtime functionality incomplete**

---

## Hardware Status

### Gateway BOM (Complete)

| Component | Part | Status |
|-----------|------|--------|
| MCU | Adafruit Feather nRF52840 | ✅ Ready |
| LoRa | RFM95W (SX1276) 915 MHz | ✅ Ready |
| Cellular | Blues Note-WBNAN | ✅ Ready |
| Carrier | Blues NoteCarrier F | ✅ Ready |
| MOSFET | Pololu High-Side (SKU: 2679) | ⚠️ **Need to order** |
| Capacitor | 47-100 µF 6.3V tantalum | ⚠️ **Need to order** |
| Battery | LiPo 3.7V 2000 mAh | ⚠️ **Need to order** |
| Antenna | LoRa whip 915 MHz | ⚠️ **Need to order** |
| Enclosure | IP65 waterproof box | ⚠️ **Need to order** |

### Node BOM (Per Node)

| Component | Part | Status |
|-----------|------|--------|
| MCU | Adafruit Feather nRF52840 | ✅ Ready |
| LoRa | RFM95W (SX1276) 915 MHz | ✅ Ready |
| Sensor | Application-specific | ⚠️ TBD |
| Battery | LiPo 3.7V 2000 mAh | ⚠️ **Need to order** |
| Antenna | LoRa whip 915 MHz | ⚠️ **Need to order** |

---

## Next Session Priorities

### Priority 1: Deep Sleep (CRITICAL!)
**Why:** Without this, battery dies in 4 days instead of 6 months  
**File:** `src/gateway/gateway_main.cpp`  
**Functions:** `enter_deep_sleep()`, `enter_deep_sleep_until()`  
**Implementation:**
1. Configure GPIO wake (DIO0)
2. Call `NRF_POWER->SYSTEMOFF = 1`
3. Test with multimeter (verify ~6 µA)

### Priority 2: RTC Implementation
**Why:** Quiet hours won't work without timekeeping  
**File:** `src/gateway/gateway_main.cpp`  
**Function:** `get_current_hour()`  
**Implementation:**
1. Initialize nRF52840 RTC in `setup()`
2. Read RTC counter in `get_current_hour()`
3. Apply timezone offset

### Priority 3: Node Code
**Why:** No nodes = no packets = nothing to test  
**File:** `src/node/node_main.cpp`  
**Implementation:**
1. Create basic event-driven node
2. Implement sensor interrupt
3. LoRa TX on wake
4. Deep sleep

### Priority 4: Flash Storage
**Why:** Config persistence across reboots  
**File:** `src/gateway/gateway_main.cpp`  
**Functions:** `load_gateway_config()`, `save_gateway_config()`  
**Implementation:**
1. Use nRF52840 built-in Flash
2. Store in last 64 KB
3. CRC validation

---

## Testing Checklist (Next Session)

### Gateway Tests

- [ ] Upload code to gateway
- [ ] Verify serial output (boot sequence)
- [ ] Verify Blues detection
- [ ] Verify LoRa initialization
- [ ] **Test deep sleep current** (target: ~6 µA)
- [ ] **Test RTC accuracy** (compare to known time)
- [ ] **Test quiet hours** (verify sleep 22:00-08:00)

### Node Tests

- [ ] Create node stub
- [ ] Test sensor interrupt
- [ ] Test LoRa TX
- [ ] **Test deep sleep current** (target: ~6 µA)
- [ ] Test gateway receives node packets

### Integration Tests

- [ ] Gateway + Node communication
- [ ] Blues webhook forwarding
- [ ] Environment variable updates
- [ ] Config persistence (reboot test)

---

## Key Design Decisions (Reference)

| Decision | Rationale |
|----------|-----------|
| **Interrupt-driven gateway** | 99% power savings vs. continuous RX |
| **MOSFET Notecard control** | Notecard kills battery if left on (25 mA vs. 6 µA) |
| **External capacitance** | Prevents brownouts during TX (47-100 µF recommended) |
| **SF9, BW125** | Balance range (2-5 km) vs. airtime (~30ms) |
| **Blues env vars (6 hrs)** | Low-overhead config (4 checks/day vs. 96) |
| **Quiet hours 22:00-08:00** | Artistic choice + 3x battery savings |
| **Event-driven nodes** | Years of battery life vs. days/weeks |

---

## File Structure

```
hummingbird-blues-gateway/
├── src/
│   ├── common/
│   │   ├── protocol.h          ✅ Complete (packet definitions)
│   │   ├── protocol.cpp        ✅ Complete (CRC + builders)
│   │   ├── radio_config.h      ✅ Complete (LoRa settings)
│   │   └── config_types.h      ✅ Complete (config structures)
│   ├── gateway/
│   │   └── gateway_main.cpp    ⚠️ Partial (deep sleep/RTC stubs)
│   └── node/
│       └── node_main.cpp       ❌ Not started
├── platformio.ini              ✅ Complete (4 environments)
├── README.md                   ✅ Complete (overview)
├── DESIGN.md                   ✅ Complete (900+ lines)
├── HARDWARE.md                 ✅ Complete (BOM + schematics)
├── QUICK_REFERENCE.md          ✅ Complete (commands + pins)
├── IMPLEMENTATION_STATUS.md    ✅ Complete (progress tracking)
└── PROJECT_STATUS_NOTES.md     ✅ This file (session recovery)
```

---

## Session Recovery

**To resume this project:**

1. **Read this file** for current status
2. **Check `IMPLEMENTATION_STATUS.md`** for task list
3. **Review `DESIGN.md`** for full specifications
4. **Start with Priority 1** (deep sleep implementation)

**Key insight:** Gateway compiles and boots, but **deep sleep is critical** - without it, battery dies in 4 days instead of 6 months!

---

## Open Questions

| Question | Status | Notes |
|----------|--------|-------|
| NoteCarrier-F capacitance sufficient? | ⚠️ **Add 47-100 µF recommended** | Onboard ~32 µF, but external is cheap insurance |
| Pololu MOSFET part number? | ✅ **SKU: 2679** | High-side switch |
| Node sensor type? | ⚠️ **TBD** | Wind/vibration for wind chime |
| Solar panel sizing? | ⚠️ **TBD** | 1W gateway, 0.5W nodes (estimate) |

---

**End of Project Status Notes**

*Save this file for next session context!*
