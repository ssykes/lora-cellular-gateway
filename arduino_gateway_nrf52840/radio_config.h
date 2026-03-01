/**
 * @file radio_config.h
 * @brief LoRa radio configuration for RFM95W/SX127x
 */

#ifndef _RADIO_CONFIG_H_
#define _RADIO_CONFIG_H_

#define LORA_FREQUENCY_DEFAULT  915.0f
#define LORA_FREQUENCY_MIN      902.0f
#define LORA_FREQUENCY_MAX      927.0f

static inline bool radio_frequency_valid(float freq) {
    return (freq >= LORA_FREQUENCY_MIN && freq <= LORA_FREQUENCY_MAX);
}

#endif
