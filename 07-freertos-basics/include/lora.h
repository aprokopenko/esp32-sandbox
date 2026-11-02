#pragma once
#include <stdint.h>

// LoRa radio settings, must match on transmitter and receiver
constexpr float LORA_FREQUENCY_MHZ = 868.3;
constexpr float LORA_BANDWIDTH_KHZ = 250.0;   // 7.8 .. 500
constexpr uint8_t LORA_SPREADING_FACTOR = 7;  // 6..12
constexpr uint8_t LORA_CODING_RATE = 5;       // 5..8 = 4/5..4/8
constexpr uint8_t LORA_SYNC_WORD = 0x22;
constexpr int8_t LORA_POWER_DBM = 5;         // 2..17
constexpr uint16_t LORA_PREAMBLE_LENGTH = 8;  // symbols
