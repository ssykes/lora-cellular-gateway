/**
 * @file node_blink_test.cpp
 * @brief Simple blink test for HelTec V4 (ESP32-S3R2)
 * 
 * This test verifies the board is working before debugging the full node code.
 */

#include <Arduino.h>

// ============================================================================
// Configuration
// ============================================================================

// Try multiple LED pins (HelTec V4 may use different GPIO)
#define LED_PINS          {15, 35, 36, 37, 47, 48}  // Common ESP32-S3 LED pins
#define BLINK_DELAY_MS    200
#define BLINK_COUNT       5

// ============================================================================
// Setup
// ============================================================================

void setup() {
  // Initialize serial (ESP32-S3 USB CDC)
  Serial.begin(115200);
  
  // Wait for USB to enumerate
  delay(3000);
  
  Serial.println("\n=== BLINK TEST STARTING ===");
  Serial.println("Testing multiple LED pins...\n");
  
  // Initialize all candidate LED pins
  int led_pins[] = LED_PINS;
  int num_pins = sizeof(led_pins) / sizeof(led_pins[0]);
  
  for (int i = 0; i < num_pins; i++) {
    pinMode(led_pins[i], OUTPUT);
    digitalWrite(led_pins[i], LOW);
    Serial.printf("Initialized GPIO %d as output\n", led_pins[i]);
  }
  
  Serial.println("\nStarting blink test on each pin...\n");
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
  int led_pins[] = LED_PINS;
  int num_pins = sizeof(led_pins) / sizeof(led_pins[0]);
  
  // Blink each pin
  for (int p = 0; p < num_pins; p++) {
    Serial.printf("\n--- Testing GPIO %d ---\n", led_pins[p]);
    
    for (int i = 0; i < BLINK_COUNT; i++) {
      digitalWrite(led_pins[p], HIGH);
      delay(BLINK_DELAY_MS);
      digitalWrite(led_pins[p], LOW);
      delay(BLINK_DELAY_MS);
    }
    
    Serial.printf("GPIO %d complete\n", led_pins[p]);
    Serial.println("Did you see an LED blink? (y/n)");
    
    // Short pause before next pin
    delay(1000);
  }
  
  Serial.println("\n=== ALL PINS TESTED ===");
  Serial.println("Check which GPIO made an LED blink.");
  Serial.println("That's your LED_BUILTIN pin!\n");
  
  // Wait before repeating
  delay(10000);
  
  Serial.println("Starting another test cycle...\n");
}
