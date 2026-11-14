#pragma once
#include "drivers/rmt_keyboard.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace services {

using drivers::RmtKeyboard;
using drivers::RmtKeyboardKey;

class Controls {
public:
  Controls(RmtKeyboard& kb);
  void begin(QueueHandle_t events);

private:
  static constexpr uint16_t MAX_TIMEOUT = 1000;

  QueueHandle_t _events = nullptr;
  drivers::RmtKeyboard& _kb;

  void onPressed(RmtKeyboardKey);
};

}  //namespace service
