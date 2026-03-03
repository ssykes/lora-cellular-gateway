// HelTec WiFi LoRa 32 V4 (ESP32-S3) pin definitions
// Based on official HelTec V4 pinout

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// GPIO pin definitions
#define GPIO_NUM_0 0
#define GPIO_NUM_1 1
#define GPIO_NUM_2 2
#define GPIO_NUM_3 3
#define GPIO_NUM_4 4
#define GPIO_NUM_5 5
#define GPIO_NUM_6 6
#define GPIO_NUM_7 7
#define GPIO_NUM_8 8
#define GPIO_NUM_9 9
#define GPIO_NUM_10 10
#define GPIO_NUM_11 11
#define GPIO_NUM_12 12
#define GPIO_NUM_13 13
#define GPIO_NUM_14 14
#define GPIO_NUM_15 15
#define GPIO_NUM_16 16
#define GPIO_NUM_17 17
#define GPIO_NUM_18 18
#define GPIO_NUM_19 19
#define GPIO_NUM_20 20
#define GPIO_NUM_21 21
#define GPIO_NUM_35 35
#define GPIO_NUM_36 36
#define GPIO_NUM_37 37
#define GPIO_NUM_41 41
#define GPIO_NUM_42 42
#define GPIO_NUM_43 43
#define GPIO_NUM_44 44
#define GPIO_NUM_45 45
#define GPIO_NUM_46 46
#define GPIO_NUM_47 47
#define GPIO_NUM_48 48

// Built-in LED
#define LED_BUILTIN 35

// I2C pins
#define SDA 41
#define SCL 42

// SPI pins
#define SS    8
#define MOSI  10
#define MISO  11
#define SCK   9

// UART pins
#define TX 43
#define RX 44

// ADC pins
#define A0 1

// LoRa SX1262 pins (HelTec V4 specific)
#define LORA_NSS   8
#define LORA_MOSI  10
#define LORA_MISO  11
#define LORA_SCK   9
#define LORA_RST   12
#define LORA_BUSY  13
#define LORA_DIO1  14

#endif // Pins_Arduino_h
