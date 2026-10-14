#pragma once
#include <stdint.h>
#include <stdio.h>
#include <array>

inline std::array<char, 5> byteToHex(const uint8_t b) {
  std::array<char, 5> buf;
  snprintf(buf.data(), buf.size(), "0x%02x", b);

  return buf;
}