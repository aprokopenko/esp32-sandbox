#pragma once
#include <stdint.h>

#ifndef LORA_FIXED_PAYLOAD_SIZE
#define LORA_FIXED_PAYLOAD_SIZE 0
#endif
static_assert(LORA_FIXED_PAYLOAD_SIZE >= 0 && LORA_FIXED_PAYLOAD_SIZE <= 255, "LoRa payload must be 0..255 bytes");

constexpr uint8_t LEDR_PIN = 11;
constexpr uint8_t LEDB_PIN = 12;
constexpr uint8_t LEDG_PIN = 13;
constexpr uint8_t SCK_PIN = 8;
constexpr uint8_t MISO_PIN = 9;
constexpr uint8_t MOSI_PIN = 10;
constexpr uint8_t CS0_PIN = 4;
constexpr uint8_t DIO1_PIN = 1;
constexpr uint8_t NRST_PIN = 2;
constexpr uint8_t BUSY_PIN = 3;
constexpr uint8_t RF_SWITCH_PIN = 5;
