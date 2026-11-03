#pragma once
#include <stdint.h>

namespace display {

enum class UiEvent : uint8_t {
  ClickSingle,
  ClickDouble,
  TxDone,
  TxFailed,  // value: RadioLib error code
};

// Creates the UI queue and the display task. The task owns Wire and the OLED:
// nothing else may touch the I2C bus after this call.
void begin();

// Non-blocking, safe to call from any task (not from an ISR).
// If the queue is full the event is dropped and counted.
void post(UiEvent type, int32_t value = 0);

}  // namespace display
