#include "led.h"
#include <Arduino.h>
#include <iterator>

namespace drivers {

Led::Led(uint8_t pin, uint8_t handlerPriority, uint8_t handlerCore)
  : _pin(pin), _handlerPrio(handlerPriority), _handlerCore(handlerCore) {}

void Led::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);

  // create task
  BaseType_t res = xTaskCreatePinnedToCore(
    xHandle,
    "ledHandler",
    TASK_STACK,
    this,  // params
    _handlerPrio,
    &_task,  // handle
    _handlerCore
  );
  if (res != pdPASS) {
    log_e("Unable to create led task. Code %d", res);
    return;
  }

  _available = true;
}

void Led::play(uint8_t mode) {
  if (! _available) {
    return;
  }

  _mode = mode % std::size(MODE_DELAY);
}

void Led::slower() {
  _mode = (_mode + 1) % std::size(MODE_DELAY);
  log_d("Faster, mode [%u]", _mode);
}

void Led::faster() {
  _mode = (_mode + std::size(MODE_DELAY) - 1) % std::size(MODE_DELAY);
  log_d("Slower, mode [%u]", _mode);
}

void Led::xHandle(void* args) {
  static_cast<Led*>(args)->handle();
}

void Led::handle() {
  for (;;) {
    uint16_t timeout = MODE_DELAY[_mode];

    digitalWrite(_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(timeout));

    digitalWrite(_pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(timeout));
  }
}

}  //namespace drivers
