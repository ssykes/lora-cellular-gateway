/**
 * @file protocol.h
 * @brief Binary protocol definition for LoRa sensor network
 * @version 1.0
 * @date 2026-02-24
 * 
 * Packet structure for communication between sensor nodes and gateway.
 * Optimized for minimal airtime and battery efficiency.
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// Packet Type Definitions
// ============================================================================

#define PKT_TYPE_SENSOR_DATA    0x01    // Node → Gateway (sensor reading + config request)
#define PKT_TYPE_CONFIG_UPDATE  0x02    // Gateway → Node (configuration update)
#define PKT_TYPE_ACK            0x03    // Node → Gateway (acknowledgment, optional)
#define PKT_TYPE_RELAY          0x04    // Node → Node (relay packet)
#define PKT_TYPE_SYNC           0x00    // Reserved for future use

// ============================================================================
// Protocol Constants
// ============================================================================

#define PROTOCOL_VERSION        1       // Protocol version (for future expansion)
#define MAX_NODE_ID             255     // Maximum node ID (1 byte)
#define MAX_HOP_COUNT           3       // Maximum relay hops before drop
#define GATEWAY_NODE_ID         0       // Reserved ID for gateway

#define CONFIG_VERSION_INITIAL  1       // Starting config version
#define CONFIG_VERSION_MAX      255     // Maximum config version (wraps to 1)

// ============================================================================
// Sensor Data Packet (Node → Gateway)
// ============================================================================

/**
 * @brief Sensor data payload structure
 * 
 * Flexible payload supporting different sensor types.
 * Initial implementation uses simple temperature/humidity/battery.
 */
typedef struct {
    float temperature;      // Temperature in Celsius (4 bytes)
    float humidity;         // Humidity in % (4 bytes)
    uint16_t battery_mv;    // Battery voltage in mV (2 bytes)
    uint8_t sensor_flags;   // Bit flags for sensor status (1 byte)
} __attribute__((packed)) sensor_payload_t;

// Sensor flags
#define SENSOR_FLAG_BATTERY_LOW   0x01    // Battery below threshold
#define SENSOR_FLAG_SENSOR_ERROR  0x02    // Sensor read error
#define SENSOR_FLAG_RELAY_MODE    0x04    // Node is in relay mode

/**
 * @brief Sensor data packet structure
 * 
 * Transmitted by nodes at configured intervals.
 * Includes config version request for automatic updates.
 */
typedef struct {
    uint8_t type;             // PKT_TYPE_SENSOR_DATA (0x01)
    uint8_t node_id;          // Source node ID (1-255)
    uint8_t hop_count;        // Number of relays (0 = direct)
    uint8_t rssi;             // RSSI in dBm (added by receiver)
    uint8_t config_version;   // Node's current config version
    sensor_payload_t payload; // Sensor data
    uint16_t crc;             // 16-bit CRC
} __attribute__((packed)) sensor_packet_t;

#define SENSOR_PACKET_SIZE    sizeof(sensor_packet_t)  // ~15 bytes

// ============================================================================
// Config Update Packet (Gateway → Node)
// ============================================================================

/**
 * @brief Configuration flags
 */
typedef struct {
    uint8_t relay_enabled : 1;    // Enable relay mode
    uint8_t reserved : 7;         // Reserved for future use
} __attribute__((packed)) config_flags_t;

/**
 * @brief Configuration update payload
 */
typedef struct {
    config_flags_t flags;     // Configuration flags
    uint8_t config_version;   // New config version number
    uint16_t interval_minutes;// Transmission interval (minutes)
    uint8_t relay_enabled;    // Relay mode (0/1) - redundant for convenience
    uint8_t tx_power_dbm;     // TX power in dBm
    uint8_t reserved;         // Reserved for future use
} __attribute__((packed)) config_payload_t;

/**
 * @brief Config update packet structure
 * 
 * Sent by gateway only when node's config version doesn't match current.
 * Silent ACK for normal operations (no response needed).
 */
typedef struct {
    uint8_t type;             // PKT_TYPE_CONFIG_UPDATE (0x02)
    uint8_t node_id;          // Target node ID
    config_flags_t flags;     // Configuration flags
    uint8_t config_version;   // New config version
    uint16_t interval_minutes;// Transmission interval (minutes)
    uint8_t relay_enabled;    // Relay mode (0/1)
    uint8_t tx_power_dbm;     // TX power in dBm
    uint16_t crc;             // 16-bit CRC
} __attribute__((packed)) config_packet_t;

#define CONFIG_PACKET_SIZE    sizeof(config_packet_t)  // ~11 bytes

// ============================================================================
// ACK Packet (Optional)
// ============================================================================

typedef struct {
    uint8_t type;             // PKT_TYPE_ACK (0x03)
    uint8_t node_id;          // Source node ID
    uint8_t ack_type;         // Type of acknowledgment
    uint8_t reserved;         // Reserved
    uint16_t crc;             // 16-bit CRC
} __attribute__((packed)) ack_packet_t;

#define ACK_PACKET_SIZE       sizeof(ack_packet_t)

// ACK types
#define ACK_TYPE_CONFIG_RECEIVED  0x01  // Config update received
#define ACK_TYPE_DATA_RECEIVED    0x02  // Sensor data received (not used - silent ACK)

// ============================================================================
// Generic Packet Union (for buffer allocation)
// ============================================================================

#define MAX_PACKET_SIZE       20        // Maximum packet size in bytes

typedef union {
    uint8_t raw[MAX_PACKET_SIZE];
    sensor_packet_t sensor;
    config_packet_t config;
    ack_packet_t ack;
} packet_t;

// ============================================================================
// CRC Functions
// ============================================================================

/**
 * @brief Calculate 16-bit CRC for packet
 * 
 * @param data Pointer to packet data (excluding CRC)
 * @param length Length of data (excluding CRC)
 * @return uint16_t CRC value
 */
uint16_t protocol_calc_crc(const uint8_t* data, size_t length);

/**
 * @brief Verify packet CRC
 * 
 * @param packet Pointer to complete packet (including CRC)
 * @param length Total packet length (including CRC)
 * @return true CRC valid
 * @return false CRC invalid
 */
bool protocol_verify_crc(const uint8_t* packet, size_t length);

// ============================================================================
// Packet Builder Functions
// ============================================================================

/**
 * @brief Build sensor data packet
 * 
 * @param pkt Pointer to packet buffer
 * @param node_id Node ID
 * @param config_ver Current config version
 * @param payload Sensor data payload
 */
void protocol_build_sensor_packet(packet_t* pkt, uint8_t node_id, 
                                   uint8_t config_ver, sensor_payload_t* payload);

/**
 * @brief Build config update packet
 * 
 * @param pkt Pointer to packet buffer
 * @param node_id Target node ID
 * @param config New configuration
 */
void protocol_build_config_packet(packet_t* pkt, uint8_t node_id, 
                                   config_payload_t* config);

// ============================================================================
// Inline Helper Functions
// ============================================================================

static inline uint8_t protocol_get_type(const packet_t* pkt) {
    return pkt->raw[0];
}

static inline uint8_t protocol_get_node_id(const packet_t* pkt) {
    return pkt->raw[1];
}

static inline bool protocol_is_sensor_packet(const packet_t* pkt) {
    return pkt->raw[0] == PKT_TYPE_SENSOR_DATA;
}

static inline bool protocol_is_config_packet(const packet_t* pkt) {
    return pkt->raw[0] == PKT_TYPE_CONFIG_UPDATE;
}

#endif // _PROTOCOL_H_
