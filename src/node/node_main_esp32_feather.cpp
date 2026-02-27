/**
 * @file node_main_esp32_feather.cpp
 * @brief Sensor node for Adafruit ESP32 Feather + LoRa FeatherWing
 * @version 1.0
 * @date 2026-02-27
 *
 * Power strategy:
 * - Light sleep between transmissions
 * - Current: ~50 µA in sleep, ~120 mA during TX
 * - Wake via timer (RTC)
 *
 * Hardware:
 * - Adafruit ESP32 Feather
 * - Adafruit LoRa FeatherWing (RFM95W)
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "protocol.h"
#include "radio_config.h"
#include "pins.h"

// ============================================================================
// Configuration
// ============================================================================

#define NODE_ID             4           // Unique ID (1-255)
#define TX_INTERVAL_SEC     300         // 5 minutes

// ============================================================================
// RadioLib Instance
// ============================================================================

SX1276 radio = new Module(LORA_CS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);

// ============================================================================
// Globals
// ============================================================================

uint32_t g_tx_count = 0;
uint8_t g_config_version = CONFIG_VERSION_INITIAL;

// ============================================================================
// Forward Declarations
// ============================================================================

void init_radio(void);
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

  Serial.println("\n*** Node ESP32 Feather ***");
  Serial.printf("Board: Adafruit ESP32 Feather + FeatherWing\n");
  Serial.printf("Node ID: %d\n", NODE_ID);
  Serial.printf("TX Interval: %d seconds\n", TX_INTERVAL_SEC);

  init_radio();

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);

  Serial.println("*** First transmission ***\n");
  send_sensor_data();

  Serial.println("\nSleeping...\n");
  enter_light_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
}

// ============================================================================
// Main Loop (wake from sleep continues here)
// ============================================================================

void loop() {
  send_sensor_data();
  enter_light_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
}

// ============================================================================
// Radio Initialization
// ============================================================================

void init_radio(void) {
  Serial.println("Initializing LoRa radio...");

  int state = radio.begin(
    LORA_FREQUENCY_DEFAULT,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODING_RATE,
    LORA_SYNC_WORD,
    LORA_TX_POWER,
    LORA_PREAMBLE_LENGTH,
    LORA_CRC_ENABLED
  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.printf("Radio OK: %.1f MHz, SF%d, %d dBm\n",
                  LORA_FREQUENCY_DEFAULT, LORA_SPREADING_FACTOR, LORA_TX_POWER);
  } else {
    Serial.printf("ERROR: Radio init failed: %d\n", state);
    while (1) {
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
      delay(200);
    }
  }
}

// ============================================================================
// Sensor Reading
// ============================================================================

void read_sensors(sensor_payload_t* payload) {
  // Fake values for testing
  payload->temperature = 22.0f + (float)g_tx_count * 0.1f;
  payload->humidity = 48.0f + (float)(g_tx_count % 8);
  payload->battery_mv = read_battery_mv();
  payload->sensor_flags = 0;

  if (payload->battery_mv < 3500) {
    payload->sensor_flags |= SENSOR_FLAG_BATTERY_LOW;
  }

  Serial.printf("Sensors: T=%.1fC, H=%.1f%%, Vbat=%dmV\n",
                payload->temperature, payload->humidity, payload->battery_mv);
}

/**
 * @brief Read battery voltage
 *
 * ESP32 Feather: Battery on A13 (through divider)
 * Note: A13 maps to GPIO35 on ESP32
 */
uint16_t read_battery_mv(void) {
  // ESP32 Feather: Battery on GPIO35 (A13)
  int adc = analogRead(PIN_BATTERY);
  float voltage = (adc / 4095.0f) * 3.3f * 2.0f;  // Approximate
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
    digitalWrite(PIN_LED, HIGH);
    delay(50);
    digitalWrite(PIN_LED, LOW);
  } else {
    Serial.printf("TX failed: %d\n", state);
  }
}

// ============================================================================
// Power Management - Light Sleep
// ============================================================================

void enter_light_sleep(uint32_t sleep_ms) {
  Serial.printf("Sleeping for %lu ms\n", sleep_ms);
  Serial.flush();

  radio.sleep();

  // Configure wake timer
  esp_sleep_enable_timer_wakeup(sleep_ms * 1000ULL);

  Serial.println("Goodnight!");
  Serial.flush();
  delay(100);

  esp_light_sleep_start();
}
