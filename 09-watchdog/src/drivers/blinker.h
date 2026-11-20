#pragma once
#include <stdint.h>

namespace drivers {

class Blinker {
public:
  virtual ~Blinker() = default;
  virtual void begin() = 0;
  virtual void play(uint8_t mode) = 0;
  virtual void faster() = 0;
  virtual void slower() = 0;
  virtual void reset() = 0;
};

}  //namespace drivers
