/**
 * @file env_vars.h
 * @brief Dynamic environment variable management for LoRa Gateway
 * @version 3.1 - Simplified (no macros)
 * @date 2026-02-28
 *
 * To add a new environment variable:
 * 1. Add field to gateway_config_t struct (below)
 * 2. Add entry to env_vars_registry[] in env_vars.cpp
 * 3. Set variable in Notehub dashboard
 * 4. It will automatically sync!
 */

#ifndef _ENV_VARS_H_
#define _ENV_VARS_H_

#include <Notecard.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// Configuration: Maximum Environment Variables
// ============================================================================

/** Maximum number of environment variables supported */
#define ENV_VARS_MAX_COUNT      16

/** Maximum length for string values */
#define ENV_VARS_MAX_STRING_LEN 64

// ============================================================================
// Environment Variable Data Types
// ============================================================================

/**
 * @brief Supported data types for environment variables
 */
typedef enum {
    ENV_TYPE_UINT8,     /**< Unsigned 8-bit integer */
    ENV_TYPE_UINT16,    /**< Unsigned 16-bit integer */
    ENV_TYPE_INT32,     /**< Signed 32-bit integer */
    ENV_TYPE_FLOAT,     /**< Floating point value */
    ENV_TYPE_BOOL       /**< Boolean value */
} env_var_type_t;

// ============================================================================
// Environment Variable Structure
// ============================================================================

/**
 * @brief Environment variable definition
 */
typedef struct {
    const char* name;           /**< Variable name (must match Notehub) */
    env_var_type_t type;        /**< Data type */
    void* storage;              /**< Pointer to storage location */
    int32_t default_int;        /**< Default value for int types */
    float default_float;        /**< Default value for float types */
    int32_t min_int;            /**< Minimum value for int types */
    int32_t max_int;            /**< Maximum value for int types */
    float min_float;            /**< Minimum value for float types */
    float max_float;            /**< Maximum value for float types */
    struct {
        bool has_min : 1;
        bool has_max : 1;
        bool read_only : 1;
    } flags;
} env_var_def_t;

// ============================================================================
// Gateway Configuration Structure (Auto-Generated from env_vars_list.h)
// ============================================================================

/**
 * @brief Gateway runtime configuration
 *
 * All configurable parameters stored in one structure.
 * Populated from Blues Notehub environment variables.
 * 
 * TODO: Add node configuration fields (Phase 2):
 * - uint8_t node_tx_interval;    // Node TX interval (minutes)
 * - uint8_t node_threshold;      // Node trigger threshold (0-100)
 * - uint8_t node_tx_power;       // Node TX power (dBm)
 * 
 * TODO: Add power management fields (Phase 3):
 * - uint8_t listen_start_hour;   // Listen window start (08:00)
 * - uint8_t listen_end_hour;     // Listen window end (22:00)
 * - bool quiet_hours_enabled;    // Enable quiet hours
 */
typedef struct {
    // Listen Window
    uint8_t gateway_listen_start;
    uint8_t gateway_listen_end;
    
    // Sync Settings
    uint16_t gateway_sync_interval;
    
    // Radio Settings
    float lora_frequency;
    uint8_t lora_tx_power;
    uint8_t lora_spreading_factor;
    
    // Node Settings
    uint8_t node_tx_interval;
    uint8_t node_threshold;
    
    // System
    uint8_t config_version;
    bool config_changed;
} gateway_config_t;

// ============================================================================
// Type Helpers (Auto-Generated)
// ============================================================================

/**
 * @brief Get type enum for a C++ type
 */
template<typename T>
inline env_var_type_t get_type_enum() {
    return ENV_TYPE_UINT8;  // Default
}

template<> inline env_var_type_t get_type_enum<uint8_t>() { return ENV_TYPE_UINT8; }
template<> inline env_var_type_t get_type_enum<uint16_t>() { return ENV_TYPE_UINT16; }
template<> inline env_var_type_t get_type_enum<int32_t>() { return ENV_TYPE_INT32; }
template<> inline env_var_type_t get_type_enum<float>() { return ENV_TYPE_FLOAT; }
template<> inline env_var_type_t get_type_enum<bool>() { return ENV_TYPE_BOOL; }

// ============================================================================
// Function Declarations
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize environment variable system
 *
 * Registers all variables and loads initial values from Notehub.
 * Must be called once at boot.
 *
 * @param notecard Pointer to Notecard instance
 * @param config Pointer to gateway configuration structure
 * @return true if successful, false if Notehub not reachable
 */
bool env_vars_init(Notecard& notecard, gateway_config_t* config);

/**
 * @brief Sync environment variables from Notehub
 *
 * Polls Notehub for all registered variables.
 * Updates config structure if values changed.
 *
 * @param notecard Pointer to Notecard instance
 * @param config Pointer to gateway configuration structure
 * @return true if config changed, false if no change
 */
bool env_vars_sync(Notecard& notecard, gateway_config_t* config);

/**
 * @brief Check if it's time to sync environment variables
 *
 * @param config Pointer to gateway configuration structure
 * @return true if sync is due, false otherwise
 */
bool env_vars_should_sync(gateway_config_t* config);

/**
 * @brief Check if currently in listen window
 *
 * @param config Pointer to gateway configuration structure
 * @return true if in listen window, false if quiet hours
 */
bool env_vars_is_listen_window(gateway_config_t* config);

/**
 * @brief Get current hour (from RTC or system time)
 *
 * TODO: Implement RTC read for nRF52840
 *
 * @return Current hour (0-23)
 */
uint8_t env_vars_get_current_hour(void);

/**
 * @brief Print all environment variables (debug helper)
 *
 * @param config Pointer to gateway configuration structure
 */
void env_vars_print_config(gateway_config_t* config);

/**
 * @brief Get number of registered environment variables
 *
 * @return Number of registered variables
 */
size_t env_vars_get_count(void);

/**
 * @brief Get variable definition by index
 *
 * @param index Variable index (0 to count-1)
 * @return Pointer to variable definition, or NULL if out of range
 */
const env_var_def_t* env_vars_get_by_index(size_t index);

/**
 * @brief Get variable definition by name
 *
 * @param name Variable name
 * @return Pointer to variable definition, or NULL if not found
 */
const env_var_def_t* env_vars_get_by_name(const char* name);

#ifdef __cplusplus
}
#endif

#endif // _ENV_VARS_H_
