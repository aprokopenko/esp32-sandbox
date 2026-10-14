#pragma once
#include <stdint.h>

// Bit-banged SPI master, mode 0 (CPOL = 0, CPHA = 0), MSB first
class CustomSPI {
private:
  uint8_t _clkPin, _csPin, _mosiPin, _misoPin;

  uint8_t transfer(uint8_t out);

public:
  CustomSPI(uint8_t clk, uint8_t cs, uint8_t mosi, uint8_t miso);

  void begin();
  uint8_t readRegister(uint8_t addr);
};
