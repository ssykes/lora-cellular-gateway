/**
 * @file radio_config.h
 * @brief LoRa radio configuration for RFM95W/SX127x
 * @version 1.0
 * @date 2026-02-24
 * 
 * Radio settings optimized for field deployment:
 * - SF10 for balance of range and airtime
 * - 915 MHz for North America
 * - 14 dBm TX power (FCC compliant)
 */

#ifndef _RADIO_CONFIG_H_
#define _RADIO_CONFIG_H_

// ============================================================================
// Frequency Settings (Region Dependent)
// ============================================================================

/**
 * Default frequency (used if no Blues Environment Variable is set)
 * Change via Blues Dashboard: notehub.io → Products → [Your Product] → Environment
 * Variable name: lora_frequency (e.g., 915.0, 903.0, 868.0)
 */

// North America (FCC) - Default
#define LORA_FREQUENCY_DEFAULT  915.0f      // MHz

// Europe (ETSI) - uncomment for European deployment
// #define LORA_FREQUENCY_DEFAULT 868.0f   // MHz

// ============================================================================
// FCC Band Limits (902-927 MHz ISM Band)
// ============================================================================

#define LORA_FREQUENCY_MIN      902.0f      // MHz (FCC lower bound)
#define LORA_FREQUENCY_MAX      927.0f      // MHz (FCC upper bound)
#define LORA_CHANNEL_STEP       0.5f        // MHz (500 kHz steps)

/**
 * Predefined channels for interference avoidance
 * Use these or any frequency within LORA_FREQUENCY_MIN/MAX
 */
static const float LORA_CHANNELS[] = {
    903.0f,   // Channel 0
    905.0f,   // Channel 1
    907.0f,   // Channel 2
    909.0f,   // Channel 3
    915.0f,   // Channel 4 (default)
    917.0f,   // Channel 5
    921.0f,   // Channel 6
    923.0f,   // Channel 7
};
#define LORA_NUM_CHANNELS       (sizeof(LORA_CHANNELS) / sizeof(LORA_CHANNELS[0]))
#define LORA_DEFAULT_CHANNEL    4           // Index of 915.0 MHz

// ============================================================================
// LoRa Modulation Settings
// ============================================================================

/**
 * Spreading Factor trade-offs:
 * SF7:  Fast, short range (~25ms airtime)
 * SF10: Balanced, medium range (~100ms airtime) <- Our choice
 * SF12: Slow, maximum range (~200ms airtime)
 */
#define LORA_BANDWIDTH          125.0f      // kHz (7.8-500 kHz available)
#define LORA_SPREADING_FACTOR   10          // SF7-SF12
#define LORA_CODING_RATE        5           // 5-8 (represents 4/5 to 4/8)
#define LORA_PREAMBLE_LENGTH    8           // Symbols
#define LORA_SYNC_WORD          0x12        // Private network (0x34 = LoRaWAN)

/**
 * TX Power settings:
 * - FCC limit: +36 dBm EIRP
 * - Our setting: +14 dBm (conservative, good battery life)
 * - Max for RFM95W: +20 dBm (with PA_BOOST)
 */
#define LORA_TX_POWER           14          // dBm (2-17 typical, up to 20)

/**
 * CRC and encoding
 */
#define LORA_CRC_ENABLED        true        // Enable CRC checking
#define LORA_INVERT_IQ          false       // Normal IQ (true for downlink)

// ============================================================================
// Timing Settings
// ============================================================================

/**
 * Gateway listening schedule
 * Gateway wakes periodically to check Blues mailbox and listen for nodes
 */
#define GATEWAY_LISTEN_START_HOUR   8         // 08:00 (24-hour format)
#define GATEWAY_LISTEN_END_HOUR     22        // 22:00
#define GATEWAY_WAKE_INTERVAL_MS    (15 * 60 * 1000UL)  // 15 minutes
#define GATEWAY_LISTEN_WINDOW_MS    (30 * 1000UL)       // 30 seconds

/**
 * Node transmission settings
 */
#define NODE_TX_INTERVAL_DEFAULT_MS (30 * 60 * 1000UL)  // 30 minutes (default)
#define NODE_TX_INTERVAL_MIN_MS     (5 * 60 * 1000UL)   // 5 minutes (minimum)
#define NODE_TX_INTERVAL_MAX_MS     (24 * 60 * 60 * 1000UL) // 24 hours (maximum)

/**
 * Config response listen window (node waits for config update after TX)
 */
#define NODE_CONFIG_LISTEN_WINDOW_MS 3000     // 3 seconds

/**
 * Relay timeout (how long to wait for packet when relaying)
 */
#define RELAY_RX_TIMEOUT_MS         5000      // 5 seconds

// ============================================================================
// RSSI Thresholds
// ============================================================================

/**
 * RSSI quality thresholds (dBm, more negative = weaker signal)
 */
#define RSSI_EXCELLENT              -50       // Excellent signal
#define RSSI_GOOD                   -70       // Good signal
#define RSSI_FAIR                   -90       // Fair signal (minimum for reliable comm)
#define RSSI_POOR                   -100      // Poor signal (may have packet loss)
#define RSSI_UNRELIABLE             -110      // Unreliable (near noise floor)

/**
 * Noise floor (approximate for 125 kHz bandwidth)
 */
#define RSSI_NOISE_FLOOR            -120      // dBm

// ============================================================================
// Channel Activity Detection (CAD)
// ============================================================================

/**
 * CAD settings for collision avoidance
 * CAD detects if channel is busy before transmitting
 */
#define CAD_ENABLED                 true
#define CAD_SYMBOLS                 8         // Number of symbols for CAD
#define CAD_TIMEOUT_MS              100       // Max time for CAD check

// ============================================================================
// Radio Chip Select Pin (Board Dependent)
// ============================================================================

// Adafruit Feather M0 + RFM95W
#if defined(ARDUINO_SAMD_FEATHER_M0)
    #define RADIO_CS_PIN            10
    #define RADIO_RST_PIN           2
    #define RADIO_DIO0_PIN          3
    #define RADIO_DIO1_PIN          6

// Adafruit nRF52840 + RFM95W
#elif defined(ARDUINO_NRF52840_FEATHER)
    #define RADIO_CS_PIN            10        // P0.06
    #define RADIO_RST_PIN           8         // P0.08
    #define RADIO_DIO0_PIN          4         // P0.04
    #define RADIO_DIO1_PIN          7         // P0.27 (verify wiring)

// HelTec WiFi LoRa 32 V5 (ESP32)
#elif defined(CONFIG_IDF_TARGET_ESP32)
    #define RADIO_CS_PIN            18        // SS
    #define RADIO_RST_PIN           14        // RST
    #define RADIO_DIO0_PIN          26        // DIO0
    #define RADIO_DIO1_PIN          33        // DIO1

// RAK4630 (WisBlock)
#elif defined(ARDUINO_RAK4630)
    #define RADIO_CS_PIN            26        // P0.26
    #define RADIO_RST_PIN           8         // P0.08
    #define RADIO_DIO0_PIN          13        // P0.13
    #define RADIO_DIO1_PIN          3         // P0.03

// Default (override in board-specific code)
#else
    #define RADIO_CS_PIN            10
    #define RADIO_RST_PIN           2
    #define RADIO_DIO0_PIN          3
    #define RADIO_DIO1_PIN          6
#endif

// ============================================================================
// RadioLib Configuration Structure
// ============================================================================

/**
 * @brief Radio configuration structure
 * 
 * Passed to RadioLib for initialization
 */
typedef struct {
    float frequency;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    int8_t txPower;
    uint16_t preambleLength;
    bool crcOn;
    bool invertIq;
} radio_config_t;

/**
 * @brief Default radio configuration
 *
 * Note: frequency is overridden by Blues Environment Variable "lora_frequency"
 * if present. All other settings use these defaults.
 */
static const radio_config_t g_radio_config_default = {
    .frequency = LORA_FREQUENCY_DEFAULT,
    .bandwidth = LORA_BANDWIDTH,
    .spreadingFactor = LORA_SPREADING_FACTOR,
    .codingRate = LORA_CODING_RATE,
    .txPower = LORA_TX_POWER,
    .preambleLength = LORA_PREAMBLE_LENGTH,
    .crcOn = LORA_CRC_ENABLED,
    .invertIq = LORA_INVERT_IQ
};

// ============================================================================
// Airtime Calculation (for reference)
// ============================================================================

/**
 * Approximate airtime for settings above (SF10, BW125, 4/5):
 * 
 * Payload Size | Airtime
 * -------------|--------
 * 10 bytes     | ~50 ms
 * 15 bytes     | ~70 ms
 * 20 bytes     | ~90 ms
 * 50 bytes     | ~180 ms
 * 
 * Formula: T = T_preamble + T_symbol * (payload_symbols)
 * Where T_symbol = 2^SF / BW
 */

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * @brief Validate frequency is within legal bounds
 * @param frequency_mhz Frequency in MHz
 * @return true if valid, false if out of range
 */
static inline bool radio_frequency_valid(float frequency_mhz) {
    return (frequency_mhz >= LORA_FREQUENCY_MIN && 
            frequency_mhz <= LORA_FREQUENCY_MAX);
}

/**
 * @brief Initialize radio with default configuration
 * @return 0 Success, negative error code
 */
int radio_init(void);

/**
 * @brief Set radio to receive mode
 * @return 0 Success, negative error code
 */
int radio_start_rx(void);

/**
 * @brief Set radio to transmit mode
 * @return 0 Success, negative error code
 */
int radio_start_tx(void);

/**
 * @brief Set radio to sleep mode (lowest power)
 * @return 0 Success, negative error code
 */
int radio_sleep(void);

/**
 * @brief Set radio to standby mode (quick wake)
 * @return 0 Success, negative error code
 */
int radio_standby(void);

/**
 * @brief Send packet
 * @param data Packet data
 * @param length Packet length
 * @return 0 Success, negative error code
 */
int radio_send(const uint8_t* data, size_t length);

/**
 * @brief Get last received RSSI
 * @return RSSI in dBm
 */
float radio_get_rssi(void);

/**
 * @brief Set TX power
 * @param power_dbm Power in dBm
 * @return 0 Success, negative error code
 */
int radio_set_tx_power(int8_t power_dbm);

#endif // _RADIO_CONFIG_H_
