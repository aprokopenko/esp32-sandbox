#pragma once
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "drivers/led.h"

namespace app {

constexpr uint8_t APP_QUEUE_LENGTH = 50;

class App {
public:
  enum class State { Loading,
                     Provisioning,
                     Online };

  static QueueHandle_t createQueue();

  App(
    QueueHandle_t events,
    drivers::Led& led
  );
  void bootstrap();

private:
  static constexpr uint8_t HANDLER_PRIO = 4;
  static constexpr uint32_t HANDLER_STACK = 4096;
  static constexpr uint8_t HANDLER_CORE = 1;

  State _state = State::Loading;
  QueueHandle_t _events;
  drivers::Led& _led;

  static void xHandle(void* args);
  void handle();
};

}  //namespace app
