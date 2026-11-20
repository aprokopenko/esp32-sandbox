#include "controls.h"
#include <esp32-hal-log.h>
#include <esp32-hal-timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "event.h"

namespace services {

using drivers::RmtKeyboard;
using drivers::RmtKeyboardKey;

Controls::Controls(RmtKeyboard &kb)
  : _kb(kb) {}

void Controls::begin(QueueHandle_t events) {
  _events = events;

  _kb.onRx([this](RmtKeyboardKey k) {
    onPressed(k);
  });
  _kb.begin();
}

void Controls::onPressed(RmtKeyboardKey k) {
  log_d("Button pressed: %d", k);

  int32_t value;
  switch (k) {
    case RmtKeyboardKey::Up:
      value = 0;
      break;
    case RmtKeyboardKey::Down:
      value = 1;
      break;
    case RmtKeyboardKey::Sharp:
      value = 2;
      break;
    default:
      value = -1;
      break;
  }

  if (value < 0) {
    log_d("Unsupported key pressed, code [%d]", k);
    return;
  }

  Event e = Event{
    .type = EventType_t::ControlPressed,
    .t_ms = millis(),
    .val = value,
  };
  if (pdTRUE != xQueueSend(_events, &e, pdMS_TO_TICKS(MAX_TIMEOUT))) {
    log_w("Queue is full, key press skipped. Code [%d].", k);
  }
}

}  //namespace services

