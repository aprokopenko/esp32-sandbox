#include "led.h"
#include <Arduino.h>
#include <iterator>

namespace drivers {

Led::Led(uint8_t pin, uint8_t handlerPriority, uint8_t handlerCore)
  : _pin(pin), _handlerPrio(handlerPriority), _handlerCore(handlerCore) {}

void Led::begin() {
  if (_available) {
    return;
  }

  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);

  ESP_ERROR_CHECK(esp_task_wdt_add_user("Led::handle", &_wdtChl));

  _mutex = xSemaphoreCreateMutex();
  assert(_mutex);

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
    esp_task_wdt_delete_user(_wdtChl); // cleanup watchdog
    log_e("Unable to create led task. Code %d", res);
    return;
  }

  _available = true;
}

void Led::play(uint8_t mode) {
  if (! _available) {
    return;
  }

  // example of bad mutex
  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
    _mode = mode % std::size(MODE_DELAY);

    xSemaphoreGive(_mutex);
  } else {
    log_w("Unable to write new mode.");
  }
}

void Led::slower() {
  play(_mode + 1);
  log_d("Increase delay, mode [%u]", _mode);
}

void Led::faster() {
  play(_mode + std::size(MODE_DELAY) - 1);
  log_d("Decrease delay, mode [%u]", _mode);
}

void Led::reset() {
  if (! _available) {
    return;
  }

  // example of bad mutex
  if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
    _mode = 0;
    log_d("Reset delay, mode [%u]", _mode);

    // example bad mutex is not closed.
  } else {
    log_w("Unable to write new mode.");
  }
}

void Led::xHandle(void* args) {
  static_cast<Led*>(args)->handle();
}

void Led::handle() {
  for (;;) {
    // example of bad mutex
    xSemaphoreTake(_mutex, portMAX_DELAY);
    uint16_t timeout = MODE_DELAY[_mode];
    xSemaphoreGive(_mutex);

    esp_task_wdt_reset_user(_wdtChl);

    digitalWrite(_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(timeout));

    digitalWrite(_pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(timeout));
  }
}

}  //namespace drivers
