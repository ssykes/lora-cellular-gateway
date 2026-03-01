/**
 * @file env_vars.cpp
 * @brief Dynamic environment variable management implementation
 * @version 3.1 - Simplified (no macros)
 * @date 2026-02-28
 */

#include "env_vars.h"
#include <Arduino.h>
#include <string.h>

// ============================================================================
// Global Configuration Instance
// ============================================================================

/** Gateway configuration (populated from environment variables) */
static gateway_config_t g_gateway_config;

// ============================================================================
// Environment Variable Registry (Auto-Generated)
// ============================================================================

/**
 * @brief Helper macro to generate registry entry
 */
// ============================================================================
// Environment Variable Registry (MANUAL - add new variables here)
// ============================================================================

/**
 * @brief Environment variable registry
 * 
 * TO ADD A NEW VARIABLE:
 * 1. Add field to gateway_config_t in env_vars.h
 * 2. Add entry below (follow existing format)
 * 3. Variable will auto-sync from Notehub!
 */
static env_var_def_t env_vars_registry[] = {
    // Listen Window
    {"gateway_listen_start", ENV_TYPE_UINT8, &g_gateway_config.gateway_listen_start, 8, 0, 0, 23, 0, 0, {true, true, false}},
    {"gateway_listen_end", ENV_TYPE_UINT8, &g_gateway_config.gateway_listen_end, 22, 0, 0, 23, 0, 0, {true, true, false}},
    
    // Sync Settings
    {"gateway_sync_interval", ENV_TYPE_UINT16, &g_gateway_config.gateway_sync_interval, 360, 0, 60, 1440, 0, 0, {true, true, false}},
    
    // Radio Settings
    {"lora_frequency", ENV_TYPE_FLOAT, &g_gateway_config.lora_frequency, 0, 915.0f, 0, 0, 868.0f, 927.0f, {true, true, false}},
    {"lora_tx_power", ENV_TYPE_UINT8, &g_gateway_config.lora_tx_power, 14, 0, 2, 17, 0, 0, {true, true, false}},
    {"lora_spreading_factor", ENV_TYPE_UINT8, &g_gateway_config.lora_spreading_factor, 10, 0, 7, 12, 0, 0, {true, true, false}},
    
    // Node Settings
    {"node_tx_interval", ENV_TYPE_UINT8, &g_gateway_config.node_tx_interval, 30, 0, 5, 120, 0, 0, {true, true, false}},
    {"node_threshold", ENV_TYPE_UINT8, &g_gateway_config.node_threshold, 50, 0, 0, 100, 0, 0, {true, true, false}},
    
    // System
    {"config_version", ENV_TYPE_UINT8, &g_gateway_config.config_version, 1, 0, 1, 255, 0, 0, {true, true, false}},
    {"config_changed", ENV_TYPE_BOOL, &g_gateway_config.config_changed, 0, 0, 0, 1, 0, 0, {true, true, false}},
    
    // Sentinel (marks end of registry)
    {NULL, ENV_TYPE_UINT8, NULL, 0, 0, 0, 0, 0, 0, {false, false, false}}
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Get number of registered variables (excluding sentinel)
 */
size_t env_vars_get_count(void) {
    size_t count = 0;
    while (env_vars_registry[count].name != NULL && count < ENV_VARS_MAX_COUNT) {
        count++;
    }
    return count;
}

/**
 * @brief Get variable definition by index
 */
const env_var_def_t* env_vars_get_by_index(size_t index) {
    if (index >= env_vars_get_count()) {
        return NULL;
    }
    return &env_vars_registry[index];
}

/**
 * @brief Get variable definition by name
 */
const env_var_def_t* env_vars_get_by_name(const char* name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < ENV_VARS_MAX_COUNT; i++) {
        if (env_vars_registry[i].name == NULL) {
            break;  // Reached sentinel
        }
        
        if (strcmp(env_vars_registry[i].name, name) == 0) {
            return &env_vars_registry[i];
        }
    }
    
    return NULL;
}

/**
 * @brief Get current hour
 */
uint8_t env_vars_get_current_hour(void) {
    // TODO: Read from nRF52840 RTC
    // For testing, return noon (inside active window)
    return 12;
}

/**
 * @brief Check if currently in listen window
 */
bool env_vars_is_listen_window(gateway_config_t* config) {
    if (config == NULL) {
        return false;
    }
    
    uint8_t hour = env_vars_get_current_hour();
    
    // Get field pointers from registry
    const env_var_def_t* start_var = env_vars_get_by_name("gateway_listen_start");
    const env_var_def_t* end_var = env_vars_get_by_name("gateway_listen_end");
    
    if (start_var == NULL || end_var == NULL) {
        return true;  // Default to active if not configured
    }
    
    uint8_t start_hour = *((uint8_t*)start_var->storage);
    uint8_t end_hour = *((uint8_t*)end_var->storage);
    
    // Handle normal window (e.g., 08:00-22:00)
    if (start_hour < end_hour) {
        return (hour >= start_hour && hour < end_hour);
    } 
    // Handle overnight window (e.g., 22:00-06:00)
    else {
        return (hour >= start_hour || hour < end_hour);
    }
}

/**
 * @brief Check if it's time to sync environment variables
 */
bool env_vars_should_sync(gateway_config_t* config) {
    if (config == NULL) {
        return false;
    }
    
    // Get sync interval from registry
    const env_var_def_t* interval_var = env_vars_get_by_name("gateway_sync_interval");
    if (interval_var == NULL) {
        return true;  // Default to sync if not configured
    }
    
    uint16_t interval_minutes = *((uint16_t*)interval_var->storage);
    
    // Static variable to track last sync
    static uint32_t last_sync = 0;
    
    // First sync
    if (last_sync == 0) {
        last_sync = millis();
        return true;
    }
    
    uint32_t now = millis();
    uint32_t interval_ms = (uint32_t)interval_minutes * 60000UL;
    
    // Check for overflow
    if (now < last_sync) {
        last_sync = now;
        return true;
    }
    
    if (now - last_sync >= interval_ms) {
        last_sync = now;
        return true;
    }
    
    return false;
}

// ============================================================================
// Notehub Communication Helpers
// ============================================================================
// TODO: Extract to notecard_comm.h/cpp when we have 3+ Notecard functions
//       Potential future functions:
//       - notecard_get_signal_strength()
//       - notecard_get_diagnostics()
//       - notecard_list_files()
//       - notecard_upload_firmware()
// ============================================================================

/**
 * @brief Read and validate integer environment variable
 */
static int32_t read_env_int(Notecard& notecard, const env_var_def_t* var_def) {
    J *req = notecard.newRequest("env.get");
    if (req == NULL) {
        return var_def->default_int;
    }
    
    JAddStringToObject(req, "name", var_def->name);
    J *rsp = notecard.requestAndResponse(req);
    
    if (rsp != NULL) {
        const char* text = JGetString(rsp, "text");
        if (text != NULL && text[0] != '\0') {
            int32_t value = atoi(text);
            notecard.deleteResponse(rsp);
            
            // Validate range
            if (var_def->flags.has_min && value < var_def->min_int) {
                return var_def->min_int;
            }
            if (var_def->flags.has_max && value > var_def->max_int) {
                return var_def->max_int;
            }
            
            return value;
        }
        notecard.deleteResponse(rsp);
    }
    
    return var_def->default_int;
}

/**
 * @brief Read and validate float environment variable
 */
static float read_env_float(Notecard& notecard, const env_var_def_t* var_def) {
    J *req = notecard.newRequest("env.get");
    if (req == NULL) {
        return var_def->default_float;
    }
    
    JAddStringToObject(req, "name", var_def->name);
    J *rsp = notecard.requestAndResponse(req);
    
    if (rsp != NULL) {
        const char* text = JGetString(rsp, "text");
        if (text != NULL && text[0] != '\0') {
            float value = atof(text);
            notecard.deleteResponse(rsp);
            
            // Validate range
            if (var_def->flags.has_min && value < var_def->min_float) {
                return var_def->min_float;
            }
            if (var_def->flags.has_max && value > var_def->max_float) {
                return var_def->max_float;
            }
            
            return value;
        }
        notecard.deleteResponse(rsp);
    }
    
    return var_def->default_float;
}

/**
 * @brief Apply default values to all variables
 */
static void apply_defaults(void) {
    for (size_t i = 0; i < ENV_VARS_MAX_COUNT; i++) {
        env_var_def_t* var = &env_vars_registry[i];
        
        if (var->name == NULL) {
            break;  // Reached sentinel
        }
        
        if (var->storage == NULL) {
            continue;
        }
        
        // Apply default based on type
        switch (var->type) {
            case ENV_TYPE_UINT8:
                *((uint8_t*)var->storage) = (uint8_t)var->default_int;
                break;
            case ENV_TYPE_UINT16:
                *((uint16_t*)var->storage) = (uint16_t)var->default_int;
                break;
            case ENV_TYPE_INT32:
                *((int32_t*)var->storage) = var->default_int;
                break;
            case ENV_TYPE_FLOAT:
                *((float*)var->storage) = var->default_float;
                break;
            case ENV_TYPE_BOOL:
                *((bool*)var->storage) = (var->default_int != 0);
                break;
        }
    }
}

/**
 * @brief Read variable from Notehub and update storage
 * @return true if value changed
 */
static bool read_and_update(Notecard& notecard, env_var_def_t* var_def) {
    if (var_def->flags.read_only) {
        return false;
    }
    
    // Save old value for comparison
    uint8_t old_uint8 = 0;
    uint16_t old_uint16 = 0;
    int32_t old_int32 = 0;
    float old_float = 0;
    bool has_old = false;
    
    if (var_def->storage != NULL) {
        switch (var_def->type) {
            case ENV_TYPE_UINT8:
                old_uint8 = *((uint8_t*)var_def->storage);
                has_old = true;
                break;
            case ENV_TYPE_UINT16:
                old_uint16 = *((uint16_t*)var_def->storage);
                has_old = true;
                break;
            case ENV_TYPE_INT32:
                old_int32 = *((int32_t*)var_def->storage);
                has_old = true;
                break;
            case ENV_TYPE_FLOAT:
                old_float = *((float*)var_def->storage);
                has_old = true;
                break;
            default:
                break;
        }
    }
    
    // Read new value based on type
    switch (var_def->type) {
        case ENV_TYPE_UINT8:
        case ENV_TYPE_UINT16:
        case ENV_TYPE_INT32:
        case ENV_TYPE_BOOL: {
            int32_t new_value = read_env_int(notecard, var_def);
            if (var_def->storage != NULL) {
                switch (var_def->type) {
                    case ENV_TYPE_UINT8:
                        *((uint8_t*)var_def->storage) = (uint8_t)new_value;
                        break;
                    case ENV_TYPE_UINT16:
                        *((uint16_t*)var_def->storage) = (uint16_t)new_value;
                        break;
                    case ENV_TYPE_INT32:
                        *((int32_t*)var_def->storage) = new_value;
                        break;
                    case ENV_TYPE_BOOL:
                        *((bool*)var_def->storage) = (new_value != 0);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        
        case ENV_TYPE_FLOAT: {
            float new_value = read_env_float(notecard, var_def);
            if (var_def->storage != NULL) {
                *((float*)var_def->storage) = new_value;
            }
            break;
        }
        
        default:
            break;
    }
    
    // Check if value changed
    if (has_old && var_def->storage != NULL) {
        switch (var_def->type) {
            case ENV_TYPE_UINT8:
                return (*((uint8_t*)var_def->storage) != old_uint8);
            case ENV_TYPE_UINT16:
                return (*((uint16_t*)var_def->storage) != old_uint16);
            case ENV_TYPE_INT32:
                return (*((int32_t*)var_def->storage) != old_int32);
            case ENV_TYPE_FLOAT:
                return (*((float*)var_def->storage) != old_float);
            default:
                return false;
        }
    }
    
    return false;
}

// ============================================================================
// Public Functions
// ============================================================================

bool env_vars_init(Notecard& notecard, gateway_config_t* config) {
    if (config == NULL) {
        return false;
    }
    
    // Store global pointer
    memcpy(&g_gateway_config, config, sizeof(gateway_config_t));
    
    // Apply defaults first
    apply_defaults();
    
    // Read all variables from Notehub
    for (size_t i = 0; i < ENV_VARS_MAX_COUNT; i++) {
        env_var_def_t* var = &env_vars_registry[i];
        
        if (var->name == NULL) {
            break;  // Reached sentinel
        }
        
        read_and_update(notecard, var);
    }
    
    // Copy back to caller's config
    memcpy(config, &g_gateway_config, sizeof(gateway_config_t));
    
    return true;
}

bool env_vars_sync(Notecard& notecard, gateway_config_t* config) {
    if (config == NULL) {
        return false;
    }
    
    // Get config_version from registry
    const env_var_def_t* version_var = env_vars_get_by_name("config_version");
    uint8_t old_version = 0;
    if (version_var != NULL && version_var->storage != NULL) {
        old_version = *((uint8_t*)version_var->storage);
    }
    
    // Read all variables from Notehub
    for (size_t i = 0; i < ENV_VARS_MAX_COUNT; i++) {
        env_var_def_t* var = &env_vars_registry[i];
        
        if (var->name == NULL) {
            break;  // Reached sentinel
        }
        
        read_and_update(notecard, var);
    }
    
    // Copy back to caller's config
    memcpy(config, &g_gateway_config, sizeof(gateway_config_t));
    
    // Check if version changed
    bool changed = false;
    if (version_var != NULL && version_var->storage != NULL) {
        changed = (*((uint8_t*)version_var->storage) != old_version);
    }
    
    config->config_changed = changed;
    
    // TODO: Broadcast config to nodes if changed (Phase 2)
    // When node config fields are added to gateway_config_t:
    // if (changed) {
    //     node_config_payload_t node_config = {
    //         .config_version = config->config_version,
    //         .tx_interval_minutes = config->node_tx_interval,
    //         .threshold = config->node_threshold,
    //         .tx_power_dbm = config->node_tx_power
    //     };
    //     protocol_build_node_config_packet(&pkt, NODE_ID_BROADCAST, &node_config);
    //     radio.transmit(pkt.raw, NODE_CONFIG_PACKET_SIZE);
    // }
    
    return changed;
}

/**
 * @brief Print all environment variables (debug helper)
 */
void env_vars_print_config(gateway_config_t* config) {
    if (config == NULL) {
        return;
    }
    
    Serial.println("=== Gateway Configuration ===");
    
    for (size_t i = 0; i < ENV_VARS_MAX_COUNT; i++) {
        env_var_def_t* var = &env_vars_registry[i];
        
        if (var->name == NULL) {
            break;  // Reached sentinel
        }
        
        Serial.print(var->name);
        Serial.print(": ");
        
        if (var->storage != NULL) {
            switch (var->type) {
                case ENV_TYPE_UINT8:
                    Serial.printf("%d", *((uint8_t*)var->storage));
                    break;
                case ENV_TYPE_UINT16:
                    Serial.printf("%d", *((uint16_t*)var->storage));
                    break;
                case ENV_TYPE_INT32:
                    Serial.printf("%ld", *((int32_t*)var->storage));
                    break;
                case ENV_TYPE_FLOAT:
                    Serial.printf("%.1f", *((float*)var->storage));
                    break;
                case ENV_TYPE_BOOL:
                    Serial.printf("%s", *((bool*)var->storage) ? "true" : "false");
                    break;
                default:
                    Serial.print("?");
                    break;
            }
        }
        Serial.println();
    }
    
    // Print version if available
    const env_var_def_t* version_var = env_vars_get_by_name("config_version");
    if (version_var != NULL && version_var->storage != NULL) {
        Serial.printf("Config Version: %d\n", *((uint8_t*)version_var->storage));
    }
    
    Serial.printf("Config Changed: %s\n", config->config_changed ? "YES" : "NO");
    Serial.println("=============================\n");
}
