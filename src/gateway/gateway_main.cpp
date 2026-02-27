/**
 * @file gateway_main.cpp
 * @brief Interrupt-driven gateway with MOSFET power control
 * @version 1.0
 * @date 2026-02-25
 *
 * Gateway behavior:
 * - Deep sleep until LoRa packet (DIO0 interrupt)
 * - Wake, forward to Blues, power off Notecard, sleep
 * - Quiet hours: 22:00-08:00 (deep sleep)
 * - Env var sync: Every 6 hours
 *
 * Hardware:
 * - Pololu high-side MOSFET for Notecard power
 * - 47-100 µF bulk capacitance recommended
 * - nRF52840 System OFF mode (~6 µA sleep)
 */

// Debug output: Set to 0 to disable all Serial output (reduces code size)
#define DEBUG 0

#include <Arduino.h>
#include <RadioLib.h>
#include <blues-minimal-i2c.h>
#include "protocol.h"
#include "radio_config.h"
#include "config_types.h"
#include "debug.h"

// ============================================================================
// Pin Definitions (Adafruit Feather nRF52840)
// ============================================================================

// RFM95W LoRa radio (SPI) - use definitions from radio_config.h
#include "radio_config.h"

// Blues Notecard power control
#define NOTECARD_POWER_PIN  5u          // P0.29 (D5 on Feather)
#define NOTECARD_BOOT_MS    1000u       // Notecard boot time (ms)

// LEDs
#define LED_GREEN           PIN_LED1    // Red LED
#define LED_BLUE            PIN_LED2    // Blue LED

// ============================================================================
// RadioLib Instance
// ============================================================================

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);

// ============================================================================
// Globals (preserved in RAM during deep sleep)
// ============================================================================

/** Packet received flag (set by ISR) */
volatile bool g_packet_received = false;

/** RX buffer */
uint8_t g_rx_buffer[MAX_PACKET_SIZE];
uint8_t g_rx_length = 0;

/** Last environment variable sync time */
uint32_t g_last_env_sync = 0;

/** Environment sync interval (6 hours) */
#define ENV_SYNC_INTERVAL_MS  (6UL * 60UL * 60UL * 1000UL)

/** Gateway configuration */
gateway_config_t g_gateway_config;

/** Config version (from Blues env vars) */
uint8_t g_config_version = 0;

/** Flag: Blues Notecard found */
bool g_has_blues = false;

// ============================================================================
// Forward Declarations
// ============================================================================

void init_radio(void);
void init_blues(void);
void load_gateway_config(void);

void notecard_power_on(void);
void notecard_power_off(void);
void forward_to_blues(sensor_packet_t* pkt);
void check_environment_variables(void);

uint8_t get_current_hour(void);
bool is_quiet_hours(void);
void enter_deep_sleep(void);
void enter_deep_sleep_until(uint8_t wake_hour);

// ============================================================================
// Interrupt Service Routine
// ============================================================================

/**
 * @brief LoRa RX interrupt handler (DIO0 pin)
 * 
 * Called when RFM95W receives a packet.
 * Runs in interrupt context - keep it fast!
 */
void onLoRaInterrupt() {
  g_packet_received = true;
}

// ============================================================================
// Power Control Functions
// ============================================================================

/**
 * @brief Power on Blues Notecard via MOSFET
 */
void notecard_power_on() {
  digitalWrite(NOTECARD_POWER_PIN, HIGH);
  delay(NOTECARD_BOOT_MS);  // Wait for Notecard boot
}

/**
 * @brief Power off Blues Notecard via MOSFET
 */
void notecard_power_off() {
  digitalWrite(NOTECARD_POWER_PIN, LOW);
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
  // Serial for debug
  Serial.begin(SERIAL_BAUD);
  delay(2000u);  // Wait for serial

  DEBUG_SERIAL.println("\n*** LoRa-to-Blues Gateway Online ***");
  DEBUG_SERIAL.printf("Board: Adafruit Feather nRF52840\n");
  DEBUG_SERIAL.printf("Interrupt-driven gateway\n");

  // Initialize power control pin
  pinMode(NOTECARD_POWER_PIN, OUTPUT);

  // Initialize LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);

  // Power ON Notecard (initial boot only)
  DEBUG_SERIAL.println("Powering on Notecard...");
  notecard_power_on();

  // Initialize Blues (one-shot at boot)
  init_blues();

  if (g_has_blues) {
    DEBUG_SERIAL.println("Blues Notecard initialized");

    // Check environment variables at boot
    DEBUG_SERIAL.println("Checking environment variables...");
    check_environment_variables();
  } else {
    DEBUG_SERIAL.println("Blues Notecard NOT found");
  }

  // Power OFF Notecard (save battery!)
  DEBUG_SERIAL.println("Powering off Notecard (sleep mode)");
  notecard_power_off();

  // Initialize LoRa radio
  init_radio();

  // Load saved configuration
  load_gateway_config();

  DEBUG_SERIAL.println("*** Gateway ready - sleeping until packet ***\n");

  // Blink blue LED to indicate startup
  digitalWrite(LED_BLUE, HIGH);
  delay(100);
  digitalWrite(LED_BLUE, LOW);

  // Enter deep sleep (wake on DIO0 interrupt)
  enter_deep_sleep();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
  // Check packet flag (set by ISR)
  if (g_packet_received) {
    g_packet_received = false;

    DEBUG_SERIAL.println("Packet received - processing");

    // Blink blue LED (activity)
    digitalWrite(LED_BLUE, HIGH);

    // Read packet from radio
    int state = radio.readData(g_rx_buffer, g_rx_length);

    if (state == RADIOLIB_ERR_NONE && g_rx_length >= SENSOR_PACKET_SIZE) {
      sensor_packet_t* sensor_pkt = (sensor_packet_t*)g_rx_buffer;

      // Verify CRC
      if (protocol_verify_crc(g_rx_buffer, g_rx_length)) {
        DEBUG_SERIAL.printf("Valid sensor packet from node %d (RSSI: %d dBm)\n",
                      sensor_pkt->node_id, sensor_pkt->rssi);

        // Power ON Notecard
        notecard_power_on();

        // Forward to Blues
        if (g_has_blues) {
          forward_to_blues(sensor_pkt);
        }

        // Power OFF Notecard
        notecard_power_off();

      } else {
        DEBUG_SERIAL.println("CRC check failed");
      }

    } else {
      DEBUG_SERIAL.printf("Packet read failed: %d (length: %d)\n", state, g_rx_length);
    }

    digitalWrite(LED_BLUE, LOW);

    // Check quiet hours (22:00-08:00)
    if (is_quiet_hours()) {
      DEBUG_SERIAL.println("Quiet hours (22:00-08:00) - sleeping until 08:00");
      enter_deep_sleep_until(8u);
    }

    // Resume listening
    radio.startReceive();

    // Back to deep sleep (wake on next packet)
    enter_deep_sleep();
  }

  // Check environment variables (every 6 hours)
  if (millis() - g_last_env_sync > ENV_SYNC_INTERVAL_MS) {
    DEBUG_SERIAL.println("Environment variable sync");

    // Power ON Notecard
    notecard_power_on();

    if (g_has_blues) {
      check_environment_variables();
    }

    // Power OFF Notecard
    notecard_power_off();

    g_last_env_sync = millis();
  }
}

// ============================================================================
// Initialization Functions
// ============================================================================

void init_radio(void) {
  DEBUG_SERIAL.println("Initializing LoRa radio...");

  // Use frequency from stored config (or default if not set)
  float frequency = g_gateway_config.lora_frequency;

  // Validate frequency, fall back to default if invalid
  if (!radio_frequency_valid(frequency)) {
    DEBUG_SERIAL.printf("Invalid frequency %.1f in config, using default %.1f MHz\n",
                  frequency, LORA_FREQUENCY_DEFAULT);
    frequency = LORA_FREQUENCY_DEFAULT;
  }

  // Initialize RadioLib with SX1276 (RFM95W)
  int state = radio.begin(
    frequency,                  // Frequency in MHz (from config or default)
    LORA_BANDWIDTH,             // Bandwidth in kHz (125.0)
    LORA_SPREADING_FACTOR,      // Spreading factor (10)
    LORA_CODING_RATE,           // Coding rate (5 = 4/5)
    LORA_SYNC_WORD,             // Sync word (0x12 for private network)
    LORA_TX_POWER,              // TX power in dBm (14)
    LORA_PREAMBLE_LENGTH,       // Preamble length (8)
    LORA_CRC_ENABLED            // CRC enabled
  );

  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_SERIAL.printf("Radio initialized successfully\n");
    DEBUG_SERIAL.printf("  Frequency: %.1f MHz\n", frequency);
    DEBUG_SERIAL.printf("  Spreading Factor: %d\n", LORA_SPREADING_FACTOR);
    DEBUG_SERIAL.printf("  Bandwidth: %d kHz\n", LORA_BANDWIDTH);
    DEBUG_SERIAL.printf("  TX Power: %d dBm\n", LORA_TX_POWER);
  } else {
    DEBUG_SERIAL.printf("Radio initialization failed: %d\n", state);
    while (1) {
      digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
      delay(200);
    }
  }

  // Set RX interrupt (DIO0 pin)
  radio.setDio0Action(onLoRaInterrupt, RISING);

  // Start receiving
  radio.startReceive();
  DEBUG_SERIAL.println("Radio in RX mode");
}

void init_blues(void) {
  DEBUG_SERIAL.println("Initializing Blues Notecard...");

  Wire.begin();
  Wire.setClock(100000u);  // 100 kHz I2C

  RAK_BLUES rak_blues;

  if (rak_blues.start_req((char *)"card.version")) {
    char blues_response[256];
    if (rak_blues.send_req(blues_response, sizeof(blues_response))) {
      g_has_blues = true;
      DEBUG_SERIAL.println("Blues Notecard found");

      // Configure for minimum power (periodic sync)
      rak_blues.start_req((char *)"hub.set");
      rak_blues.add_string_entry((char *)"product", (char *)"com.hummingbird.gateway:main");
      rak_blues.add_string_entry((char *)"mode", (char *)"minimum");
      rak_blues.add_int32_entry((char *)"seconds", 3600);  // 1 hour default
      rak_blues.send_req();

    } else {
      DEBUG_SERIAL.println("Blues Notecard not responding");
    }
  } else {
    DEBUG_SERIAL.println("Blues Notecard not found");
  }

  if (!g_has_blues) {
    DEBUG_SERIAL.println("WARNING: No Blues Notecard - data will not be forwarded");
  }
}

void load_gateway_config(void) {
  // Load gateway configuration from Flash
  // TODO: Implement Flash read

  // For now, use defaults
  g_gateway_config.listen_start_hour = 8u;
  g_gateway_config.listen_end_hour = 22u;
  g_gateway_config.wake_interval_minutes = 360u;  // 6 hours
  g_gateway_config.global_config_version = 1u;
  g_gateway_config.lora_frequency = LORA_FREQUENCY_DEFAULT;

  DEBUG_SERIAL.printf("Gateway config loaded (version %d, frequency %.1f MHz)\n",
                g_gateway_config.global_config_version, g_gateway_config.lora_frequency);
}

// ============================================================================
// Environment Variables
// ============================================================================

void check_environment_variables(void) {
  DEBUG_SERIAL.println("Checking Blues environment variables...");

  RAK_BLUES rak_blues;

  if (rak_blues.start_req((char *)"hub.get")) {
    rak_blues.add_bool_entry((char *)"env", true);

    char blues_response[2048];
    if (rak_blues.send_req(blues_response, sizeof(blues_response))) {
      DEBUG_SERIAL.printf("Env response: %s\n", blues_response);

      // Parse JSON response
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, blues_response);

      if (error) {
        DEBUG_SERIAL.printf("JSON parse error: %s\n", error.c_str());
        return;
      }

      JsonObject env = doc["env"];

      // Check for frequency override
      if (env.containsKey("lora_frequency")) {
        float new_freq = env["lora_frequency"];

        // Validate frequency is within legal bounds
        if (radio_frequency_valid(new_freq)) {
          DEBUG_SERIAL.printf("Applying frequency from env: %.1f MHz\n", new_freq);

          // Update radio frequency
          radio.setFrequency(new_freq);

          // Store in global config for persistence
          g_gateway_config.lora_frequency = new_freq;
        } else {
          DEBUG_SERIAL.printf("Invalid frequency %.1f MHz (must be %.1f-%.1f)\n",
                        new_freq, LORA_FREQUENCY_MIN, LORA_FREQUENCY_MAX);
        }
      }

      // TODO: Parse other environment variables
      // gateway_listen_start, gateway_listen_end, etc.

    } else {
      DEBUG_SERIAL.println("Env var read failed");
    }
  }
}

// ============================================================================
// Packet Forwarding
// ============================================================================

void forward_to_blues(sensor_packet_t* pkt) {
  DEBUG_SERIAL.println("Forwarding to Blues...");

  RAK_BLUES rak_blues;

  // Build note for events.qo
  if (rak_blues.start_req((char *)"note.add")) {
    rak_blues.add_string_entry((char *)"file", (char *)"events.qo");
    rak_blues.add_bool_entry((char *)"sync", true);

    // Add sensor data
    rak_blues.add_nested_int32_entry((char *)"body", (char *)"node_id", pkt->node_id);
    rak_blues.add_nested_float_entry((char *)"body", (char *)"value_1", pkt->payload.temperature);
    rak_blues.add_nested_float_entry((char *)"body", (char *)"value_2", pkt->payload.humidity);
    rak_blues.add_nested_int32_entry((char *)"body", (char *)"battery_mv", pkt->payload.battery_mv);
    rak_blues.add_nested_int32_entry((char *)"body", (char *)"rssi", pkt->rssi);
    rak_blues.add_nested_int32_entry((char *)"body", (char *)"config_version", pkt->config_version);

    if (!rak_blues.send_req()) {
      DEBUG_SERIAL.println("Blues send failed");
    } else {
      DEBUG_SERIAL.println("Blues send successful");
    }
  }
}

// ============================================================================
// Time and Sleep Functions
// ============================================================================

/**
 * @brief Get current hour from RTC
 * 
 * TODO: Implement RTC read for nRF52840
 * For now, returns dummy value for testing
 */
uint8_t get_current_hour(void) {
  // TODO: Read from nRF52840 RTC
  return 12u;  // Noon (inside active window for testing)
}

/**
 * @brief Check if currently in quiet hours (22:00-08:00)
 */
bool is_quiet_hours(void) {
  uint8_t hour = get_current_hour();
  return (hour < 8u || hour >= 22u);
}

/**
 * @brief Enter deep sleep (System OFF mode)
 *
 * Wake sources:
 * - DIO0 interrupt (LoRa packet)
 * - Any GPIO interrupt
 */
void enter_deep_sleep(void) {
  DEBUG_SERIAL.println("Entering deep sleep...");

  // Turn off peripherals
  radio.sleep();
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);

  // TODO: Implement nRF52840 System OFF
  // For now, use simple delay (NOT low-power!)

  // Enter deep sleep (placeholder - implement properly)
  // delay(1000);  // Don't actually sleep yet

  // WARNING: Proper deep sleep requires:
  // 1. Configure GPIO wake (DIO0)
  // 2. Call sd_app_evt_wait() or NRF_POWER->SYSTEMOFF = 1
  // 3. RAM retention enabled
}

/**
 * @brief Enter deep sleep until specified hour
 *
 * @param wake_hour Hour to wake (0-23)
 */
void enter_deep_sleep_until(uint8_t wake_hour) {
  DEBUG_SERIAL.printf("Entering deep sleep until %02d:00\n", wake_hour);

  // Turn off peripherals
  radio.sleep();
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);

  // TODO: Implement nRF52840 RTC alarm wake
  // For now, use simple delay (NOT low-power!)

  // Calculate sleep duration
  uint8_t current_hour = get_current_hour();
  uint32_t sleep_hours = (wake_hour - current_hour + 24u) % 24u;
  if (sleep_hours == 0u) sleep_hours = 24u;

  uint32_t sleep_ms = sleep_hours * 3600000UL;

  DEBUG_SERIAL.printf("Sleeping for %lu hours (%lu ms)\n", sleep_hours, sleep_ms);

  // WARNING: This is NOT low-power! Replace with proper deep sleep.
  // For nRF52840, use: sd_app_evt_wait() with RTC alarm

  // Enter deep sleep (placeholder - implement properly)
  delay(sleep_ms);

  // Wake up - reinitialize radio
  DEBUG_SERIAL.println("Wake up!");
  init_radio();
}
