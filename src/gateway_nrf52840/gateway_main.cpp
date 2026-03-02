/**
 * @file gateway_main.cpp
 * @brief LoRa-to-Blues Gateway for Adafruit Feather nRF52840 Express
 * @version 5.0 - With Attention Pin Support
 *
 * Hardware:
 * - Adafruit Feather nRF52840 Express
 * - LoRa FeatherWing (RFM95W)
 * - Blues Notecarrier-F
 * - Blues Notecard (NOTE-WBNAN)
 *
 * Power Management:
 * - Current: Always-on, USB powered for testing
 * - Phase 3: Deep sleep with ATTN pin wake-up
 *
 * Attention Pin (ATTN):
 * - Wire N_ATTN (Notecarrier F) to GPIO pin (e.g., D2, D3)
 * - Notecard interrupts gateway when cloud has data
 * - Gateway sleeps (~6 µA), wakes on ATTN interrupt
 * - Handles: env var updates, inbound Notes, commands
 *
 * TODO Phase 3 - Power Management:
 * - Add MOSFET circuit for Notecard power control (D5 → gate)
 * - Implement nRF52840 System OFF sleep
 * - Add RTC alarm for periodic wake
 * - Add battery voltage monitoring
 * - Add 47-100 µF bulk capacitor on Notecard VCC
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Notecard.h>
#include "protocol.h"
#include "env_vars.h"
#include "radio_config.h"
#include "debug.h"

// ============================================================================
// Pin Definitions
// ============================================================================

// YOUR PRODUCT ID
#define PRODUCT_ID "net.pacbell.ssykes11:lora_cellular_field_gateway"

// FeatherWing pins (working configuration)
#define LORA_CS               10
#define LORA_RST              11
#define LORA_DIO0             6

// Notecard pins
#define NOTECARD_POWER_PIN    5         // TODO Phase 3: MOSFET gate for power control
#define NOTECARD_ATTN_PIN     2         // N_ATTN interrupt pin (wire from Notecarrier F)

// Status LED
#define LED_PIN               13

// ============================================================================
// Global Instances
// ============================================================================

// Radio instance (RFM95W = SX1276)
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, RADIOLIB_NC);

// Blues Notecard instance (OFFICIAL library)
Notecard notecard;

// Gateway configuration (from Notehub environment variables)
gateway_config_t g_config;

// RX buffer for LoRa packets
uint8_t g_rx_buffer[30];
uint8_t g_rx_length = 0;

// ATTN pin state
volatile bool g_attn_fired = false;   // Set by ISR when ATTN pin triggers
bool g_attn_enabled = false;          // true if ATTN interrupt is armed

// ============================================================================
// Forward Declarations
// ============================================================================

void init_radio();
void init_blues();
void forward_to_blues(sensor_packet_t* pkt);
void handle_attn_interrupt();
void arm_attn_interrupt(uint32_t seconds);
void clear_attn_interrupt();

// ============================================================================
// Helper Functions (Debug)
// ============================================================================

/**
 * @brief Blink LED to indicate successful transmission (debug only)
 *
 * Visual feedback for testing - removed in production builds.
 */
#if DEBUG == 1
void blink_tx_success() {
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}
#endif

// ============================================================================
// Attention Pin Interrupt Service Routine
// ============================================================================

/**
 * @brief ATTN pin interrupt handler
 *
 * Called when Notecard pulls N_ATTN pin high.
 * Sets flag for main loop to handle (don't do heavy work in ISR!).
 *
 * Note: nRF52840 supports interrupt on any GPIO pin.
 * For ESP32, would need IRAM_ATTR decorator.
 */
void attn_isr() {
  g_attn_fired = true;
}

// ============================================================================
// Attention Pin Management Functions
// ============================================================================

/**
 * @brief Arm the Notecard attention interrupt
 *
 * Configures Notecard to assert N_ATTN pin when:
 * - Environment variables are updated
 * - Inbound Note is received
 * - Optional timer expires (if seconds > 0)
 *
 * @param seconds Timer duration (0 = no timer, event-only)
 */
void arm_attn_interrupt(uint32_t seconds = 0) {
  J *req = notecard.newRequest("card.attn");
  if (req == NULL) {
    DEBUG_PRINT("  ✗ Failed to create card.attn request");
    return;
  }

  JAddStringToObject(req, "mode", "arm");
  if (seconds > 0) {
    JAddNumberToObject(req, "seconds", seconds);
    DEBUG_PRINTF("  Armed ATTN interrupt (timer: %lu sec)\n", seconds);
  } else {
    DEBUG_PRINT("  Armed ATTN interrupt (event-only)");
  }

  notecard.sendRequest(req);
  g_attn_enabled = true;
}

/**
 * @brief Clear the Notecard attention interrupt
 *
 * Resets the latching ATTN pin after handling the event.
 * Must be called after each ATTN event.
 */
void clear_attn_interrupt() {
  J *req = notecard.newRequest("card.attn");
  if (req == NULL) {
    DEBUG_PRINT("  ✗ Failed to create card.attn clear request");
    return;
  }

  JAddStringToObject(req, "mode", "clear");
  notecard.sendRequest(req);
  g_attn_fired = false;
  g_attn_enabled = false;
  
  DEBUG_PRINT("  Cleared ATTN interrupt");
}

/**
 * @brief Handle ATTN interrupt event
 *
 * Called when ATTN pin fires. Checks for:
 * 1. Environment variable updates (config changes)
 * 2. Inbound Notes (commands from cloud)
 * 3. Timer expiration (if armed with timeout)
 */
void handle_attn_interrupt() {
  DEBUG_PRINT("\n=== ATTN Interrupt Received ===");

  // Clear the interrupt first (re-arms for next event)
  clear_attn_interrupt();

  // Check for environment variable updates
  DEBUG_PRINT("Checking for config updates...");
  if (env_vars_sync(notecard, &g_config)) {
    DEBUG_PRINT("✓ Configuration updated from Notehub!");
    env_vars_print_config(&g_config);

    // TODO: Apply new radio settings if frequency/power changed
    // if (radio_settings_changed) {
    //   reconfigure_radio();
    // }

    // TODO: Broadcast config to nodes if node settings changed
    // if (node_config_changed) {
    //   broadcast_node_config();
    // }
  } else {
    DEBUG_PRINT("  No configuration changes");
  }

  // TODO: Check for inbound Notes (commands from cloud)
  // J *req = notecard.newRequest("note.get");
  // JAddStringToObject(req, "file", "commands.qo");
  // J *rsp = notecard.sendRequest(req);
  // if (rsp != NULL && JGetObject(rsp, "notes") != NULL) {
  //   handle_cloud_commands(rsp);
  // }

  // Re-arm ATTN for next event
  arm_attn_interrupt();
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(3000);

  DEBUG_PRINT("\n*** LoRa-to-Blues Gateway ***");
  DEBUG_PRINT("Board: Adafruit Feather nRF52840 Express");
  DEBUG_PRINT("Using OFFICIAL Blues Library\n");

  // Initialize status LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize Notecard power (TODO Phase 3: MOSFET control)
  pinMode(NOTECARD_POWER_PIN, OUTPUT);
  digitalWrite(NOTECARD_POWER_PIN, HIGH);
  delay(1000);

  // Initialize ATTN pin interrupt
  pinMode(NOTECARD_ATTN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(NOTECARD_ATTN_PIN), attn_isr, RISING);
  DEBUG_PRINT("ATTN interrupt configured on pin D2");

  // Initialize Blues Notecard
  init_blues();

  // Initialize environment variables from Notehub
  DEBUG_PRINT("Loading configuration from Notehub...");
  if (env_vars_init(notecard, &g_config)) {
    DEBUG_PRINT("✓ Configuration loaded");
    env_vars_print_config(&g_config);
  } else {
    DEBUG_PRINT("✗ Using default configuration");
  }

  // Configure Notecard for inbound sync
  J *hub_req = notecard.newRequest("hub.set");
  JAddStringToObject(hub_req, "product", PRODUCT_ID);
  JAddStringToObject(hub_req, "mode", "continuous");
  // Get sync interval from registry
  const env_var_def_t* interval_var = env_vars_get_by_name("gateway_sync_interval");
  if (interval_var != NULL && interval_var->storage != NULL) {
    JAddNumberToObject(hub_req, "inbound", *((uint16_t*)interval_var->storage) / 60);
  }
  notecard.sendRequest(hub_req);

  // Initialize LoRa radio
  init_radio();

  // Arm ATTN interrupt for cloud communication
  arm_attn_interrupt();

  // Status blink
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  DEBUG_PRINT("\n*** Gateway ready - listening for packets ***\n");
  DEBUG_PRINT("ATTN pin enabled for cloud notifications");

  // Test transmit (debug only)
#if DEBUG == 1
  DEBUG_PRINT("Sending TEST packet...");
  const char* test_msg = "GATEWAY TEST";
  int state = radio.transmit((uint8_t*)test_msg, strlen(test_msg) + 1);
  DEBUG_PRINTF("Test TX: %s (%d)\n",
                state == RADIOLIB_ERR_NONE ? "SUCCESS" : "FAILED", state);

  radio.startReceive();
  DEBUG_PRINT("Radio back in RX mode\n");
#endif
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
  // Check for ATTN interrupt from Notecard (HIGH PRIORITY)
  if (g_attn_fired) {
    handle_attn_interrupt();
  }

  // Check if it's time to sync environment variables (periodic backup)
  if (env_vars_should_sync(&g_config)) {
    DEBUG_PRINT("\n=== Periodic Config Sync ===");

    if (env_vars_sync(notecard, &g_config)) {
      DEBUG_PRINT("✓ Configuration updated from Notehub!");
      env_vars_print_config(&g_config);

      // TODO: If config changed, apply new settings
      // e.g., update LoRa frequency if changed
    } else {
      DEBUG_PRINT("  No configuration changes");
    }
  }

  // TODO Phase 3 - LoRa Interrupt Support (Eliminate RX Polling)
  // ==========================================================================
  // Current: Polling mode - radio.receive() blocks and times out every 500ms
  // Goal: Interrupt-driven RX - sleep between packets, wake on DIO0 interrupt
  //
  // Hardware Changes Required:
  // --------------------------
  // 1. Wire RFM95W/RFM96W DIO0 pin to nRF52840 GPIO with interrupt support
  //    - RFM95W DIO0 → nRF52840 D3 (or any free GPIO: D4, D7, D11, D12, D13)
  //    - Current code uses LORA_DIO0 = 6, but pin 6 isn't broken out on Feather
  //    - Recommendation: Use D3 (GPIO P0.03) - available, interrupt-capable
  //
  // Code Changes Required:
  // ----------------------
  // 1. Add pin definition:
  //    #define LORA_DIO0_INTERRUPT_PIN  3
  //
  // 2. Add global flag:
  //    volatile bool g_lora_packet_ready = false;
  //
  // 3. Add ISR handler:
  //    void lora_dio0_isr() {
  //      g_lora_packet_ready = true;
  //    }
  //
  // 4. In setup(), attach interrupt:
  //    pinMode(LORA_DIO0_INTERRUPT_PIN, INPUT);
  //    attachInterrupt(digitalPinToInterrupt(LORA_DIO0_INTERRUPT_PIN),
  //                    lora_dio0_isr, RISING);
  //
  // 5. Configure RadioLib for DIO0 interrupt:
  //    radio.setDio0Action(lora_dio0_isr, RISING);  // Or use setPacketReceivedAction()
  //
  // 6. Replace polling loop with interrupt-driven RX:
  //    void loop() {
  //      if (g_attn_fired) { /* ... */ }
  //
  //      if (g_lora_packet_ready) {
  //        g_lora_packet_ready = false;
  //        int state = radio.readData(g_rx_buffer, 30);
  //        if (state == RADIOLIB_ERR_NONE) {
  //          // Process packet (existing code)
  //        }
  //      }
  //
  //      // Enter deep sleep (Phase 3)
  //      if (!g_attn_fired && !g_lora_packet_ready) {
  //        enter_system_off();  // ~6 µA, wake on ATTN or LORA_DIO0
  //      }
  //    }
  //
  // 7. Update radio.begin() to use correct DIO0 pin:
  //    SX1276 radio = new Module(LORA_CS, LORA_DIO0_INTERRUPT_PIN, LORA_RST, RADIOLIB_NC);
  //
  // Benefits:
  // ---------
  // - Eliminate 500ms polling loop → massive power savings
  // - Gateway sleeps ~6 µA between packets
  // - Wake instantly when LoRa packet arrives
  // - Combined with ATTN pin: fully event-driven architecture
  //
  // Pin Availability (nRF52840 Feather):
  // ------------------------------------
  // Available GPIOs with interrupt support:
  //   D3  (P0.03) ✓ Recommended
  //   D4  (P0.04) - Used by Wire (I2C SDA) - avoid
  //   D7  (P0.27) ✓ Available
  //   D11 (P0.01) ✓ Available (SPI MOSI)
  //   D12 (P0.02) ✓ Available (SPI MISO)
  //   D13 (P0.05) - Used by LED - avoid
  //
  // Note: RadioLib SX127x supports DIO0, DIO1, DIO2 interrupts
  //       DIO0 = Packet received (most common for RX)
  //       DIO1 = RX timeout (optional, for sleep timing)
  // ==========================================================================

  // Listen for LoRa packets from nodes (POLLING MODE - Phase 3: convert to interrupt)
  DEBUG_PRINT("Listening...");
  int state = radio.receive(g_rx_buffer, 30);

  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_PRINTF("[RX] Packet! Length: %d, RSSI: %d dBm\n",
                 radio.getPacketLength(), (int)radio.getRSSI());
    digitalWrite(LED_PIN, HIGH);

    // Print raw bytes
    DEBUG_PRINT("  Data: ");
    for (int i = 0; i < radio.getPacketLength(); i++) {
      DEBUG_PRINTF("%02X ", g_rx_buffer[i]);
    }
    DEBUG_PRINT("");

    // Parse sensor packet
    if (radio.getPacketLength() >= SENSOR_PACKET_SIZE) {
      sensor_packet_t* pkt = (sensor_packet_t*)g_rx_buffer;
      DEBUG_PRINTF("  Type: 0x%02X, Node: %d, Config: %d\n",
                   pkt->type, pkt->node_id, pkt->config_version);
      DEBUG_PRINTF("  Temp: %.1fC, Humidity: %.1f%%, Battery: %dmV\n",
                   pkt->payload.temperature, pkt->payload.humidity, pkt->payload.battery_mv);

      // Forward to Blues
      forward_to_blues(pkt);
    }

    digitalWrite(LED_PIN, LOW);
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    DEBUG_PRINT("  (timeout - no packet)");
  } else {
    DEBUG_PRINTF("  Error: %d\n", state);
  }

  delay(500);
}

void init_radio() {
  DEBUG_PRINT("Initializing LoRa radio...");

  int state = radio.begin(915.0, 125.0, 10, 5, 0x12, 14, 8, true);

  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_PRINT("  ✓ Radio OK");
    DEBUG_PRINT("  Freq: 915 MHz, SF: 10, BW: 125 kHz");
    DEBUG_PRINT("  Sync: 0x12, CRC: on");
  } else {
    DEBUG_PRINTF("  ✗ FAILED: %d\n", state);
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
}

void init_blues() {
  DEBUG_PRINT("Initializing Blues Notecard...");

  notecard.begin();

  // Configure product (REQUIRED!)
  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", PRODUCT_ID);
  JAddStringToObject(req, "mode", "continuous");  // Sync immediately
  notecard.sendRequest(req);

  DEBUG_PRINTF("✓ Product configured: %s\n", PRODUCT_ID);

  // Test connection
  req = notecard.newRequest("card.version");
  if (req != NULL) {
    if (notecard.sendRequest(req)) {
      DEBUG_PRINT("✓ Blues Notecard found!\n");
      return;
    }
  }

  DEBUG_PRINT("✗ Notecard NOT responding - check I2C!\n");
}

void forward_to_blues(sensor_packet_t* pkt) {
  DEBUG_PRINT("  Forwarding to Blues...");

  J *req = notecard.newRequest("note.add");
  if (req == NULL) {
    DEBUG_PRINT("  ✗ newRequest failed!");
    return;
  }

  JAddStringToObject(req, "file", "events.qo");
  JAddBoolToObject(req, "sync", true);  // Sync immediately

  // Build packet body with sensor data
  J *body = JAddObjectToObject(req, "body");
  if (body == NULL) {
    DEBUG_PRINT("  ✗ Failed to create body object");
    return;
  }

  // Add all fields - any failure means abort
  bool ok = true;
  ok &= (JAddNumberToObject(body, "node_id", pkt->node_id) != NULL);
  ok &= (JAddNumberToObject(body, "temp", pkt->payload.temperature) != NULL);
  ok &= (JAddNumberToObject(body, "humidity", pkt->payload.humidity) != NULL);
  ok &= (JAddNumberToObject(body, "battery_mv", pkt->payload.battery_mv) != NULL);
  ok &= (JAddNumberToObject(body, "rssi", (int32_t)pkt->rssi) != NULL);
  ok &= (JAddNumberToObject(body, "config_version", pkt->config_version) != NULL);

  if (!ok) {
    DEBUG_PRINT("  ✗ Failed to add sensor data to packet");
    return;
  }

  // Send to Blues
  if (notecard.sendRequest(req)) {
    DEBUG_PRINT("  ✓ Blues send SUCCESS!");
#if DEBUG == 1
    blink_tx_success();
#endif
  } else {
    DEBUG_PRINT("  ✗ Blues send FAILED");
  }
}
