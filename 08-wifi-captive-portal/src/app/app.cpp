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
  services::SetupPortal& setup,
  services::Notifier& notifier,
  services::WorldClock& worldClock,
  services::IsrButton& btn
) :
  _events(events),
  _led(led),
  _net(net),
  _setup(setup),
  _notifier(notifier),
  _clock(worldClock),
  _btn(btn)
{}

void App::bootstrap() {
  _led.begin();
  _led.play(drivers::Led::Pattern::Blink);
  _notifier.begin();
  _btn.begin();

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
        _state = State::Provisioning;
        _led.play(drivers::Led::Pattern::DoubleBlink);
        _setup.begin();
        break;

      case EventType_t::SettingsSaved:
        log_i("Settings saved event received.");
        _led.play(drivers::Led::Pattern::Blink);
        vTaskDelay(pdMS_TO_TICKS(5000)); // wait few seconds, so wifi is not closed on phone too fast
        esp_restart(); //restart board to try reconnect
        break;

      case EventType_t::WiFiUp: {
        log_i("WiFi is up event received.");
        _clock.sync();
        Event tsEvent = createEvent(EventType_t::TimeSynced);
        xQueueSend(_events, &tsEvent, 0);
        break;
      }

      case EventType_t::TimeSynced:
        log_i("Time synced: %s.", _clock.format().c_str());
        // init button
        _btn.listen();
        _state = State::Online;
        // we're ready to work
        _led.play(drivers::Led::Pattern::On);
        break;

      case EventType_t::ButtonPressed: {
        log_d("Button clicked");
        // send notification
        _notifier.send(e);
        break;
      }

      case EventType_t::NotifyDone:
        if (e.val >= 200 && e.val < 300) {
          log_i("Notification delivered, HTTP %d.", e.val);
        } else {
          log_w("Notification failed, code %d.", e.val);
        }
        break;

      default:
        log_e("Unknown event type received. type: [%u], ms: [%lu], val: [%d]", e.type, e.t_ms, e.val);
        break;
    }
  }
}

}  //namespace app
