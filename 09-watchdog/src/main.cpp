#include <Arduino.h>
#include "config.h"
#include "event.h"
#include "drivers/ir_keyboard.h"
#include "drivers/led.h"
#include "services/controls.h"
#include "app/app.h"

QueueHandle_t events;
drivers::IRKeyboard kb(IR_PIN);
drivers::Led led(LED_PIN);
services::Controls ctrl(kb);
app::App apk(led);

void setup() {
  Serial.begin(115200);

  events = xQueueCreate(20, sizeof(struct Event));
  if (! events) {
    log_e("Unable to create main events queue.");
    abort();
  }

  apk.listen(events);
  led.begin();
  ctrl.begin(events);

  log_i("Setup completed.");
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
