/**
 * @file node_main.cpp
 * @brief Hello World sensor node for end-to-end testing
 * @version 1.0
 * @date 2026-02-27
 *
 * Simple test node that:
 * - Sends "hello world" sensor data every 30 seconds
 * - Uses fake temperature/humidity values
 * - Reports battery voltage
 * - Works with Adafruit Feather M0 RFM9x
 *
 * Upload to Blues Dashboard to see data arrive in real-time.
 */

#include <Arduino.h>
#include <RadioLib.h>
#include "protocol.h"
#include "radio_config.h"
#include "pins.h"

// ============================================================================
// Configuration
// ============================================================================

/**
 * Node ID (1-255)
 * Must be unique for each node in your network
 * Gateway is reserved as node ID 0
 */
#define NODE_ID             1

/**
 * Transmission interval (seconds)
 * For testing: 30 seconds
 * For deployment: 300-1800 seconds (5-30 minutes)
 */
#define TX_INTERVAL_SEC     30

/**
 * Fake sensor values for testing
 * Replace with actual sensor reads in production
 */
#define FAKE_TEMPERATURE    22.5f       // Celsius
#define FAKE_HUMIDITY       45.0f       // Percent

// ============================================================================
// RadioLib Instance
// ============================================================================

// RFM95W (SX1276) using pins from pins.h
SX1276 radio = new Module(LORA_CS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);

// ============================================================================
// Globals
// ============================================================================

/** Transmission counter */
uint32_t g_tx_count = 0;

/** Last transmission time */
uint32_t g_last_tx = 0;

/** Node configuration version */
uint8_t g_config_version = CONFIG_VERSION_INITIAL;

// ============================================================================
// Forward Declarations
// ============================================================================

void init_radio(void);
void read_sensors(sensor_payload_t* payload);
void send_sensor_data(void);
uint16_t read_battery_mv(void);

// ============================================================================
// Setup
// ============================================================================

void setup() {
  // Serial for debug
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Wait for serial

  Serial.println("\n*** Node Hello World ***");
  Serial.printf("Board: Adafruit Feather M0 RFM9x\n");
  Serial.printf("Node ID: %d\n", NODE_ID);
  Serial.printf("TX Interval: %d seconds\n", TX_INTERVAL_SEC);

  // Initialize radio
  init_radio();

  // Blink to indicate startup
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);

  Serial.println("*** Node ready - sending first packet ***\n");

  // Send first packet immediately
  send_sensor_data();

  g_last_tx = millis();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
  // Check if it's time to transmit
  if (millis() - g_last_tx >= (uint32_t)TX_INTERVAL_SEC * 1000UL) {
    send_sensor_data();
    g_last_tx = millis();
  }

  // Small delay to avoid watchdog
  delay(100);
}

// ============================================================================
// Radio Initialization
// ============================================================================

void init_radio(void) {
  Serial.println("Initializing LoRa radio...");

  // Use frequency from config (can be overridden by Blues env var on gateway)
  float frequency = LORA_FREQUENCY_DEFAULT;

  int state = radio.begin(
    frequency,                  // MHz
    LORA_BANDWIDTH,             // kHz
    LORA_SPREADING_FACTOR,      // SF7-SF12
    LORA_CODING_RATE,           // 5-8
    LORA_SYNC_WORD,             // 0x12 (private)
    LORA_TX_POWER,              // dBm
    LORA_PREAMBLE_LENGTH,       // symbols
    LORA_CRC_ENABLED            // true/false
  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.printf("Radio initialized successfully\n");
    Serial.printf("  Frequency: %.1f MHz\n", frequency);
    Serial.printf("  Spreading Factor: %d\n", LORA_SPREADING_FACTOR);
    Serial.printf("  Bandwidth: %d kHz\n", LORA_BANDWIDTH);
    Serial.printf("  TX Power: %d dBm\n", LORA_TX_POWER);
  } else {
    Serial.printf("ERROR: Radio initialization failed: %d\n", state);
    Serial.println("Check wiring and restart");
    while (1) {
      digitalWrite(PIN_LED, !digitalRead(PIN_LED));
      delay(200);
    }
  }
}

// ============================================================================
// Sensor Reading
// ============================================================================

/**
 * @brief Read sensor values (fake for testing)
 *
 * In production, replace with actual sensor reads:
 * - BME280/BME680 for temp/humidity
 * - Analog read for battery
 * - Digital input for buttons/motion
 */
void read_sensors(sensor_payload_t* payload) {
  // Fake sensor values for hello world test
  payload->temperature = FAKE_TEMPERATURE + (float)g_tx_count * 0.1f;
  payload->humidity = FAKE_HUMIDITY;
  payload->battery_mv = read_battery_mv();
  payload->sensor_flags = 0;

  // Check for low battery (example threshold)
  if (payload->battery_mv < 3500) {
    payload->sensor_flags |= SENSOR_FLAG_BATTERY_LOW;
  }

  Serial.printf("Sensors: temp=%.1fC, humidity=%.1f%%, battery=%dmV\n",
                payload->temperature, payload->humidity, payload->battery_mv);
}

/**
 * @brief Read battery voltage
 *
 * Feather M0 has battery voltage on A7 (divided by 2)
 * Returns voltage in millivolts
 */
uint16_t read_battery_mv(void) {
  // Feather M0: Battery voltage is divided by 2 before ADC
  // ADC is 10-bit (0-1023) with 3.3V reference
  // Formula: Vbat = (ADC / 1023) * 3.3V * 2

  int adc = analogRead(PIN_BATTERY);
  float voltage = (adc / 1023.0f) * 3.3f * 2.0f;
  return (uint16_t)(voltage * 1000.0f);
}

// ============================================================================
// Packet Transmission
// ============================================================================

void send_sensor_data(void) {
  Serial.printf("\n[TX %lu] Sending sensor data...\n", ++g_tx_count);

  // Read sensors
  sensor_payload_t payload;
  read_sensors(&payload);

  // Build sensor packet
  packet_t pkt;
  protocol_build_sensor_packet(&pkt, NODE_ID, g_config_version, &payload);

  // Transmit
  Serial.println("Transmitting packet...");
  int state = radio.transmit(pkt.raw, SENSOR_PACKET_SIZE);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX successful!");
    digitalWrite(PIN_LED, HIGH);
    delay(50);
    digitalWrite(PIN_LED, LOW);
  } else {
    Serial.printf("TX failed: %d\n", state);
  }

  // Print packet info for debug
  Serial.printf("Packet size: %d bytes\n", SENSOR_PACKET_SIZE);
  Serial.printf("Node ID: %d, Config version: %d\n", NODE_ID, g_config_version);
}

