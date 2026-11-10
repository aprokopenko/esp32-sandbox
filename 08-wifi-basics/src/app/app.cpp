#include <Arduino.h>
#include "app.h"
#include "event.h"
#include "event_factory.h"
#include "helpers.h"

namespace app {

QueueHandle_t App::createQueue() {
  return xQueueCreate(APP_QUEUE_LENGTH, sizeof(struct Event));
}

App::App(
  QueueHandle_t events,
  drivers::Led& led,
  services::Net& net,
  services::SetupPortal& setup
) :
  _events(events),
  _led(led),
  _net(net),
  _setup(setup)
{}

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
        // try to connect
        _net.connect();
        break;

      case EventType_t::WiFiFailed:
        log_w("WiFi is down, need to load access point");
        _led.play(drivers::Led::Pattern::DoubleBlink);
        _setup.begin();
        break;

      case EventType_t::TimeSynced:
        // we're ready to work
        _led.play(drivers::Led::Pattern::On);
        break;


      default:
        log_e("Unknown event type received. type: [%u], ms: [%lu], val: [%d]", e.type, e.t_ms, e.val);
        break;
    }
  }
}

}  //namespace app
