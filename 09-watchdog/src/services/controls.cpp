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

  Event e = Event{
    .type = EventType_t::ControlPressed,
    .t_ms = millis(),
    .val = (k == RmtKeyboardKey::Down) ? 1 : 0,
  };
  if (pdTRUE != xQueueSend(_events, &e, pdMS_TO_TICKS(MAX_TIMEOUT))) {
    log_w("Queue is full, key press skipped. Code [%d].", k);
  }
}

}  //namespace services

