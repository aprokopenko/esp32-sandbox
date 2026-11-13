#include "notifier.h"
#include <Arduino.h>
#include <inttypes.h>
#include "event_factory.h"

namespace services {

Notifier::Notifier(QueueHandle_t events, drivers::Transport& transport, services::WorldClock& worldClock)
  : _events(events), _transport(transport), _clock(worldClock) {}

void Notifier::begin() {
  _queue = xQueueCreate(QUEUE_LENGTH, sizeof(Event));
  if (_queue == nullptr) {
    log_e("Unable to create notifier queue.");
    return;
  }

  BaseType_t res = xTaskCreatePinnedToCore(
    xHandle,
    "notifierHandler",
    TASK_STACK,
    this,  // params
    TASK_PRIO,
    nullptr,  // task handler
    TASK_CORE
  );
  if (res != pdPASS) {
    log_e("Unable to create notifier task. Code %d", res);
    return;
  }

  _available = true;
}

bool Notifier::send(const Event& e) {
  if (!_available) {
    return false;
  }

  // called from the App task: never wait here
  if (xQueueSend(_queue, &e, 0) != pdTRUE) {
    _dropped++;
    log_w("Notifier queue is full, event dropped (%" PRIu32 " so far).", _dropped);
    return false;
  }

  return true;
}

void Notifier::xHandle(void* args) {
  static_cast<Notifier*>(args)->handle();
}

void Notifier::handle() {
  Event e;
  Notification n;

  for (;;) {
    xQueueReceive(_queue, &e, portMAX_DELAY);

    format(e, n);
    log_i("Sending notification: %s", n.text);

    int code = _transport.send(n);
    log_i("  transport result: %d, stack left: %u bytes", code, uxTaskGetStackHighWaterMark(nullptr));

    Event done = createEvent(EventType_t::NotifyDone);
    done.val = code;
    xQueueSend(_events, &done, 0);
  }
}

void Notifier::format(const Event& e, Notification& n) {
  time_t epoch = _clock.fromMs(e.t_ms);
  String epochStr = _clock.format(&epoch);

  switch (e.type) {
    case EventType_t::ButtonPressed: {
        const char* typeName = "Unrecognized";
        if (e.val == (int)ButtonEvent_t::Single) typeName = "Single";
        if (e.val == (int)ButtonEvent_t::Double) typeName = "Double";
        if (e.val == (int)ButtonEvent_t::Long) typeName = "Long";

        snprintf(n.text, sizeof(n.text), "%s press detected at %s.", typeName, epochStr.c_str());
        break;
    }

    default:
      snprintf(n.text, sizeof(n.text), "Event %d at %" PRIu32 " ms.", static_cast<int>(e.type), e.t_ms);
      break;
  }
}

}  // namespace services
