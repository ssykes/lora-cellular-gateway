# nRF52840 Gateway - Using OFFICIAL Blues Library

**This version uses the official Blues library that Blues Wireless has tested and confirmed works with the Adafruit Feather nRF52840 Express.**

Reference: https://dev.blues.io/datasheets/notecarrier-datasheet/notecarrier-f-v1-3/

## Files

- `arduino_gateway_nrf52840.ino` - Main gateway sketch
- `protocol.cpp` / `protocol.h` - Packet CRC functions

## Arduino IDE Setup

### 1. Install Arduino IDE

Download from: https://www.arduino.cc/en/software

### 2. Install Adafruit nRF52 Board Support

1. **File → Preferences**
2. Add to **"Additional Boards Manager URLs"**:
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
3. **Tools → Board → Boards Manager**
4. Search **"Adafruit nRF52"**
5. Install **"Adafruit nRF52 by Adafruit"**

### 3. Install Libraries

**Sketch → Include Library → Manage Libraries:**

1. **RadioLib** by Jan Gromes
2. **Blues** by Blues Wireless (OFFICIAL library)

### 4. Open Sketch

**File → Open →** `arduino_gateway_nrf52840\arduino_gateway_nrf52840.ino`

### 5. Select Board

**Tools → Board → Adafruit nRF52 → Adafruit Feather nRF52840 Express**

### 6. Select Port

**Tools → Port →** COMxx

### 7. Upload

1. Click **→ (Upload)**
2. If upload fails, **double-tap reset** button

### 8. Serial Monitor

1. **Tools → Serial Monitor**
2. Baud rate: **115200**
3. Press reset on board

## Expected Output

```
*** LoRa-to-Blues Gateway ***
Board: Adafruit Feather nRF52840 Express
Using OFFICIAL Blues Library

Initializing Blues Notecard...
✓ Blues Notecard found!
✓ Notecard configured

Initializing LoRa radio...
  ✓ Radio OK
  Freq: 915 MHz, SF: 10, BW: 125 kHz
  Sync: 0x12, CRC: on

*** Gateway ready - listening for packets ***
```

## When Node Transmits

```
Listening...
[RX] Packet! Length: 18, RSSI: -48 dBm
  Data: 01 01 00 00 01 ...
  Type: 0x01, Node: 1, Config: 1
  Temp: 24.0C, Humidity: 45.0%, Battery: 4316mV
  Forwarding to Blues...
  ✓ Blues send SUCCESS!
```

## Hardware Connections

Your existing setup:
```
LoRa FeatherWing (RFM95W)
    ↓
Adafruit Feather nRF52840 Express
    ↓
Blues Notecarrier-F (plugs into Feather socket)
    ↓
Blues Notecard (inside Notecarrier-F)
```

**No additional wiring needed!** Everything connects through the Feather headers.

## Check Blues Notehub

1. Go to https://notehub.io
2. Select your project
3. Look for `events.qo` files
4. Should contain sensor data from node

## Troubleshooting

### Notecard NOT Found

```
✗ Notecard NOT found - check I2C wiring!
```

**Check:**
- Notecard is seated properly in Notecarrier-F
- Notecarrier-F is firmly plugged into Feather nRF52840
- Try reseating all connections

### Blues Send Fails

```
✗ Blues send FAILED
```

**Check:**
- Notecard has cellular signal (check antenna if using cellular model)
- Product ID is correct in code
- Notecard is activated in Notehub

### No Packets Received

**Check:**
- Node is transmitting (verify with node Serial output)
- LoRa antenna is connected to FeatherWing
- Both boards use same frequency (915 MHz)

---

**This should work because Blues has officially tested this exact hardware combination!** 🚀
