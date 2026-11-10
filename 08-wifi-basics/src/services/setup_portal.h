#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <vector>
#include "services/settings_store.h"
#include <DNSServer.h>
#include <WebServer.h>
#include <IPAddress.h>

namespace services {

class SetupPortal {
public:
  SetupPortal(QueueHandle_t events, services::SettingsStore& settings);
  void begin();
private:
  static constexpr auto DEFAULT_AP_SSID = "ESP32Connect";
  static inline const IPAddress SERVER_IP{192, 168, 10, 10};
  static constexpr uint8_t DNS_PORT = 53;
  static constexpr uint8_t TASK_PRIO = 2;
  static constexpr uint32_t TASK_STACK = 8192;
  static constexpr uint8_t TASK_CORE = 1;

  struct NetInfo {
    char ssid[33];
    int32_t rssi;
    bool open;
  };

  QueueHandle_t _events;
  services::SettingsStore& _settings;

  std::vector<NetInfo> _networks;
  WebServer _server{80};
  DNSServer _dns;

  void scanNetworks();
  void startAP();
  void initRouting();
  static void handleRequestCallback(void* args);
  void handleRequest();

};

}  //namespace services
