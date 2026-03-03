/**
 * @file node_main_heltec_v4.cpp
 * @brief Sensor node for HelTec WiFi LoRa 32 V4 (ESP32-S3 + SX1262)
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
 * - HelTec WiFi LoRa 32 V4 (HTIT-WB32LAF V4.2)
 * - Built-in SX1262 LoRa radio
 * - Optional: OLED, GPS, sensors
 */

#include <Arduino.h>
#include <RadioLib.h>
#include "protocol.h"
#include "radio_config.h"
#include "pins.h"
#include "debug.h"

// ============================================================================
// Configuration
// ============================================================================

#define NODE_ID             3           // Unique ID (1-255)
#define TX_INTERVAL_SEC     5          // 30 seconds (for testing)         

// Display
#define USE_OLED            0           // 1=Enable OLED, 0=Disable

#if USE_OLED
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define OLED_WIDTH  128
  #define OLED_HEIGHT 64
  Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);
#endif

// ============================================================================
// RadioLib Instance (SX1262 for HelTec V4)
// ============================================================================

// SPI instance for ESP32-S3 (must be initialized before radio)
SPIClass* g_lora_spi = nullptr;

// SX1262: CS, DIO1, RST, BUSY (DIO2/3 not needed for basic operation)
// Will be initialized after SPI in init_radio()
SX1262* radio = nullptr;

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
  // ESP32-S3 USB needs time to enumerate
  delay(3000);

  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    delay(10);
  }
  delay(2000);

  Serial.println("\n*** Node HelTec V4 (ESP32-S3) ***");
  DEBUG_PRINT("\n*** Node HelTec V4 (ESP32-S3) ***");
  DEBUG_PRINTF("Board: HelTec WiFi LoRa 32 V4\n");
  DEBUG_PRINTF("Node ID: %d\n", NODE_ID);
  DEBUG_PRINTF("TX Interval: %d seconds\n", TX_INTERVAL_SEC);

#if USE_OLED
  init_display();
#endif

  init_radio();

  DEBUG_PRINT("*** Ready ***\n");

  // First transmission
  send_sensor_data();

#if DEBUG == 0
  // Production mode: initial sleep
  DEBUG_PRINT("\nSleeping...\n");
  enter_light_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
#endif
  // Debug mode: skip sleep, let loop() handle it with delay
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
#if DEBUG == 1
  // Debug mode: use delay (keeps USB serial alive)
  DEBUG_PRINT("\n[LOOP] Starting loop iteration\n");
  
  radio->standby();
  delay(10);
  DEBUG_PRINT("[LOOP] Radio woken\n");
  
  send_sensor_data();
  DEBUG_PRINT("[LOOP] TX complete\n");
  
  DEBUG_PRINTF("[LOOP] Waiting %d seconds...\n\n", TX_INTERVAL_SEC);
  delay((uint32_t)TX_INTERVAL_SEC * 1000UL);
#else
  // Production mode: use light sleep (lower power, but USB CDC is lost)
  send_sensor_data();
  enter_light_sleep((uint32_t)TX_INTERVAL_SEC * 1000UL);
#endif
}

// ============================================================================
// Display Initialization
// ============================================================================

void init_display(void) {
#if USE_OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    DEBUG_PRINT("OLED init failed\n");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("HelTec Node V4");
  display.printf("ID: %d\n", NODE_ID);
  display.display();
  delay(100);

  DEBUG_PRINT("OLED initialized\n");
#endif
}

// ============================================================================
// Radio Initialization
// ============================================================================

void init_radio(void) {
  DEBUG_PRINT("Initializing SX1262...\n");

  // Initialize SPI for ESP32-S3 (SCK, MISO, MOSI, SS)
  // This must be done before creating the radio instance
  g_lora_spi = new SPIClass(HSPI);
  g_lora_spi->begin(LORA_SPI_SCK, LORA_SPI_MISO, LORA_SPI_MOSI, LORA_CS_PIN);
  DEBUG_PRINTF("SPI: SCK=%d, MISO=%d, MOSI=%d, CS=%d\n", 
                LORA_SPI_SCK, LORA_SPI_MISO, LORA_SPI_MOSI, LORA_CS_PIN);
  delay(100);

  // Create radio instance with explicit SPI class
  // SX1262 constructor: CS, DIO1, RST, BUSY, SPI
  radio = new SX1262(new Module(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_DIO0_PIN, *g_lora_spi));

  // SX1262 requires different init than SX1276
  // RadioLib SX1262::begin() uses: frequency, bandwidth, SF, codingRate, syncWord, power, preambleLength
  int state = radio->begin(
    LORA_FREQUENCY_DEFAULT,       // Frequency in MHz
    LORA_BANDWIDTH,               // Bandwidth in kHz
    LORA_SPREADING_FACTOR,        // Spreading Factor (7-12)
    LORA_CODING_RATE,             // Coding Rate (5-8, represents 4/5 to 4/8)
    LORA_SYNC_WORD,               // Sync word
    LORA_TX_POWER,                // TX power in dBm
    LORA_PREAMBLE_LENGTH          // Preamble length in symbols
  );

  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_PRINTF("Radio OK: %.1f MHz, SF%d, %d dBm\n",
                  LORA_FREQUENCY_DEFAULT, LORA_SPREADING_FACTOR, LORA_TX_POWER);
  } else {
    DEBUG_PRINTF("ERROR: Radio init failed: %d\n", state);
    // Blink LED on error
    while (1) {
      digitalWrite(PIN_LED, HIGH);
      delay(200);
      digitalWrite(PIN_LED, LOW);
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

  DEBUG_PRINTF("Sensors: T=%.1fC, H=%.1f%%, Vbat=%dmV\n",
                payload->temperature, payload->humidity, payload->battery_mv);

#if USE_OLED
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
  DEBUG_PRINTF("\n[TX %lu] Sending...\n", ++g_tx_count);

  sensor_payload_t payload;
  read_sensors(&payload);

  packet_t pkt;
  protocol_build_sensor_packet(&pkt, NODE_ID, g_config_version, &payload);

  DEBUG_PRINT("Transmitting...");
  int state = radio->transmit(pkt.raw, SENSOR_PACKET_SIZE);

  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_PRINT("TX OK!");
  } else {
    DEBUG_PRINTF("TX failed: %d\n", state);
  }
}

// ============================================================================
// Power Management - Light Sleep
// ============================================================================

/**
 * @brief Enter light sleep (ESP32-S3)
 *
 * ESP32 light sleep:
 * - Current: ~100 µA
 * - RAM preserved
 * - Wake via: timer, GPIO, touch
 */
void enter_light_sleep(uint32_t sleep_ms) {
  DEBUG_PRINTF("Sleeping for %lu ms\n", sleep_ms);

  // Turn off radio
  radio->sleep();

#if USE_OLED
  display.ssd1306_command(SSD1306_DISPLAYOFF);
#endif

  // Configure wake timer (ESP32-S3 uses microseconds)
  esp_sleep_enable_timer_wakeup(sleep_ms * 1000ULL);

  // Enter light sleep
  DEBUG_PRINT("Goodnight!\n");
  Serial.flush();
  delay(100);

  esp_light_sleep_start();

  // After wake: continues from loop()
  // Re-initialize SPI and radio after sleep
  g_lora_spi->begin(LORA_SPI_SCK, LORA_SPI_MISO, LORA_SPI_MOSI, LORA_CS_PIN);
  delay(50);
  
  // Re-init USB Serial (non-blocking - don't hang if monitor not open)
  Serial.begin(SERIAL_BAUD);
  delay(100);  // Brief wait for USB, but don't block
}
