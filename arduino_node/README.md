# Arduino Node - Feather M0 RFM9x

LoRa sensor node for Adafruit Feather M0 RFM9x board.

## Files

- `arduino_node.ino` - Main sketch
- `protocol.cpp` - Packet building and CRC functions
- `protocol.h` - Protocol definitions
- `radio_config.h` - LoRa radio configuration

## Setup Arduino IDE

1. **Install Arduino IDE**: https://www.arduino.cc/en/software

2. **Install Adafruit SAMD board support**:
   - Open Arduino IDE
   - File → Preferences
   - In "Additional Boards Manager URLs" add:
     ```
     https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
     ```
   - Tools → Board → Boards Manager
   - Search "Adafruit SAMD"
   - Install "Adafruit SAMD Boards"

3. **Install RadioLib library**:
   - Sketch → Include Library → Manage Libraries
   - Search "RadioLib"
   - Install "RadioLib" by Jan Gromes

## Upload

1. **Open sketch**: File → Open → `arduino_node\arduino_node.ino`

2. **Select board**:
   - Tools → Board → Adafruit SAMD (nRF52840/SAMD21) → **Adafruit Feather M0 RFM9x**

3. **Select port**:
   - Tools → Port → COMxx (Windows) or /dev/ttyxx (Mac/Linux)

4. **Upload**:
   - Click → (Upload) button
   - If upload fails, **double-tap reset** button and try again

## Verify

1. **Open Serial Monitor**:
   - Tools → Serial Monitor
   - Set baud rate: **115200**
   - Press reset on board

2. **Expected output**:
   ```
   *** Node Hello World ***
   Board: Adafruit Feather M0 RFM9x
   Node ID: 1
   TX Interval: 30 seconds

   Initializing LoRa radio...
   Radio initialized successfully
     Frequency: 915.0 MHz
     Spreading Factor: 10
     Bandwidth: 125 kHz
     TX Power: 14 dBm

   *** Node ready - sending first packet ***

   [TX 1] Sending sensor data...
   Sensors: temp=22.5C, humidity=45.0%, battery=3700mV
   Transmitting packet...
   TX successful!
   ```

3. **LED behavior**:
   - Blinks 3 times fast on startup
   - Flashes briefly each time a packet is sent (every 30 seconds)

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Upload fails | Double-tap reset, try again |
| No serial output | Check baud rate (115200), press reset |
| Radio init failed | Check board is Feather M0 RFM9x |
| TX failed | Check antenna is connected |

## Next Steps

- Pair with gateway to receive packets
- Add real sensors (BME280, etc.)
- Configure for deployment (longer TX interval)
