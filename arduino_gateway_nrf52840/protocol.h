/**
 * @file protocol.h
 * @brief Binary protocol definition for LoRa sensor network
 */

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PKT_TYPE_SENSOR_DATA    0x01
#define PKT_TYPE_CONFIG_UPDATE  0x02
#define PKT_TYPE_ACK            0x03
#define PKT_TYPE_RELAY          0x04

#define CONFIG_VERSION_INITIAL  1

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

#define MAX_PACKET_SIZE       20

typedef union {
    uint8_t raw[MAX_PACKET_SIZE];
    sensor_packet_t sensor;
    config_packet_t config;
} packet_t;

uint16_t protocol_calc_crc(const uint8_t* data, size_t length);
bool protocol_verify_crc(const uint8_t* packet, size_t length);
void protocol_build_sensor_packet(packet_t* pkt, uint8_t node_id,
                                   uint8_t config_ver, sensor_payload_t* payload);
void protocol_build_config_packet(packet_t* pkt, uint8_t node_id,
                                   config_payload_t* config);

#endif
