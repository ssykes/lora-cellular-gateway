/**
 * @file debug.h
 * @brief Debug output macros for conditional compilation
 * @version 1.0
 * @date 2026-02-26
 *
 * Usage:
 * ======
 * 
 * Option 1: Define in source file (before including debug.h)
 *   #define DEBUG 1  // Enable debug output
 *   #define DEBUG 0  // Disable debug output (reduces code size)
 *   #include "debug.h"
 *
 * Option 2: Define in platformio.ini
 *   build_flags = -D DEBUG=1   ; Enable
 *   build_flags = -D DEBUG=0   ; Disable
 *
 * Option 3: Use default (DEBUG=1 if not defined)
 *
 * Debug output macros:
 *   DEBUG_SERIAL.println("Message");
 *   DEBUG_SERIAL.printf("Value: %d\n", value);
 *   DEBUG_SERIAL_IF(condition).printf("Conditional: %s\n", str);
 *   DEBUG_PRINT("Auto-newline message");
 *   DEBUG_PRINTF("Formatted: %d", value);
 *   DEBUG_HEX_DUMP(data, length);
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <Arduino.h>

// ============================================================================
// Debug Configuration
// ============================================================================

/**
 * Set DEBUG to 1 to enable debug output, 0 to disable
 * Can be overridden in platformio.ini build_flags (-D DEBUG=1)
 * Default: 0 (debug output disabled for production builds)
 */
#ifndef DEBUG
    #define DEBUG 0
#endif

// ============================================================================
// Debug Macros
// ============================================================================

#if DEBUG == 1
    /**
     * @brief Debug serial output (enabled)
     *
     * Usage:
     *   DEBUG_SERIAL.println("Message");
     *   DEBUG_SERIAL.printf("Value: %d\n", value);
     */
    #define DEBUG_SERIAL Serial

    /**
     * @brief Conditional debug output
     *
     * Usage:
     *   DEBUG_SERIAL_IF(my_condition).printf("Only if true: %d\n", val);
     */
    #define DEBUG_SERIAL_IF(condition) \
        if (condition) Serial

    /**
     * @brief Debug print with automatic newline
     *
     * Usage:
     *   DEBUG_PRINT("Message");
     *   DEBUG_PRINT("Value: ", value);
     */
    #define DEBUG_PRINT(...) Serial.println(__VA_ARGS__)

    /**
     * @brief Debug print without newline
     *
     * Usage:
     *   DEBUG_PRINT_RAW("Loading...");
     */
    #define DEBUG_PRINT_RAW(...) Serial.print(__VA_ARGS__)

    /**
     * @brief Debug printf (formatted)
     *
     * Usage:
     *   DEBUG_PRINTF("Node %d, RSSI: %d dBm\n", node_id, rssi);
     */
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)

    /**
     * @brief Debug hex dump
     *
     * Usage:
     *   DEBUG_HEX_DUMP(data, length);
     */
    #define DEBUG_HEX_DUMP(data, len) debug_hex_dump(Serial, data, len)

#else
    /**
     * @brief Debug serial output (disabled - compiles to nothing)
     *
     * All debug output is removed at compile time when DEBUG=0
     */
    #define DEBUG_SERIAL if (false) Serial
    #define DEBUG_SERIAL_IF(condition) if (false) Serial
    #define DEBUG_PRINT(...) ((void)0)
    #define DEBUG_PRINT_RAW(...) ((void)0)
    #define DEBUG_PRINTF(...) ((void)0)
    #define DEBUG_HEX_DUMP(data, len) ((void)0)
#endif

// ============================================================================
// Helper Functions
// ============================================================================

#if DEBUG == 1
/**
 * @brief Print hex dump of data (debug only)
 * @param stream Serial stream
 * @param data Data buffer
 * @param length Data length
 */
static inline void debug_hex_dump(Print& stream, const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (i > 0 && i % 16 == 0) {
            stream.println();
        } else if (i > 0 && i % 8 == 0) {
            stream.print("  ");
        }
        stream.printf("%02X ", data[i]);
    }
    stream.println();
}
#endif

#endif // _DEBUG_H_
