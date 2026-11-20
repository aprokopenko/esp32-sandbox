#include <Arduino.h>
#include "config.h"
#include "event.h"
#include "drivers/ir_keyboard.h"
#include "drivers/led.h"
#include "services/controls.h"
#include "app/app.h"
#include "watchdog.h"
#include "reset_info.h"

QueueHandle_t events;
drivers::IRKeyboard kb(IR_PIN);
drivers::Led led(LED_PIN);
services::Controls ctrl(kb);
app::App apk(led);

void setup() {
  Serial.begin(115200);
  log_i("\n================= SETUP BEGIN ===============\n");

  log_i("reset reason: %d (%s)", esp_reset_reason(), getEspReasonName(esp_reset_reason()));
  log_i("free heap: %lu", (unsigned long)esp_get_free_heap_size());

  events = xQueueCreate(20, sizeof(struct Event));
  if (! events) {
    log_e("Unable to create main events queue.");
    abort();
  }

  watchdog::begin();

  apk.listen(events);
  led.begin();
  ctrl.begin(events);

  log_i("Completed.\n================= SETUP END ===============\n");
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
