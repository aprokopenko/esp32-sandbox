#pragma once
#include "event.h"
#include "drivers/blinker.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace app {

using drivers::Blinker;

class App {
public:
  explicit App(Blinker& blink);
  void listen(QueueHandle_t events);
private:
  static constexpr uint8_t HANDLER_PRIO = 2;
  static constexpr uint32_t HANDLER_STACK = 4096;
  static constexpr uint8_t HANDLER_CORE = 1;

  QueueHandle_t _events;
  Blinker& _blink;

  static void handleCallback(void* arg);
  void handle();
  void processEvent(const Event& e);
};

}  //namespace app
