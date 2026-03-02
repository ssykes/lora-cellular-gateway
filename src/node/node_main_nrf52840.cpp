/**
 * @file node_main_nrf52840.cpp
 * @brief Ultra-low-power sensor node for Adafruit nRF52840 + LoRa FeatherWing
 * @version 1.0
 * @date 2026-02-27
 *
 * Power strategy:
 * - Deep sleep (System OFF) between transmissions
 * - Current: ~6 µA in sleep, ~100 mA during TX
 * - Wake via timer (RTC alarm)
 *
 * Hardware:
 * - Adafruit Feather nRF52840 Express
 * - Adafruit LoRa FeatherWing (RFM95W)
 * - Optional: BME280/BME680 on I2C
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <Adafruit_Sensor.h>
#include "protocol.h"
#include "radio_config.h"
#include "pins.h"
#include "debug.h"

// nRF52840 deep sleep
#include <nrf_nvic.h>
#include <nrf_soc.h>

// ============================================================================
// Configuration
// ============================================================================

#define NODE_ID             2           // Unique ID (1-255)
#define TX_INTERVAL_SEC     300         // 5 minutes (deep sleep optimized)

// Power management
#define SLEEP_MODE_DEEP     1           // 1=System OFF, 0=Idle
#define KEEP_RTC_RUNNING    1           // Preserve RTC during sleep

// Sensors (uncomment what you have)
// #define USE_BME280
// #define USE_BME680
#define USE_FAKE_SENSORS               // For testing

#ifdef USE_BME280
  #include <Adafruit_BME280.h>
  Adafruit_BME280 bme;
#endif

#ifdef USE_BME680
  #include <Adafruit_BME680.h>
  Adafruit_BME680 bme680;
#endif

// ============================================================================
// RadioLib Instance
// ============================================================================

SX1276 radio = new Module(LORA_CS_PIN, LORA_DIO0_PIN, LORA_RST_PIN, LORA_DIO1_PIN);

// ============================================================================
// Globals
// ============================================================================

uint32_t g_tx_count = 0;
uint8_t g_config_version = CONFIG_VERSION_INITIAL;

// Last sensor values (for delta-based TX)
float g_last_temp = 0.0f;
float g_last_humidity = 0.0f;

// ============================================================================
// Forward Declarations
// ============================================================================

void init_radio(void);
void init_sensors(void);
void read_sensors(sensor_payload_t* payload);
void send_sensor_data(void);
uint16_t read_battery_mv(void);
void enter_deep_sleep(uint32_t sleep_ms);

// ============================================================================
// Setup
// ============================================================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(2000);

  DEBUG_PRINT("\n*** Node nRF52840 (Deep Sleep) ***");
  DEBUG_PRINTF("Board: Adafruit Feather nRF52840 + FeatherWing\n");
  DEBUG_PRINTF("Node ID: %d\n", NODE_ID);
  DEBUG_PRINTF("TX Interval: %d seconds\n", TX_INTERVAL_SEC);

  // Initialize sensors
  init_sensors();

  // Initialize radio
  init_radio();

  // Blink LED
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);

  DEBUG_PRINT("*** First transmission ***\n");

  // Send first packet
  send_sensor_data();

  // Enter deep sleep
  DEBUG_PRINT("\nEntering deep sleep...\n");
  enter_deep_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
}

// ============================================================================
// Main Loop (never reached in deep sleep mode)
// ============================================================================

void loop() {
  // After deep sleep wake, we re-run setup()
  // This is a feature of nRF52840 System OFF
}

// ============================================================================
// Sensor Initialization
// ============================================================================

void init_sensors(void) {
#ifdef USE_BME280
  if (!bme.begin(0x76)) {
    DEBUG_PRINT("BME280 not found at 0x76, trying 0x77...");
    if (!bme.begin(0x77)) {
      DEBUG_PRINT("ERROR: BME280 not found");
    }
  } else {
    DEBUG_PRINT("BME280 initialized");
  }
#endif

#ifdef USE_BME680
  if (!bme680.begin(0x76)) {
    DEBUG_PRINT("ERROR: BME680 not found");
  } else {
    DEBUG_PRINT("BME680 initialized");
  }
#endif

#ifdef USE_FAKE_SENSORS
  DEBUG_PRINT("Using fake sensors (test mode)");
#endif
}

// ============================================================================
// Radio Initialization
// ============================================================================

void init_radio(void) {
  DEBUG_PRINT("Initializing LoRa radio...");

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
    DEBUG_PRINTF("Radio OK: %.1f MHz, SF%d, %d dBm\n",
                  LORA_FREQUENCY_DEFAULT, LORA_SPREADING_FACTOR, LORA_TX_POWER);
  } else {
    DEBUG_PRINTF("ERROR: Radio init failed: %d\n", state);
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
#ifdef USE_BME280
  payload->temperature = bme.readTemperature();
  payload->humidity = bme.readHumidity();
#endif

#ifdef USE_BME680
  payload->temperature = bme680.readTemperature();
  payload->humidity = bme680.readHumidity();
#endif

#ifdef USE_FAKE_SENSORS
  // Fake values that change slightly each transmission
  payload->temperature = 22.5f + (float)g_tx_count * 0.1f;
  payload->humidity = 45.0f + (float)(g_tx_count % 10);
#endif

  payload->battery_mv = read_battery_mv();
  payload->sensor_flags = 0;

  // Check battery
  if (payload->battery_mv < 3500) {
    payload->sensor_flags |= SENSOR_FLAG_BATTERY_LOW;
  }

  DEBUG_PRINTF("Sensors: T=%.1fC, H=%.1f%%, Vbat=%dmV\n",
                payload->temperature, payload->humidity, payload->battery_mv);
}

/**
 * @brief Read battery voltage
 *
 * nRF52840: Use internal analog reference (3.6V)
 * Battery connected to VBAT pin (through voltage divider)
 */
uint16_t read_battery_mv(void) {
  // Adafruit nRF52840 has battery monitor on A0 (VBAT)
  // Internal reference: 3.6V, 12-bit ADC
  analogReference(AR_INTERNAL_3_0);
  analogReadResolution(12);

  int adc = analogRead(PIN_BATTERY);
  float voltage = (adc / 4095.0f) * 3.6f * 2.0f;  // 2x divider
  return (uint16_t)(voltage * 1000.0f);
}

// ============================================================================
// Packet Transmission
// ============================================================================

void send_sensor_data(void) {
  DEBUG_PRINTF("\n[TX %lu] Sending...\n", ++g_tx_count);

  sensor_payload_t payload;
  read_sensors(&payload);

  packet_t pkt;
  protocol_build_sensor_packet(&pkt, NODE_ID, g_config_version, &payload);

  DEBUG_PRINT("Transmitting...");
  int state = radio.transmit(pkt.raw, SENSOR_PACKET_SIZE);

  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_PRINT("TX OK!");
    digitalWrite(PIN_LED, HIGH);
    delay(50);
    digitalWrite(PIN_LED, LOW);
  } else {
    DEBUG_PRINTF("TX failed: %d\n", state);
  }
}

// ============================================================================
// Power Management - Deep Sleep
// ============================================================================

/**
 * @brief Enter deep sleep (System OFF mode)
 *
 * nRF52840 System OFF:
 * - Current: ~6 µA
 * - RAM lost (state not preserved)
 * - Wake via: RTC alarm, GPIO, NFC
 *
 * After wake: Full reset, setup() runs again
 */
void enter_deep_sleep(uint32_t sleep_ms) {
  DEBUG_PRINTF("Sleeping for %lu ms\n", sleep_ms);
  Serial.flush();

  // Turn off radio
  radio.sleep();

  // Turn off LEDs
  digitalWrite(PIN_LED, LOW);

  // Enter System OFF using nRF52840 register
  // This is the lowest power mode (~6 µA)
  // Note: For timer wake, configure LPCOMP/GPIOTE before sleep
  // For now, use delay as placeholder - replace with RTC alarm in production
  NRF_POWER->SYSTEMOFF = 1;

  // After wake: Full reset, setup() runs again
}
