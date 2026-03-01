/**
 * @file node_led_test.cpp
 * @brief Test multiple LED pins
 */

#include <Arduino.h>

// Try different LED pins
#define LED_PIN_1   13      // Common default
#define LED_PIN_2   PIN_LED // Board definition
#define LED_PIN_3   6       // Some Feather boards

void blink_led(int pin, int times) {
  pinMode(pin, OUTPUT);
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(150);
    digitalWrite(pin, LOW);
    delay(150);
  }
  pinMode(pin, INPUT);  // Release pin
  delay(500);
}

void setup() {
  // Try pin 13 first
  blink_led(LED_PIN_1, 3);
  delay(1000);
  
  // Try PIN_LED
  blink_led(PIN_LED, 3);
  delay(1000);
  
  // Try pin 6
  blink_led(LED_PIN_3, 3);
}

void loop() {
  delay(1000);
}
