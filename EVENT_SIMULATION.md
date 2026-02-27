# Simulating Sensor Events

**Goal:** Send test LoRa packets from a node and see the data appear in the Blues Notehub dashboard.

**Date:** February 26, 2026  
**Project:** LoRa Cellular Gateway

---

## Overview

This document describes three methods to simulate sensor events and verify data flows through your gateway to the Blues cloud.

| Method | Hardware | Complexity | Tests |
|--------|----------|------------|-------|
| **Option 1** | 2 boards + LoRa radios | Medium | Full end-to-end (LoRa + Cellular) |
| **Option 2** | 1 board (gateway only) | Low | Gateway → Blues path only |
| **Option 3** | 1 board (gateway only) | Low | Blues connectivity only |

---

## Option 1: Two-Board Test (Recommended)

This is the most realistic test — uses actual LoRa radio communication between two devices.

### Hardware Required

- **2×** Adafruit Feather nRF52840 (or compatible boards)
- **2×** RFM95W LoRa radio modules (915 MHz)
- **1×** Blues Notecard (installed in gateway)
- **1×** Blues NoteCarrier F (for gateway)
- **2×** LoRa antennas (915 MHz)

### Wiring (Both Boards)

Connect RFM95W to nRF52840:

| RFM95W Pin | nRF52840 Pin |
|------------|--------------|
| 3.3V       | 3.3V         |
| GND        | GND          |
| MISO       | D12 (MISO)   |
| MOSI       | D11 (MOSI)   |
| SCK        | D13 (SCK)    |
| NSS (CS)   | D10          |
| RST        | D8           |
| DIO0       | D4           |
| DIO1       | D7 (optional)|

### Step 1: Build and Upload Gateway

```bash
# Build gateway firmware
pio run -e gateway_nrf52840

# Upload to first board
pio run -e gateway_nrf52840 -t upload
```

### Step 2: Create Test Node Sketch

Create a new Arduino sketch or PlatformIO project for the second board. Here's a minimal test node:

```cpp
/**
 * Test Node Sketch
 * Sends fake sensor data via LoRa to gateway
 */

#include <SPI.h>
#include <RadioLib.h>
#include "protocol.h"

// Pin definitions
#define RADIO_CS    10
#define RADIO_RST   8
#define RADIO_DIO0  4

// Radio instance
SX1276 radio = new Module(RADIO_CS, RADIO_DIO0, RADIO_RST);

// Node configuration
#define NODE_ID  1
#define NODE_FREQ   915.0f
#define NODE_BW     125.0f
#define NODE_SF     10
#define NODE_CR     5
#define NODE_SYNC   0x12
#define NODE_PWR    14

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n*** Test Node ***");
  
  // Initialize radio
  int state = radio.begin(
    NODE_FREQ, NODE_BW, NODE_SF, NODE_CR,
    NODE_SYNC, NODE_PWR, 8, true
  );
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio OK");
  } else {
    Serial.printf("Radio FAIL: %d\n", state);
    while (1);
  }
}

void loop() {
  if (Serial.available() && Serial.read() == 't') {
    send_test_packet();
  }
  delay(100);
}

void send_test_packet() {
  Serial.println("\nSending test packet...");
  
  // Build sensor payload (fake data)
  sensor_payload_t payload;
  payload.temperature = 23.5;
  payload.humidity = 55.0;
  payload.battery_mv = 4000;
  payload.sensor_flags = 0;
  
  // Build packet
  packet_t pkt;
  protocol_build_sensor_packet(&pkt, NODE_ID, 1, &payload);
  
  // Transmit
  int state = radio.transmit(pkt.raw, SENSOR_PACKET_SIZE);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX successful!");
  } else {
    Serial.printf("TX failed: %d\n", state);
  }
}
```

### Step 3: Upload Test Node

```bash
# Upload to second board (adjust port as needed)
pio run -t upload -p <COM_PORT>
```

Or use Arduino IDE to upload the sketch.

### Step 4: Open Serial Monitors

**Terminal 1 - Gateway:**
```bash
pio device monitor -e gateway_nrf52840 -b 115200
```

**Terminal 2 - Test Node:**
```bash
pio device monitor -p <COM_PORT> -b 115200
```

### Step 5: Send Test Packet

In the **test node** serial monitor, type:
```
t
```

### Step 6: Verify Gateway Received

In the **gateway** serial monitor, you should see:
```
Packet received - processing
Valid sensor packet from node 1 (RSSI: -XX dBm)
Forwarding to Blues...
Blues send successful
```

### Step 7: Check Blues Dashboard

1. Go to **https://notehub.io**
2. Sign in and select your **Product**
3. Click on **events.qo** file
4. You should see sensor data:
   - `node_id`: 1
   - `value_1`: 23.5 (temperature)
   - `value_2`: 55.0 (humidity)
   - `battery_mv`: 4000
   - `rssi`: Signal strength (e.g., -72)

---

## Option 2: Single-Board Loopback Test

If you only have one board, test the gateway-to-Blues path without LoRa.

### Modify Gateway for Test Mode

Add this to `gateway_main.cpp` in the `loop()` function:

```cpp
// TEST MODE: Simulate packet when 'T' is pressed
if (Serial.available() && Serial.read() == 'T') {
  DEBUG_SERIAL.println("TEST: Simulating sensor packet...");
  
  // Build fake sensor packet
  sensor_packet_t test_pkt;
  test_pkt.type = PKT_TYPE_SENSOR_DATA;
  test_pkt.node_id = 99;  // Test node ID
  test_pkt.hop_count = 0;
  test_pkt.config_version = 1;
  test_pkt.payload.temperature = 23.5;
  test_pkt.payload.humidity = 55.0;
  test_pkt.payload.battery_mv = 4000;
  test_pkt.payload.sensor_flags = 0;
  test_pkt.rssi = -75;  // Fake RSSI
  
  // Forward to Blues
  forward_to_blues(&test_pkt);
}
```

### Build and Upload

```bash
pio run -e gateway_nrf52840 -t upload
```

### Test

```bash
# Open serial monitor
pio device monitor -e gateway_nrf52840 -b 115200

# Type 'T' and press Enter
```

### Expected Output

```
TEST: Simulating sensor packet...
Forwarding to Blues...
Blues send successful
```

Check **https://notehub.io** for the test data.

---

## Option 3: Direct Blues Notehub Test

Test Blues connectivity without any LoRa involvement.

### Create Test Sketch

```cpp
/**
 * Direct Blues Test
 * Sends a test note directly to Blues Notehub
 */

#include <Wire.h>
#include <blues-minimal-i2c.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n*** Blues Direct Test ***");
  
  Wire.begin();
  
  RAK_BLUES blues;
  
  Serial.println("Sending test note to Blues...");
  
  if (blues.start_req((char *)"note.add")) {
    blues.add_string_entry((char *)"file", (char *)"events.qo");
    blues.add_bool_entry((char *)"sync", true);
    
    // Add test data
    blues.add_nested_string_entry(
      (char *)"body", (char *)"test", (char *)"Hello from gateway!"
    );
    blues.add_nested_int32_entry(
      (char *)"body", (char *)"value", 42
    );
    blues.add_nested_string_entry(
      (char *)"body", (char *)"source", (char *)"direct_test"
    );
    
    if (blues.send_req()) {
      Serial.println("Test note sent successfully!");
    } else {
      Serial.println("Failed to send test note");
    }
  } else {
    Serial.println("Failed to start request");
  }
  
  Serial.println("\nCheck https://notehub.io for the note");
  
  while (1) {
    delay(1000);
  }
}

void loop() {}
```

### Build and Upload

```bash
pio run -e gateway_nrf52840 -t upload
```

### Check Blues Dashboard

1. Go to **https://notehub.io**
2. Select your **Product**
3. Click **events.qo**
4. Look for the test note with:
   - `test`: "Hello from gateway!"
   - `value`: 42
   - `source`: "direct_test"

---

## Troubleshooting

### Gateway Doesn't Receive Packet

| Issue | Solution |
|-------|----------|
| LoRa settings mismatch | Ensure both boards use same frequency, SF, BW, sync word |
| Wiring error | Double-check SPI connections (CS, MOSI, MISO, SCK) |
| No antenna | Always attach antenna before testing! |
| Too far apart | Start with boards 1 meter apart |

### Blues Notecard Not Found

| Issue | Solution |
|-------|----------|
| I2C wiring | Check SDA/SCL connections |
| Power | Verify 3.3V to Notecard VUSB |
| Notecard dead | Try different Notecard or carrier |

### Packet Received But Not Forwarded

| Issue | Solution |
|-------|----------|
| g_has_blues = false | Notecard not initialized, check I2C |
| Blues send failed | Check cellular signal in Blues dashboard |
| No product configured | Set product UID in code or dashboard |

### Blues Dashboard Shows Nothing

| Issue | Solution |
|-------|----------|
| Wait longer | Cellular upload takes 2-5 seconds |
| Refresh page | Dashboard may not auto-refresh |
| No signal | Check Notecard status in dashboard |
| Wrong product | Verify product UID matches |

---

## Data Flow Diagram

```
┌─────────────────┐
│   Test Node     │
│  (presses 't')  │
└────────┬────────┘
         │
         │ LoRa packet (915 MHz, SF10)
         │ ~15 bytes, ~70ms airtime
         ▼
┌─────────────────┐
│     Gateway     │
│  (DIO0 interrupt)│
└────────┬────────┘
         │
         │ Verify CRC, parse packet
         ▼
┌─────────────────┐
│     Gateway     │
│ (power on Notecard)│
└────────┬────────┘
         │
         │ I2C transaction
         │ note.add request
         ▼
┌─────────────────┐
│ Blues Notecard  │
│  (cellular modem)│
└────────┬────────┘
         │
         │ LTE-M / NB-IoT
         │ ~2-5 seconds
         ▼
┌─────────────────┐
│ Blues Notehub   │
│    (cloud)      │
└────────┬────────┘
         │
         │ Webhook (if configured)
         ▼
┌─────────────────┐
│ Your Server /   │
│   Dashboard     │
└─────────────────┘
```

---

## Quick Reference Commands

```bash
# Build and upload gateway
pio run -e gateway_nrf52840 -t upload

# Serial monitor (gateway)
pio device monitor -e gateway_nrf52840 -b 115200

# List serial ports
pio device list

# Clean build
pio run --target clean
```

---

## Test Node Commands

If using the full test node sketch:

| Command | Action |
|---------|--------|
| `t` | Send test packet immediately |
| `s` | Show status (node ID, frequency) |
| `h` | Show help |

---

## Expected RSSI Values

| Distance | RSSI (dBm) | Quality |
|----------|------------|---------|
| 1 meter | -30 to -50 | Excellent |
| 10 meters | -50 to -70 | Good |
| 100 meters | -70 to -90 | Fair |
| 500+ meters | -90 to -110 | Poor |

---

## Next Steps

After successful testing:

1. **Configure Blues webhooks** to forward data to your server
2. **Set up environment variables** for remote configuration
3. **Deploy in field** with solar power
4. **Monitor dashboard** for incoming events

---

**End of Event Simulation Guide**
