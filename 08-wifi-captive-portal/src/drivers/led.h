#pragma once
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace drivers {

class Led {
public:
  enum class Pattern { Off, On, Blink, DoubleBlink };
  explicit Led(uint8_t pin);
  void begin();
  void play(const Pattern p);

private:
  static constexpr uint8_t TASK_PRIO = 1;
  static constexpr uint32_t TASK_STACK = 2048;
  static constexpr uint8_t TASK_CORE = 1;

  uint8_t _pin;
  TaskHandle_t _task;
  bool _available = false;


  static void xHandle(void* args);
  void handle();
};

}  // namespace drivers
