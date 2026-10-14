#include "CustomSPI.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include "../../include/helpers.h"

CustomSPI::CustomSPI(uint8_t clk, uint8_t cs, uint8_t mosi, uint8_t miso)
  : _clkPin(clk), _csPin(cs), _mosiPin(mosi), _misoPin(miso) {}

void CustomSPI::begin() {
  // mode 0: clock is LOW while idle
  pinMode(_clkPin, OUTPUT);
  digitalWrite(_clkPin, LOW);

  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);

  pinMode(_mosiPin, OUTPUT);
  digitalWrite(_mosiPin, LOW);

  pinMode(_misoPin, INPUT_PULLUP);
}

uint8_t CustomSPI::readRegister(uint8_t addr) {
  digitalWrite(_csPin, LOW);

  // always set MSB to 0 in case wrong param passed, cause MSB = 0 > read operation
  Log.traceln("  sending addr: %s", byteToHex(addr).data());
  transfer(addr & 0x7F);
  Log.traceln("  reading response");
  uint8_t in = transfer(0x00);

  digitalWrite(_csPin, HIGH);

  return in;
}

uint8_t CustomSPI::transfer(uint8_t out) {
  uint8_t in = 0x00;

  for (int i = 7; i >= 0; i--) {
    uint8_t outBit = (out >> i) & 1;

    // set MOSI before CLK change, cause once it changes - MOSI is read by slave
    digitalWrite(_mosiPin, outBit);

    // rising edge: slave reads MOSI, we read MISO
    digitalWrite(_clkPin, HIGH);
    uint8_t inBit = digitalRead(_misoPin);

    // falling edge: slave shifts out its next bit
    digitalWrite(_clkPin, LOW);

    in = (in << 1) | inBit;
    Log.traceln("  transfer: out %d / in %d", outBit, inBit);
  }

  return in;
}