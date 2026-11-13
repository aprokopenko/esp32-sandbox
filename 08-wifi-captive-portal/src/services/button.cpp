#include "button.h"
#include "event_factory.h"
#include <Arduino.h>

namespace services {

IsrButton::IsrButton(QueueHandle_t events, OneButton &btn)
  : _events(events), _btn(btn) {}

void IsrButton::begin() {
  BaseType_t status = xTaskCreatePinnedToCore(
    tickHandlerCallback,
    "btnTickHandler",
    TASK_STACK,
    this,
    TASK_PRIO,
    &_tickTask,
    TASK_CORE
  );
  if (status != pdPASS) {
    log_e("Unable to create button processing task");
    return;
  }

  _btn.attachClick(
    [](void *p) {
      static_cast<IsrButton *>(p)->onButtonClicked(ButtonEvent_t::Single);
    },
    this
  );
  _btn.attachDoubleClick(
    [](void *p) {
      static_cast<IsrButton *>(p)->onButtonClicked(ButtonEvent_t::Double);
    },
    this
  );
  _btn.attachLongPressStart(
    [](void *p) {
      static_cast<IsrButton *>(p)->onButtonClicked(ButtonEvent_t::Long);
    },
    this
  );

  _setupCompleted = true;
  log_i("Button task and events configured.");
}

void IsrButton::listen() {
  if (!_setupCompleted) {
    log_e("Trying to listen button without proper initialization. Cancelling!");
    return;
  }
  if (_listening) {
    return;
  }

  // attach ISR (not good cause we operate pin here, but our driver (OneButton) not working with ISR)
  // we won't wrote driver wrapper for this line only in this example
  attachInterruptArg(_btn.pin(), isrHandler, this, CHANGE);
  _listening = true;

  log_i("Start button interrupts listening.");
}

void IsrButton::tickHandlerCallback(void *arg) {
  static_cast<IsrButton*>(arg)->tickHandler();
}

void IsrButton::tickHandler() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    uint32_t wokenMs = millis();
    uint32_t nowMs = wokenMs;

    do {
      _btn.tick();

      vTaskDelay(pdMS_TO_TICKS(5));
      nowMs = millis();
    } while (!_btn.isIdle() || (nowMs - wokenMs) < 100);
  }
}

IRAM_ATTR void IsrButton::isrHandler(void *arg) {
  auto *self = static_cast<IsrButton *>(arg);
  BaseType_t woken = pdFALSE;
  vTaskNotifyGiveFromISR(self->_tickTask, &woken);
  if (woken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

void IsrButton::onButtonClicked(ButtonEvent_t click) {
  log_v("Clicked with type [%d]", click);

  Event e = createEvent(Event{.type = EventType_t::ButtonPressed, .val = (int32_t)click});
  if (pdTRUE != xQueueSend(_events, &e, pdMS_TO_TICKS(1000))) {
    log_w("Click event lost, type [%d]", click);
  }
}

}  //namespace services
