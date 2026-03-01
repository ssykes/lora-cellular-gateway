/**
 * @file protocol.h
 * @brief Binary protocol definition for LoRa sensor network
 * 
 * Protocol Version: 1.0
 * 
 * This file defines:
 * - Packet structures for LoRa communication
 * - CRC functions for data integrity
 * - Configuration types for gateway and nodes
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// Packet Type Definitions
// ============================================================================

/** Standard sensor data packet (Node → Gateway) */
#define PKT_TYPE_SENSOR_DATA    0x01

/** Gateway configuration update (reserved for future use) */
#define PKT_TYPE_CONFIG_UPDATE  0x02

/** Acknowledgment packet (optional, rarely used) */
#define PKT_TYPE_ACK            0x03

/** Relay packet (Node → Node, for multi-hop) */
#define PKT_TYPE_RELAY          0x04

/** Node configuration update (Gateway → Node) */
#define PKT_TYPE_NODE_CONFIG    0x05

// ============================================================================
// Protocol Constants
// ============================================================================

/** Initial configuration version */
#define CONFIG_VERSION_INITIAL  1

/** Maximum node ID (1-255, 0 = broadcast) */
#define MAX_NODE_ID             255

/** Broadcast node ID */
#define NODE_ID_BROADCAST       0

/** Gateway node ID (reserved) */
#define GATEWAY_NODE_ID         0

// ============================================================================
// Sensor Data Packet (Node → Gateway)
// ============================================================================

typedef struct {
    float temperature;
    float humidity;
    uint16_t battery_mv;
    uint8_t sensor_flags;
} __attribute__((packed)) sensor_payload_t;

#define SENSOR_FLAG_BATTERY_LOW   0x01
#define SENSOR_FLAG_SENSOR_ERROR  0x02
#define SENSOR_FLAG_RELAY_MODE    0x04

typedef struct {
    uint8_t type;
    uint8_t node_id;
    uint8_t hop_count;
    uint8_t rssi;
    uint8_t config_version;
    sensor_payload_t payload;
    uint16_t crc;
} __attribute__((packed)) sensor_packet_t;

#define SENSOR_PACKET_SIZE    sizeof(sensor_packet_t)

typedef struct {
    uint8_t relay_enabled : 1;
    uint8_t reserved : 7;
} __attribute__((packed)) config_flags_t;

typedef struct {
    config_flags_t flags;
    uint8_t config_version;
    uint16_t interval_minutes;
    uint8_t relay_enabled;
    uint8_t tx_power_dbm;
    uint8_t reserved;
} __attribute__((packed)) config_payload_t;

typedef struct {
    uint8_t type;
    uint8_t node_id;
    config_flags_t flags;
    uint8_t config_version;
    uint16_t interval_minutes;
    uint8_t relay_enabled;
    uint8_t tx_power_dbm;
    uint16_t crc;
} __attribute__((packed)) config_packet_t;

#define CONFIG_PACKET_SIZE    sizeof(config_packet_t)

// ============================================================================
// Node Configuration Packet (Gateway → Node)
// ============================================================================

/**
 * @brief Node configuration payload
 * 
 * Sent from gateway to nodes when configuration changes in Notehub.
 * Can be broadcast (node_id = 0) or unicast (node_id = 1-255).
 */
typedef struct {
    /** New configuration version (incremented when config changes) */
    uint8_t config_version;
    
    /** Transmission interval in minutes (5-120 min) */
    uint8_t tx_interval_minutes;
    
    /** Trigger threshold for event detection (0-100) */
    uint8_t threshold;
    
    /** Radio TX power in dBm (2-17 dBm) */
    uint8_t tx_power_dbm;
    
    /** Reserved for future expansion */
    uint8_t reserved[3];
} __attribute__((packed)) node_config_payload_t;

/**
 * @brief Node configuration update packet
 * 
 * Gateway broadcasts this when node config changes in Notehub.
 * Nodes receive, validate CRC, and save to Flash.
 */
typedef struct {
    /** Packet type (PKT_TYPE_NODE_CONFIG = 0x05) */
    uint8_t type;
    
    /** Target node ID (0 = broadcast, 1-255 = specific node) */
    uint8_t node_id;
    
    /** Configuration payload */
    node_config_payload_t config;
    
    /** 16-bit CRC for data integrity */
    uint16_t crc;
} __attribute__((packed)) node_config_packet_t;

/** Size of node config packet in bytes */
#define NODE_CONFIG_PACKET_SIZE   sizeof(node_config_packet_t)

// ============================================================================
// Generic Packet Union (for buffer allocation)
// ============================================================================

/** Maximum packet size in bytes (allocate buffers for largest packet) */
#define MAX_PACKET_SIZE       20

/**
 * @brief Generic packet union
 * 
 * Use this to allocate buffers that can hold any packet type.
 * Access specific packet types via the union members.
 */
typedef union {
    /** Raw byte access */
    uint8_t raw[MAX_PACKET_SIZE];
    
    /** Sensor data packet (Node → Gateway) */
    sensor_packet_t sensor;
    
    /** Gateway config packet (reserved) */
    config_packet_t config;
    
    /** Node config packet (Gateway → Node) */
    node_config_packet_t node_config;
} packet_t;

uint16_t protocol_calc_crc(const uint8_t* data, size_t length);
bool protocol_verify_crc(const uint8_t* packet, size_t length);

/**
 * @brief Build sensor data packet
 * 
 * @param pkt Pointer to packet buffer
 * @param node_id Node ID (1-255)
 * @param config_ver Current config version
 * @param payload Sensor data payload
 */
void protocol_build_sensor_packet(packet_t* pkt, uint8_t node_id,
                                   uint8_t config_ver, sensor_payload_t* payload);

/**
 * @brief Build gateway config packet (reserved for future use)
 * 
 * @param pkt Pointer to packet buffer
 * @param node_id Target node ID
 * @param config Gateway configuration
 */
void protocol_build_config_packet(packet_t* pkt, uint8_t node_id,
                                   config_payload_t* config);

/**
 * @brief Build node configuration update packet
 * 
 * Called when gateway detects config change in Notehub.
 * Broadcasts to all nodes or unicasts to specific node.
 * 
 * @param pkt Pointer to packet buffer
 * @param node_id Target node (0 = broadcast, 1-255 = specific)
 * @param config Node configuration to send
 */
void protocol_build_node_config_packet(packet_t* pkt, uint8_t node_id,
                                        node_config_payload_t* config);

#endif // _PROTOCOL_H_
