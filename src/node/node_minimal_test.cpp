/**
 * @file node_minimal_test.cpp
 * @brief Minimal test for HelTec V4 - no radio init
 * 
 * This verifies the board works without radio initialization.
 */

#include <Arduino.h>

#define PIN_LED           35          // HelTec V4 LED
#define TX_INTERVAL_SEC   30

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("\n=== MINIMAL NODE TEST ===");
  Serial.println("Board: HelTec V4 (ESP32-S3R2)");
  Serial.println("LED: GPIO 35");
  Serial.println("Radio: DISABLED (test mode)\n");
  
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  Serial.println("\n[TX] Sending fake sensor data...");
  
  // Blink LED
  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);
  
  Serial.println("  LED blinked!");
  Serial.println("  (Radio would transmit here)");
  Serial.printf("  Sleeping for %d seconds...\n", TX_INTERVAL_SEC);
  
  // Short sleep for testing (normally 30 min)
  delay(5000);
  
  Serial.println("\n[WAKE] Woke from sleep!");
}
