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

#include <stdint.h>

// ============================================================================
// Frequency Settings (Region Dependent)
// ============================================================================

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

// ============================================================================
// LoRa Modulation Settings
// ============================================================================

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

#endif // _RADIO_CONFIG_H_
