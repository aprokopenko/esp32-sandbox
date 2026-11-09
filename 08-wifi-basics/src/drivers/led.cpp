#include "led.h"
#include <Arduino.h>

namespace drivers {

namespace {
  struct Step {
    bool on;
    uint16_t ms;
  };

  struct PatternDef {
    const Step* steps;
    uint8_t count;
  };

  const Step STEPS_OFF[] = { { false, 0 } };
  const Step STEPS_ON[] = { { true, 0 } };
  const Step STEPS_BLINK[] = { { true, 250 }, { false, 250 } };
  const Step STEPS_DOUBLE[] = { { true, 100 }, { false, 100 }, { true, 100 }, { false, 500 } };

  PatternDef getPatternDef(Led::Pattern p) {
    switch (p)
    {
      case Led::Pattern::On:
        return { STEPS_ON, std::size(STEPS_ON) };
      case Led::Pattern::Blink:
        return { STEPS_BLINK, std::size(STEPS_BLINK) };
      case Led::Pattern::DoubleBlink:
        return { STEPS_DOUBLE, std::size(STEPS_DOUBLE) };
      default:
        return { STEPS_OFF, std::size(STEPS_OFF) };
    }
  }

}  //namespace

Led::Led(uint8_t pin)
  : _pin(pin) {}

void Led::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);

  // create task
  BaseType_t res = xTaskCreatePinnedToCore(
    xHandle,
    "ledHandler",
    TASK_STACK,
    this,  // params
    TASK_PRIO,
    &_task,  // handle
    TASK_CORE
  );
  if (res != pdPASS) {
    log_e("Unable to create led task. Code %d", res);
    return;
  }

  _available = true;
}

void Led::play(const Pattern p) {
  if (! _available) {
    return;
  }

  xTaskNotify(_task, (uint32_t) p, eSetValueWithOverwrite);
}

void Led::xHandle(void* args) {
  static_cast<Led*>(args)->handle();
}

void Led::handle() {
  Pattern p = Pattern::Off;
  size_t i = 0;

  for (;;) {
    PatternDef def = getPatternDef(p);

    const Step& s = def.steps[i];

    digitalWrite(_pin, s.on);

    uint32_t newP;
    TickType_t waitTicks = s.ms ? pdMS_TO_TICKS(s.ms) : portMAX_DELAY;

    if (pdTRUE == xTaskNotifyWait(0, 0, &newP, waitTicks)) {
      p = static_cast<Pattern>(newP);
      i = 0;
    } else {
      i = (i+1) % def.count;
    }
  }
}

}  //namespace drivers
