#include <Arduino.h>
#include "app.h"
#include "event.h"
#include "event_factory.h"
#include "helpers.h"

namespace app {

QueueHandle_t App::createQueue() {
  return xQueueCreate(APP_QUEUE_LENGTH, sizeof(struct Event));
}

App::App(QueueHandle_t events, drivers::Led& led)
  : _events(events), _led(led) {}

void App::bootstrap() {
  _led.begin();
  _led.play(drivers::Led::Pattern::Blink);

  // queue processing task
  BaseType_t res = xTaskCreatePinnedToCore(
    xHandle,
    "appQueueHandler",
    HANDLER_STACK,
    this, // params
    HANDLER_PRIO,
    nullptr, // task handler
    HANDLER_CORE
  );
  if (res != pdPASS) {
    _led.play(drivers::Led::Pattern::Off);
    HALT("Unable to create App queue handler task");
  }

  log_i("Bootstrap completed.");
  Event e = createEvent(EventType_t::Initialized);
  xQueueSend(_events, &e, portMAX_DELAY);
}

void App::xHandle(void* args) {
  static_cast<App*>(args)->handle();
}

void App::handle() {
  Event e;

  for (;;) {
    xQueueReceive(_events, &e, portMAX_DELAY);
    log_d("event received: type[%d], ms[%lu], val[%d]", e.type, e.t_ms, e.val);

    switch (e.type)
    {
      case EventType_t::Initialized:
        // TODO: remove, this is for test only
        vTaskDelay(pdMS_TO_TICKS(10000));
        _led.play(drivers::Led::Pattern::On);
        break;

      default:
        log_e("Unknown event type received. type: [%u], ms: [%lu], val: [%d]", e.type, e.t_ms, e.val);
        break;
    }
  }
}

}  //namespace app
