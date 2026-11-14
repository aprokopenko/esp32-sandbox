#include "app.h"
#include <esp32-hal-log.h>

namespace app {

App::App(Blinker &blink)
  : _blink(blink) {}

void App::listen(QueueHandle_t events) {
  _events = events;

  BaseType_t status = xTaskCreatePinnedToCore(
    handleCallback,
    "appQueueHandle",
    HANDLER_STACK,
    this,
    HANDLER_PRIO,
    nullptr,
    HANDLER_CORE
  );
  if (status != pdPASS) {
    log_e("Unable to create App queue handler");
    abort();
  }
}

void App::handleCallback(void *arg) {
  static_cast<App *>(arg)->handle();
}

void App::handle() {
  Event e;

  for (;;) {
    xQueueReceive(_events, &e, portMAX_DELAY);

    processEvent(e);
  }
}

void App::processEvent(const Event &e) {
  switch (e.type) {
    case EventType_t::ControlPressed: {
        if (e.val == 0) {
          _blink.slower();
        } else {
          _blink.faster();
        }
        break;
      }

    default:
      log_w("Unknown event type received. Type [%d]", e.type);
      break;
  }
}

}  //namespace app
