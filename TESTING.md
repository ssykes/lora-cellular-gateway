# Testing Guide: Simulate Sensor Events

**Goal:** Send a test LoRa packet from a node and see it appear in the Blues Notehub dashboard.

---

## Option 1: Two-Board Test (Recommended)

This is the most realistic test - uses actual LoRa radio communication.

### Hardware Required

- **2x Adafruit Feather nRF52840** (or compatible)
- **2x RFM95W LoRa radios** (915 MHz)
- **1x Blues Notecard** (in gateway)
- **1x Blues NoteCarrier F** (for gateway)
- Antennas for both radios

### Setup

#### 1. Build and Upload Gateway Firmware

```bash
# Build gateway
pio run -e gateway_nrf52840

# Upload to first board
pio run -e gateway_nrf52840 -t upload
```

#### 2. Build and Upload Test Node Firmware

```bash
# Build test node
pio run -e test_node

# Upload to second board
pio run -e test_node -t upload
```

#### 3. Open Serial Monitors

**Terminal 1 - Gateway:**
```bash
pio device monitor -e gateway_nrf52840 -b 115200
```

**Terminal 2 - Test Node:**
```bash
pio device monitor -e test_node -b 115200
```

#### 4. Send Test Packet

In the **test node** serial monitor:
```
t    # Press 't' and Enter to send a test packet
```

#### 5. Verify Gateway Received

In the **gateway** serial monitor, you should see:
```
Packet received - processing
Valid sensor packet from node 1 (RSSI: -XX dBm)
Forwarding to Blues...
Blues send successful
```

#### 6. Check Blues Dashboard

1. Go to **https://notehub.io**
2. Select your **Product**
3. Click on **events.qo** file
4. You should see the sensor data:
   - `node_id`: 1
   - `value_1`: Temperature (fake)
   - `value_2`: Humidity (fake)
   - `battery_mv`: Battery voltage (fake)
   - `rssi`: Signal strength

---

## Option 2: Single-Board Loopback Test

If you only have one board, you can test the gateway-to-Blues path without LoRa.

### Modify Gateway for Testing

Temporarily add this to `gateway_main.cpp` in the `loop()` function:

```cpp
// TEST MODE: Simulate packet received
if (Serial.available() && Serial.read() == 'T') {
  Serial.println("TEST: Simulating sensor packet...");
  
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

### Test Steps

1. **Upload modified gateway:**
   ```bash
   pio run -e gateway_nrf52840 -t upload
   ```

2. **Open serial monitor:**
   ```bash
   pio device monitor -e gateway_nrf52840 -b 115200
   ```

3. **Send test command:**
   ```
   T    # Press 'T' and Enter
   ```

4. **Check Blues dashboard** as described above.

---

## Option 3: Direct Blues Notehub Test

If you want to test Blues connectivity without LoRa at all:

### Create Test Note

```cpp
#include <Wire.h>
#include <blues-minimal-i2c.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Wire.begin();
  
  RAK_BLUES blues;
  
  Serial.println("Sending test note to Blues...");
  
  if (blues.start_req((char *)"note.add")) {
    blues.add_string_entry((char *)"file", (char *)"events.qo");
    blues.add_bool_entry((char *)"sync", true);
    
    // Add test data
    blues.add_nested_string_entry((char *)"body", (char *)"test", (char *)"Hello from gateway!");
    blues.add_nested_int32_entry((char *)"body", (char *)"value", 42);
    
    if (blues.send_req()) {
      Serial.println("Test note sent successfully!");
    } else {
      Serial.println("Failed to send test note");
    }
  }
  
  while (1) delay(1000);
}

void loop() {}
```

### Upload and Monitor

```bash
# Compile and upload
pio run -e gateway_nrf52840 -t upload

# Open serial monitor
pio device monitor -e gateway_nrf52840 -b 115200
```

Check **https://notehub.io** for the test note.

---

## Troubleshooting

### Gateway Doesn't Receive Packet

1. **Check LoRa settings match:**
   - Same frequency (915.0 MHz)
   - Same spreading factor (SF10)
   - Same sync word (0x12)
   - Same bandwidth (125 kHz)

2. **Check wiring:**
   - SPI connections (CS, MOSI, MISO, SCK)
   - Reset pin connected
   - DIO0 interrupt pin connected
   - Antenna attached!

3. **Check distance:**
   - Start with boards 1 meter apart
   - Move further apart once working

### Blues Notecard Not Found

1. **Check I2C connections:**
   - SDA to SDA
   - SCL to SCL
   - 3.3V power
   - Ground

2. **Check Notecard power:**
   - NOTECARD_POWER_PIN (D5) should be HIGH
   - Measure voltage at Notecard VUSB

3. **Check product UID:**
   - Should match your Blues product

### Packet Received But Not Forwarded

1. **Check g_has_blues flag:**
   - Should be `true` after init
   - If `false`, Notecard not responding

2. **Check Blues response:**
   - "Blues send successful" = good
   - "Blues send failed" = check Notecard

### Blues Dashboard Shows Nothing

1. **Wait 2-5 seconds** for cellular upload
2. **Refresh the page**
3. **Check Notecard has signal:**
   - Blues dashboard should show "online" status
4. **Check events.qo file exists:**
   - Should auto-create on first note

---

## Expected Data Flow

```
Test Node (presses 't')
    │
    │ LoRa packet (915 MHz, SF10)
    │ ~15 bytes
    │ Airtime: ~70ms
    ▼
Gateway (receives on DIO0 interrupt)
    │
    │ Verify CRC
    │ Parse packet
    ▼
Gateway (powers on Notecard)
    │
    │ I2C transaction
    │ note.add request
    ▼
Blues Notecard
    │
    │ Cellular (LTE-M/NB-IoT)
    │ ~2-5 seconds
    ▼
Blues Notehub (cloud)
    │
    │ Webhook (if configured)
    ▼
Your Server / Dashboard
```

---

## Quick Reference Commands

```bash
# Build and upload gateway
pio run -e gateway_nrf52840 -t upload

# Build and upload test node
pio run -e test_node -t upload

# Serial monitor (gateway)
pio device monitor -e gateway_nrf52840 -b 115200

# Serial monitor (test node)
pio device monitor -e test_node -b 115200

# Both at once (in separate terminals)
start pio device monitor -e gateway_nrf52840 -b 115200
start pio device monitor -e test_node -b 115200
```

---

## Test Node Commands

When running test_node firmware:

| Command | Action |
|---------|--------|
| `t` | Send test packet immediately |
| `a` | Toggle auto-send (every 60 seconds) |
| `s` | Show status (node ID, frequency, etc.) |
| `h` | Show help |

---

**End of Testing Guide**
