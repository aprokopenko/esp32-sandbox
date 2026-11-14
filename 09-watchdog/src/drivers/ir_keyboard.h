#pragma once
#include <stdint.h>
#include <driver/rmt.h>
#include <functional>
#include "rmt_keyboard.h"

namespace drivers {

class IRKeyboard : public RmtKeyboard {
public:
  explicit IRKeyboard(uint8_t pin, uint8_t handlerPriority = 3, uint8_t handlerCore = 0);
  void begin() override;
  void onRx(kbCallback_t cb) override;

private:
  static constexpr auto RMT_CHL = RMT_CHANNEL_0;
  static constexpr uint16_t RMT_BUF_SIZE = 1000;
  static constexpr uint16_t RMT_EDGE_LVL = 1100;
  static constexpr uint32_t HANDLER_STACK = 4096;

  uint8_t _pin;
  uint8_t _handlerPriority;
  uint8_t _handlerCore;
  RingbufHandle_t _rb;
  kbCallback_t _onRx = nullptr;

  static void irHandlerCallback(void *arg);
  void beginRmtRx();
  void irHandler();
  RmtKeyboardKey parseNecCmd(uint8_t necCmd);
};

} //namespace drivers
