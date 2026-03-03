/**
 * @file pins.h
 * @brief Board-specific pin definitions for LoRa sensor nodes
 * @version 1.0
 * @date 2026-02-27
 *
 * Pin mappings for supported hardware platforms.
 * Add new boards by adding #elif blocks as needed.
 */

#pragma once

// ============================================================================
// Node Pin Definitions (board-specific)
// ============================================================================

#if defined(BOARD_NODE_NRF52840)
  // Adafruit nRF52840 Express + Adafruit LoRa FeatherWing (RFM95W)
  // FeatherWing uses standard Feather SPI pins
  #define LORA_CS_PIN       10          // P0.06
  #define LORA_RST_PIN      8           // P0.08
  #define LORA_DIO0_PIN     4           // P0.04 (RX/TX interrupt)
  #define LORA_DIO1_PIN     7           // P0.27 (optional, CAD)
  #define PIN_LED           PIN_LED1    // Status LED
  #define PIN_BATTERY       A0          // Battery voltage sense

#elif defined(BOARD_NODE_FEATHER_M0)
  // Adafruit Feather M0 RFM9x LoRa Radio 900MHz (built-in RFM95W)
  // Radio is integrated on the board, pins are fixed
  #define LORA_CS_PIN       8           // P0.08
  #define LORA_RST_PIN      4           // P0.04
  #define LORA_DIO0_PIN     3           // P0.09 (RX/TX interrupt)
  #define LORA_DIO1_PIN     6           // P0.05 (optional, CAD)
  #undef PIN_LED
  #define PIN_LED           PIN_LED2    // Status LED (blue)
  #define PIN_BATTERY       A7          // Battery voltage sense (divided by 2)

#elif defined(BOARD_NODE_HELTEC_V4)
  // HelTec WiFi LoRa 32 V4 (ESP32-S3 + SX1262)
  // Board: HTIT-WB32LAF V4.2
  // SX1262 radio with ESP32-S3 hardware SPI
  // RadioLib Module: CS, DIO1, RST, BUSY
  #define LORA_CS_PIN       8           // NSS (GPIO 8)
  #define LORA_RST_PIN      12          // RESET (GPIO 12)
  #define LORA_DIO0_PIN     13          // BUSY (GPIO 13) - RadioLib uses for busy/wait
  #define LORA_DIO1_PIN     14          // DIO1 (GPIO 14)
  // Hardware SPI (ESP32-S3 default: HSPI)
  #define LORA_SPI_SCK      9           // SCK (GPIO 9)
  #define LORA_SPI_MOSI     10          // MOSI (GPIO 10)
  #define LORA_SPI_MISO     11          // MISO (GPIO 11)
  #define PIN_LED           35          // Built-in LED (V4 specific)
  #define PIN_BATTERY       A0          // Battery voltage sense
  // OLED (V4, if present) - per HelTec V4 documentation
  #define OLED_SDA          17          // OLED SDA (GPIO 17)
  #define OLED_SCL          18          // OLED SCL (GPIO 18)
  #define OLED_RST          21          // OLED Reset (GPIO 21)

#elif defined(BOARD_NODE_ESP32_FEATHER)
  // Adafruit ESP32 Feather + Adafruit LoRa FeatherWing (RFM95W)
  // FeatherWing uses standard Feather SPI pins
  #define LORA_CS_PIN       33          // GPIO33 (SPI CS)
  #define LORA_RST_PIN      13          // GPIO13 (RST)
  #define LORA_DIO0_PIN     27          // GPIO27 (RX/TX interrupt)
  #define LORA_DIO1_PIN     32          // GPIO32 (optional, CAD)
  #define PIN_LED           13          // GPIO13 (red LED, also RST pin - use carefully)
  #define PIN_BATTERY       35          // GPIO35 = A13 (battery sense)

#elif defined(BOARD_NODE_HELTEC)
  // HelTec WiFi LoRa 32 V5 (ESP32 + SX1262) - legacy alias
  #define LORA_CS_PIN       18
  #define LORA_RST_PIN      12
  #define LORA_DIO0_PIN     14
  #define LORA_DIO1_PIN     13
  #define PIN_LED           LED_BUILTIN
  #define PIN_BATTERY       A0

#else
  #error "BOARD_NODE_* not defined! Select a board environment in platformio.ini"
#endif

// ============================================================================
// Gateway Pin Definitions (for reference)
// ============================================================================

#if defined(BOARD_GATEWAY_NRF52840)
  // Adafruit Feather nRF52840 + RFM95W + Blues Notecard
  #define LORA_CS_PIN       10          // P0.06
  #define LORA_RST_PIN      8           // P0.08
  #define LORA_DIO0_PIN     4           // P0.04
  #define LORA_DIO1_PIN     7           // P0.27

  #define NOTECARD_SDA      PIN_WIRE_SDA    // P0.30 (D20/A4)
  #define NOTECARD_SCL      PIN_WIRE_SCL    // P0.31 (D21/A5)
  #define NOTECARD_POWER_PIN 5              // P0.29 (D5)

#elif defined(BOARD_GATEWAY_FEATHER_M0)
  // Adafruit Feather M0 + RFM95W + Blues Notecard
  #define LORA_CS_PIN       8
  #define LORA_RST_PIN      4
  #define LORA_DIO0_PIN     3
  #define LORA_DIO1_PIN     6

  #define NOTECARD_SDA      PIN_WIRE_SDA
  #define NOTECARD_SCL      PIN_WIRE_SCL
  #define NOTECARD_POWER_PIN 5

#elif defined(BOARD_GATEWAY_RAK4630)
  // RAK4630 + Blues Notecard
  #define LORA_CS_PIN       24          // P0.15
  #define LORA_RST_PIN      38          // P0.18
  #define LORA_DIO0_PIN     17          // P0.22
  #define LORA_DIO1_PIN     13          // P0.24

  #define NOTECARD_SDA      PIN_WIRE_SDA
  #define NOTECARD_SCL      PIN_WIRE_SCL
  #define NOTECARD_POWER_PIN 5

#endif
