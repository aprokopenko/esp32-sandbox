#pragma once
#include <stdint.h>

// FSK radio settings, must match on transmitter and receiver
constexpr float FSK_FREQUENCY_MHZ = 868.3;
constexpr float FSK_BITRATE_KBPS = 15.2;               // 0.6..300, test: 15.2 / 100.0
constexpr float FSK_FREQ_DEV_KHZ = FSK_BITRATE_KBPS;   // 0.6..200, f_dev = bitrate
constexpr float FSK_RX_BANDWIDTH_KHZ = 312.0;          // RX only, >= 2 * f_dev + bitrate
constexpr int8_t FSK_POWER_DBM = 14;                   // -9..22
constexpr uint16_t FSK_PREAMBLE_LENGTH = 16;           // bits
