#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace radio {

extern bool isAvailable;

void begin();

BaseType_t sendClicksNotification(unsigned int clicks);

}  //namespace radio