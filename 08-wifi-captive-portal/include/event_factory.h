#pragma once
#include <esp32-hal.h>
#include "event.h"

inline Event createEvent(EventType_t type) {
  return Event {
    .type = type,
    .t_ms = millis(),
    .val = 0
  };
}

inline Event createEvent(Event e) {
  if (! e.t_ms) {
    e.t_ms = millis();
  }

  return e;
}
