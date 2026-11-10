#pragma once
#include "settings.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace services {

class Net {
public:
  explicit Net(QueueHandle_t events, const WifiSettings& settings);
  void connect();
private:
  static constexpr uint16_t MAX_CONNECTION_TIMEOUT_MS = 10000;

  QueueHandle_t _events;
  WifiSettings _settings;
};

}  //namespace services
