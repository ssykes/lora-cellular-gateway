/**
 * @file node_main_heltec_v4.cpp
 * @brief Sensor node for HelTec WiFi LoRa 32 V3 (ESP32 + SX1262)
 * @version 1.0
 * @date 2026-02-27
 *
 * Power strategy:
 * - Light sleep between transmissions
 * - Current: ~100 µA in sleep, ~150 mA during TX
 * - Wake via timer (RTC)
 * - OLED display for debug (optional)
 *
 * Hardware:
 * - HelTec WiFi LoRa 32 V3
 * - Built-in SX1262 LoRa radio
 * - Optional: OLED, GPS, sensors
 */

#include <Arduino.h>
#include <RadioLib.h>
#include "protocol.h"
#include "radio_config.h"
#include "pins.h"

// ============================================================================
// Configuration
// ============================================================================

#define NODE_ID             3           // Unique ID (1-255)
#define TX_INTERVAL_SEC     300         // 5 minutes

// Display
#define USE_OLED            0           // 1=Enable OLED, 0=Disable

#ifdef USE_OLED
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define OLED_WIDTH  128
  #define OLED_HEIGHT 64
  Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
#endif

// ============================================================================
// RadioLib Instance (SX1262 for HelTec V4)
// ============================================================================

SX1262 radio = new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

// ============================================================================
// Globals
// ============================================================================

uint32_t g_tx_count = 0;
uint8_t g_config_version = CONFIG_VERSION_INITIAL;
float g_last_temp = 0.0f;

// ============================================================================
// Forward Declarations
// ============================================================================

void init_radio(void);
void init_display(void);
void read_sensors(sensor_payload_t* payload);
void send_sensor_data(void);
uint16_t read_battery_mv(void);
void enter_light_sleep(uint32_t sleep_ms);

// ============================================================================
// Setup
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(2000);

  Serial.println("\n*** Node HelTec V4 (ESP32-S3) ***");
  Serial.printf("Board: HelTec WiFi LoRa 32 V3\n");
  Serial.printf("Node ID: %d\n", NODE_ID);
  Serial.printf("TX Interval: %d seconds\n", TX_INTERVAL_SEC);

#ifdef USE_OLED
  init_display();
#endif

  init_radio();

  Serial.println("*** Ready ***\n");

  // First transmission
  send_sensor_data();

  // Sleep
  Serial.println("\nSleeping...\n");
  enter_light_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
}

// ============================================================================
// Main Loop (not used - sleep in setup)
// ============================================================================

void loop() {
  // Wake from sleep continues here
  send_sensor_data();
  enter_light_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
}

// ============================================================================
// Display Initialization
// ============================================================================

void init_display(void) {
#ifdef USE_OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("HelTec Node V3");
  display.printf("ID: %d\n", NODE_ID);
  display.display();

  Serial.println("OLED initialized");
#endif
}

// ============================================================================
// Radio Initialization
// ============================================================================

void init_radio(void) {
  Serial.println("Initializing SX1262...");

  // SX1262 requires different init than SX1276
  int state = radio.begin(
    LORA_FREQUENCY_DEFAULT,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODING_RATE,
    LORA_SYNC_WORD,
    LORA_TX_POWER,
    LORA_PREAMBLE_LENGTH,
    0                       // Current limit (0 = auto)
  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.printf("Radio OK: %.1f MHz, SF%d, %d dBm\n",
                  LORA_FREQUENCY_DEFAULT, LORA_SPREADING_FACTOR, LORA_TX_POWER);
  } else {
    Serial.printf("ERROR: Radio init failed: %d\n", state);
    while (1) {
      delay(200);
    }
  }
}

// ============================================================================
// Sensor Reading
// ============================================================================

void read_sensors(sensor_payload_t* payload) {
  // HelTec V4 has no built-in sensors (except battery)
  // Add external sensors as needed

  // Fake values for testing
  payload->temperature = 23.0f + (float)g_tx_count * 0.05f;
  payload->humidity = 50.0f + (float)(g_tx_count % 5);
  payload->battery_mv = read_battery_mv();
  payload->sensor_flags = 0;

  if (payload->battery_mv < 3500) {
    payload->sensor_flags |= SENSOR_FLAG_BATTERY_LOW;
  }

  Serial.printf("Sensors: T=%.1fC, H=%.1f%%, Vbat=%dmV\n",
                payload->temperature, payload->humidity, payload->battery_mv);

#ifdef USE_OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("TX #%lu\n", g_tx_count);
  display.printf("T: %.1fC\n", payload->temperature);
  display.printf("H: %.1f%%\n", payload->humidity);
  display.printf("V: %dmV\n", payload->battery_mv);
  display.display();
#endif
}

/**
 * @brief Read battery voltage
 *
 * HelTec V4: Battery voltage on A0 (through divider)
 */
uint16_t read_battery_mv(void) {
  int adc = analogRead(PIN_BATTERY);
  float voltage = (adc / 4095.0f) * 3.3f * 2.0f;  // Approximate divider
  return (uint16_t)(voltage * 1000.0f);
}

// ============================================================================
// Packet Transmission
// ============================================================================

void send_sensor_data(void) {
  Serial.printf("\n[TX %lu] Sending...\n", ++g_tx_count);

  sensor_payload_t payload;
  read_sensors(&payload);

  packet_t pkt;
  protocol_build_sensor_packet(&pkt, NODE_ID, g_config_version, &payload);

  Serial.println("Transmitting...");
  int state = radio.transmit(pkt.raw, SENSOR_PACKET_SIZE);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX OK!");
  } else {
    Serial.printf("TX failed: %d\n", state);
  }
}

// ============================================================================
// Power Management - Light Sleep
// ============================================================================

/**
 * @brief Enter light sleep (ESP32)
 *
 * ESP32 light sleep:
 * - Current: ~100 µA
 * - RAM preserved
 * - Wake via: timer, GPIO, touch
 */
void enter_light_sleep(uint32_t sleep_ms) {
  Serial.printf("Sleeping for %lu ms\n", sleep_ms);
  Serial.flush();

  // Turn off radio
  radio.sleep();

#ifdef USE_OLED
  display.ssd1306_command(SSD1306_DISPLAYOFF);
#endif

  // Configure wake timer
  esp_sleep_enable_timer_wakeup(sleep_ms * 1000ULL);

  // Enter light sleep
  Serial.println("Goodnight!");
  Serial.flush();
  delay(100);

  esp_light_sleep_start();

  // After wake: continues from loop()
}
