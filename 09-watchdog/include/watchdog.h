#pragma once
#include <esp_task_wdt.h>
#include <esp32-hal.h>
#include "config.h"

namespace watchdog {

inline void begin() {
  static esp_task_wdt_config_t cfg = {
    .timeout_ms = WATCHDOG_TIMEOUT,
    .idle_core_mask = 1 << 0,
    .trigger_panic = true,
  };

  if(ESP_ERR_INVALID_STATE == esp_task_wdt_status(NULL)) {
    // invalid state on status means it's not running
    ESP_ERROR_CHECK(esp_task_wdt_init(&cfg));
  } else {
    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&cfg));
  }

  disableLoopWDT();
};

};  //namespace watchdog
