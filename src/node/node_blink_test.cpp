/**
 * @file node_blink_test.cpp
 * @brief Super simple blink test - no Serial, just LED
 */

#include <Arduino.h>

#define PIN_LED_TEST    13    // Built-in LED on Feather M0

void setup() {
  pinMode(PIN_LED_TEST, OUTPUT);
  digitalWrite(PIN_LED_TEST, HIGH);  // Turn on and leave on
}

void loop() {
  digitalWrite(PIN_LED_TEST, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_TEST, LOW);
  delay(1000);
}
