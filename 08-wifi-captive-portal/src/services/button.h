#pragma once
#include <OneButton.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "event.h"

namespace services {

class IsrButton {
public:
  explicit IsrButton(QueueHandle_t events, OneButton& btn);
  void begin();
  void listen();

private:
  static constexpr uint8_t TASK_PRIO = 5;
  static constexpr uint32_t TASK_STACK = 4096;
  static constexpr uint8_t TASK_CORE = 1;

  QueueHandle_t _events;
  OneButton& _btn;
  TaskHandle_t _tickTask;
  bool _setupCompleted = false;
  bool _listening = false;

  static void tickHandlerCallback(void* arg);
  void tickHandler();

  IRAM_ATTR static void isrHandler(void* arg);

  void onButtonClicked(ButtonEvent_t click);
};

}  //namespace services
