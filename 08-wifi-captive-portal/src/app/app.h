#pragma once
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "drivers/led.h"
#include "services/net.h"
#include "services/setup_portal.h"
#include "services/notifier.h"
#include "services/world_clock.h"
#include "services/button.h"

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
    drivers::Led& led,
    services::Net& net,
    services::SetupPortal& setup,
    services::Notifier& notifier,
    services::WorldClock& worldClock,
    services::IsrButton& btn
  );
  void bootstrap();

private:
  static constexpr uint8_t HANDLER_PRIO = 4;
  static constexpr uint32_t HANDLER_STACK = 4096;
  static constexpr uint8_t HANDLER_CORE = 1;

  State _state = State::Loading;
  QueueHandle_t _events;

  drivers::Led& _led;
  services::Net& _net;
  services::SetupPortal& _setup;
  services::Notifier& _notifier;
  services::WorldClock& _clock;
  services::IsrButton& _btn;

  static void xHandle(void* args);
  void handle();
};

}  //namespace app
