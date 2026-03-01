/**
 * @file gateway_main.cpp
 * @brief LoRa-to-Blues Gateway for Adafruit Feather nRF52840 Express
 * @version 4.0 - With Environment Variable Management
 *
 * Hardware:
 * - Adafruit Feather nRF52840 Express
 * - LoRa FeatherWing (RFM95W)
 * - Blues Notecarrier-F
 * - Blues Notecard (NOTE-WBNAN)
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Notecard.h>
#include "protocol.h"
#include "env_vars.h"

// YOUR PRODUCT ID
#define PRODUCT_ID "net.pacbell.ssykes11:lora_cellular_field_gateway"

// LoRa settings - MUST MATCH NODE
#define LORA_FREQUENCY        915.0f
#define LORA_BANDWIDTH        125.0f
#define LORA_SPREADING_FACTOR 10
#define LORA_CODING_RATE      5
#define LORA_TX_POWER         14
#define LORA_SYNC_WORD        0x12
#define LORA_PREAMBLE_LENGTH  8

// FeatherWing pins (working configuration)
#define LORA_CS               10
#define LORA_RST              11
#define LORA_DIO0             6

#define NOTECARD_POWER_PIN    5
#define LED_PIN               13

// Radio instance (RFM95W = SX1276)
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, RADIOLIB_NC);

// Blues Notecard instance (OFFICIAL library)
Notecard notecard;

// Gateway configuration (from Notehub environment variables)
gateway_config_t g_config;

uint8_t g_rx_buffer[30];
uint8_t g_rx_length = 0;

void init_radio();
void init_blues();
void forward_to_blues(sensor_packet_t* pkt);

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("\n*** LoRa-to-Blues Gateway ***");
  Serial.println("Board: Adafruit Feather nRF52840 Express");
  Serial.println("Using OFFICIAL Blues Library\n");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Power Notecard
  pinMode(NOTECARD_POWER_PIN, OUTPUT);
  digitalWrite(NOTECARD_POWER_PIN, HIGH);
  delay(1000);

  // Initialize Blues
  init_blues();

  // Initialize environment variables from Notehub
  Serial.println("Loading configuration from Notehub...");
  if (env_vars_init(notecard, &g_config)) {
    Serial.println("✓ Configuration loaded");
    env_vars_print_config(&g_config);
  } else {
    Serial.println("✗ Using default configuration");
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

  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n*** Gateway ready - listening for packets ***\n");
  
  // Test transmit
  Serial.println("Sending TEST packet...");
  const char* test_msg = "GATEWAY TEST";
  int state = radio.transmit((uint8_t*)test_msg, strlen(test_msg) + 1);
  Serial.printf("Test TX: %s (%d)\n", 
                state == RADIOLIB_ERR_NONE ? "SUCCESS" : "FAILED", state);
  
  radio.startReceive();
  Serial.println("Radio back in RX mode\n");
}

void loop() {
  // Check if it's time to sync environment variables
  if (env_vars_should_sync(&g_config)) {
    Serial.println("\n=== Syncing Environment Variables ===");
    
    if (env_vars_sync(notecard, &g_config)) {
      Serial.println("✓ Configuration updated from Notehub!");
      env_vars_print_config(&g_config);
      
      // TODO: If config changed, apply new settings
      // e.g., update LoRa frequency if changed
    } else {
      Serial.println("  No configuration changes");
    }
  }
  
  Serial.println("Listening...");
  int state = radio.receive(g_rx_buffer, 30);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.printf("[RX] Packet! Length: %d, RSSI: %d dBm\n",
                  radio.getPacketLength(), (int)radio.getRSSI());
    digitalWrite(LED_PIN, HIGH);

    // Print raw bytes
    Serial.print("  Data: ");
    for (int i = 0; i < radio.getPacketLength(); i++) {
      Serial.printf("%02X ", g_rx_buffer[i]);
    }
    Serial.println();

    // Parse sensor packet
    if (radio.getPacketLength() >= SENSOR_PACKET_SIZE) {
      sensor_packet_t* pkt = (sensor_packet_t*)g_rx_buffer;
      Serial.printf("  Type: 0x%02X, Node: %d, Config: %d\n",
                    pkt->type, pkt->node_id, pkt->config_version);
      Serial.printf("  Temp: %.1fC, Humidity: %.1f%%, Battery: %dmV\n",
                    pkt->payload.temperature, pkt->payload.humidity, pkt->payload.battery_mv);

      // Forward to Blues
      forward_to_blues(pkt);
    }

    digitalWrite(LED_PIN, LOW);
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println("  (timeout - no packet)");
  } else {
    Serial.printf("  Error: %d\n", state);
  }

  delay(500);
}

void init_radio() {
  Serial.println("Initializing LoRa radio...");

  int state = radio.begin(915.0, 125.0, 10, 5, 0x12, 14, 8, true);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("  ✓ Radio OK");
    Serial.println("  Freq: 915 MHz, SF: 10, BW: 125 kHz");
    Serial.println("  Sync: 0x12, CRC: on");
  } else {
    Serial.printf("  ✗ FAILED: %d\n", state);
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
}

void init_blues() {
  Serial.println("Initializing Blues Notecard...");
  
  notecard.begin();
  
  // Configure product (REQUIRED!)
  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", PRODUCT_ID);
  JAddStringToObject(req, "mode", "continuous");  // Sync immediately
  notecard.sendRequest(req);
  
  Serial.printf("✓ Product configured: %s\n", PRODUCT_ID);
  
  // Test connection
  req = notecard.newRequest("card.version");
  if (req != NULL) {
    if (notecard.sendRequest(req)) {
      Serial.println("✓ Blues Notecard found!\n");
      return;
    }
  }
  
  Serial.println("✗ Notecard NOT responding - check I2C!\n");
}

void forward_to_blues(sensor_packet_t* pkt) {
  Serial.println("  Forwarding to Blues...");

  J *req = notecard.newRequest("note.add");
  if (req == NULL) {
    Serial.println("  ✗ newRequest failed!");
    return;
  }
  
  JAddStringToObject(req, "file", "events.qo");
  JAddBoolToObject(req, "sync", true);  // Sync immediately
  
  J *body = JAddObjectToObject(req, "body");
  if (body != NULL) {
    JAddNumberToObject(body, "node_id", pkt->node_id);
    JAddNumberToObject(body, "temp", pkt->payload.temperature);
    JAddNumberToObject(body, "humidity", pkt->payload.humidity);
    JAddNumberToObject(body, "battery_mv", pkt->payload.battery_mv);
    JAddNumberToObject(body, "rssi", (int32_t)pkt->rssi);
    JAddNumberToObject(body, "config_version", pkt->config_version);
    
    if (notecard.sendRequest(req)) {
      Serial.println("  ✓ Blues send SUCCESS!");
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
    } else {
      Serial.println("  ✗ Blues send FAILED");
    }
  } else {
    Serial.println("  ✗ Failed to create body object");
  }
}
