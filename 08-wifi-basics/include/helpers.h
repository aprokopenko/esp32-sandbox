#pragma once
#include <esp32-hal-log.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define PANIC(msg, ...) \
  do { \
    log_e(msg, ##__VA_ARGS__); \
    abort(); \
  } while (0)

#define HALT(msg, ...) \
  do { \
    log_e(msg, ##__VA_ARGS__); \
    vTaskSuspend(nullptr); \
  } while (0)
