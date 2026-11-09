#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

extern QueueHandle_t xBtnEvents;

enum class ButtonPressMode {
  Single,
  Double
};

struct ButtonEvent {
  int32_t ms;
  ButtonPressMode mode;
};

void buttonBegin();
