#pragma once
#include "blinker.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

namespace drivers {

class Led : public drivers::Blinker {
public:
  explicit Led(uint8_t pin, uint8_t handlerPriority = 1, uint8_t handlerCore = 1);
  void begin() override;
  void play(uint8_t mode) override;
  void faster() override;
  void slower() override;
  void reset() override;

private:
  static constexpr uint32_t TASK_STACK = 2048;
  static constexpr uint16_t MODE_DELAY[4] = { 250, 500, 1000, 2000 };

  uint8_t _pin;
  uint8_t _handlerPrio;
  uint8_t _handlerCore;
  TaskHandle_t _task;
  uint8_t _mode = 0;
  bool _available = false;
  esp_task_wdt_user_handle_t _wdtChl = nullptr;

  SemaphoreHandle_t _mutex = nullptr;

  static void xHandle(void* args);
  void handle();
};

}  // namespace drivers
