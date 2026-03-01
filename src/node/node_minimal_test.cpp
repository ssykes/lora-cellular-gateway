/**
 * @file node_minimal_test.cpp
 * @brief Bare minimum test - LED only, NO Serial
 */

#include <Arduino.h>

void setup() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);  // Turn on and stay on
}

void loop() {
  digitalWrite(13, HIGH);
  delay(200);
  digitalWrite(13, LOW);
  delay(200);
}
