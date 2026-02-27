/**
 * @file config_types.h
 * @brief Configuration data structures for gateway and nodes
 * @version 1.0
 * @date 2026-02-24
 * 
 * Shared configuration structures stored in Flash and synced via Blues.
 */

#ifndef _CONFIG_TYPES_H_
#define _CONFIG_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Configuration Magic Numbers (for Flash validation)
// ============================================================================

#define CONFIG_MAGIC_GATEWAY      0x47574154  // "GWAT" in ASCII
#define CONFIG_MAGIC_NODE         0x4E4F4445  // "NODE" in ASCII
#define CONFIG_VERSION_CURRENT    1           // Config structure version

// ============================================================================
// Gateway Configuration
// ============================================================================

/**
 * @brief Gateway configuration structure
 * 
 * Stored in Flash, updated via Blues mailbox
 */
typedef struct {
    uint32_t magic;               // CONFIG_MAGIC_GATEWAY
    uint8_t config_version;       // Structure version (not network config version)
    
    // Listen window settings
    uint8_t listen_start_hour;    // 0-23 (default: 8 = 08:00)
    uint8_t listen_end_hour;      // 0-23 (default: 22 = 22:00)
    uint8_t timezone_offset;      // UTC offset in hours (e.g., -5 for EST)
    
    // Wake cycle settings
    uint16_t wake_interval_minutes;  // How often to check Blues (default: 15)
    uint16_t listen_window_seconds;  // How long to listen for packets (default: 30)
    
    // Radio settings
    int8_t tx_power_dbm;          // TX power (default: 14)
    float lora_frequency;         // LoRa frequency in MHz (default: 915.0)

    // Network config version (incremented when node config changes)
    uint8_t global_config_version; // Current network config version (1-255)

    // Padding for alignment
    uint8_t reserved[1];
    
    // CRC for integrity
    uint16_t crc;
} __attribute__((packed)) gateway_config_t;

#define GATEWAY_CONFIG_SIZE sizeof(gateway_config_t)

/**
 * @brief Default gateway configuration
 */
static const gateway_config_t g_gateway_config_default = {
    .magic = CONFIG_MAGIC_GATEWAY,
    .config_version = CONFIG_VERSION_CURRENT,
    .listen_start_hour = 8,
    .listen_end_hour = 22,
    .timezone_offset = 0,           // UTC (adjust for local timezone)
    .wake_interval_minutes = 15,
    .listen_window_seconds = 30,
    .tx_power_dbm = 14,
    .lora_frequency = LORA_FREQUENCY_DEFAULT,
    .global_config_version = 1,
    .reserved = {0},
    .crc = 0                        // Calculated at runtime
};

// ============================================================================
// Node Configuration
// ============================================================================

/**
 * @brief Node configuration structure
 * 
 * Stored in Flash on each node, updated by gateway
 */
typedef struct {
    uint32_t magic;               // CONFIG_MAGIC_NODE
    uint8_t config_version;       // Structure version (not network config version)
    
    // Node identity
    uint8_t node_id;              // Unique node ID (1-255, 0 = gateway)
    
    // Transmission settings
    uint32_t tx_interval_minutes; // Time between transmissions (default: 30)
    
    // Relay settings
    bool relay_enabled;           // Whether to relay other nodes' packets
    uint8_t max_hops;             // Maximum relay hops (default: 3)
    
    // Radio settings
    int8_t tx_power_dbm;          // TX power (default: 14)
    
    // Network config version (synced with gateway)
    uint8_t network_config_version; // Current network config version
    
    // Sensor settings (for future expansion)
    uint8_t sensor_type;          // 0 = default, 1-255 = custom sensor types
    uint8_t sensor_flags;         // Bit flags for sensor behavior
    
    // Padding for alignment
    uint8_t reserved[2];
    
    // CRC for integrity
    uint16_t crc;
} __attribute__((packed)) node_config_t;

#define NODE_CONFIG_SIZE sizeof(node_config_t)

/**
 * @brief Default node configuration
 */
static const node_config_t g_node_config_default = {
    .magic = CONFIG_MAGIC_NODE,
    .config_version = CONFIG_VERSION_CURRENT,
    .node_id = 1,                 // Should be set per-node
    .tx_interval_minutes = 30,
    .relay_enabled = false,
    .max_hops = 3,
    .tx_power_dbm = 14,
    .network_config_version = 1,
    .sensor_type = 0,
    .sensor_flags = 0,
    .reserved = {0, 0},
    .crc = 0                        // Calculated at runtime
};

// ============================================================================
// Node Configuration Flags
// ============================================================================

#define NODE_FLAG_BATTERY_LOW     0x01    // Battery below threshold
#define NODE_FLAG_SENSOR_ERROR    0x02    // Sensor read error
#define NODE_FLAG_RELAY_ACTIVE    0x04    // Currently relaying packets
#define NODE_FLAG_CONFIG_PENDING  0x08    // Config update pending

// ============================================================================
// Per-Node Config Cache (Gateway Only)
// ============================================================================

/**
 * @brief Per-node configuration cache (gateway maintains this)
 * 
 * Tracks last-known config version for each node.
 * Used to determine if config update is needed.
 */
typedef struct {
    uint8_t node_id;              // Node ID (0 = unused entry)
    uint8_t last_known_version;   // Last config version sent to this node
    uint32_t last_seen_timestamp; // Last time node transmitted (Unix time)
    int8_t last_rssi;             // Last RSSI from this node
    uint8_t reserved[2];
} node_cache_entry_t;

#define MAX_NODE_CACHE_ENTRIES  16    // Cache up to 16 nodes

/**
 * @brief Node cache structure (gateway only)
 */
typedef struct {
    uint32_t magic;               // CONFIG_MAGIC_GATEWAY
    uint8_t entry_count;          // Number of valid entries
    uint8_t reserved[3];
    node_cache_entry_t entries[MAX_NODE_CACHE_ENTRIES];
    uint16_t crc;
} __attribute__((packed)) node_cache_t;

// ============================================================================
// Blues Mailbox Structures
// ============================================================================

/**
 * @brief Blues mailbox config note structure
 * 
 * This is the JSON structure expected in Blues Notehub config.qo
 */
typedef struct {
    // Gateway settings
    uint8_t listen_start_hour;
    uint8_t listen_end_hour;
    uint8_t timezone_offset;
    uint16_t wake_interval_minutes;
    uint16_t listen_window_seconds;
    int8_t tx_power_dbm;
    
    // Global config version
    uint8_t global_config_version;
    
    // Node count
    uint8_t node_count;
    
    // Node configurations (variable length, max 16)
    struct {
        uint8_t node_id;
        uint32_t tx_interval_minutes;
        bool relay_enabled;
        int8_t tx_power_dbm;
    } nodes[16];
} blues_config_note_t;

// ============================================================================
// Flash Storage Addresses (Board Dependent)
// ============================================================================

// These are example addresses - adjust for specific MCU
// nRF52840: 1 MB Flash, last 64 KB reserved for config
// ESP32: 4 MB Flash, use NVS partition
// SAMD21: 256 KB Flash, last 16 KB reserved

#if defined(NRF52840_XXAA)
    // nRF52840 - last 64 KB of Flash
    #define FLASH_CONFIG_ADDR_GATEWAY   0x000FC000
    #define FLASH_CONFIG_ADDR_NODE      0x000FC000
    #define FLASH_CONFIG_ADDR_CACHE     0x000FE000

#elif defined(CONFIG_IDF_TARGET_ESP32)
    // ESP32 - use NVS or custom partition
    #define FLASH_CONFIG_ADDR_GATEWAY   0x3F0000    // Example address
    #define FLASH_CONFIG_ADDR_NODE      0x3F0000
    #define FLASH_CONFIG_ADDR_CACHE     0x3F1000

#elif defined(__SAMD21G18A__)
    // SAMD21 - last 16 KB of Flash
    #define FLASH_CONFIG_ADDR_GATEWAY   0x0003C000
    #define FLASH_CONFIG_ADDR_NODE      0x0003C000
    #define FLASH_CONFIG_ADDR_CACHE     0x0003E000

#else
    // Default (override in board-specific code)
    #define FLASH_CONFIG_ADDR_GATEWAY   0x0003C000
    #define FLASH_CONFIG_ADDR_NODE      0x0003C000
    #define FLASH_CONFIG_ADDR_CACHE     0x0003E000
#endif

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * @brief Initialize configuration from Flash
 * @return 0 Success, negative error code
 */
int config_init(void);

/**
 * @brief Save configuration to Flash
 * @return 0 Success, negative error code
 */
int config_save(void);

/**
 * @brief Load configuration from Flash
 * @return 0 Success, negative error code
 */
int config_load(void);

/**
 * @brief Reset configuration to defaults
 * @return 0 Success, negative error code
 */
int config_reset(void);

/**
 * @brief Verify configuration CRC
 * @param config Pointer to config structure
 * @return true CRC valid
 * @return false CRC invalid
 */
bool config_verify_crc(const void* config);

/**
 * @brief Calculate configuration CRC
 * @param config Pointer to config structure (CRC field should be 0)
 * @return uint16_t CRC value
 */
uint16_t config_calc_crc(const void* config);

// ============================================================================
// Inline Helper Functions
// ============================================================================

static inline bool config_is_valid_gateway(const gateway_config_t* cfg) {
    return (cfg->magic == CONFIG_MAGIC_GATEWAY) &&
           (cfg->config_version == CONFIG_VERSION_CURRENT) &&
           config_verify_crc(cfg);
}

static inline bool config_is_valid_node(const node_config_t* cfg) {
    return (cfg->magic == CONFIG_MAGIC_NODE) &&
           (cfg->config_version == CONFIG_VERSION_CURRENT) &&
           config_verify_crc(cfg);
}

static inline uint8_t config_increment_version(uint8_t version) {
    return (version >= 255) ? 1 : version + 1;
}

#endif // _CONFIG_TYPES_H_
