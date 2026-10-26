#pragma once
#include <esp_system.h>

struct EspResetReason {
    esp_reset_reason_t code;
    const char* name;
};

#define valname(x) {x, #x}
constexpr EspResetReason ESP_RESET_REASONS[] = {
  valname(ESP_RST_UNKNOWN),    //!< Reset reason can not be determined
  valname(ESP_RST_POWERON),    //!< Reset due to power-on event
  valname(ESP_RST_EXT),        //!< Reset by external pin (not applicable for ESP32)
  valname(ESP_RST_SW),         //!< Software reset via esp_restart
  valname(ESP_RST_PANIC),      //!< Software reset due to exception/panic
  valname(ESP_RST_INT_WDT),    //!< Reset (software or hardware) due to interrupt watchdog
  valname(ESP_RST_TASK_WDT),   //!< Reset due to task watchdog
  valname(ESP_RST_WDT),        //!< Reset due to other watchdogs
  valname(ESP_RST_DEEPSLEEP),  //!< Reset after exiting deep sleep mode
  valname(ESP_RST_BROWNOUT),   //!< Brownout reset (software or hardware)
  valname(ESP_RST_SDIO),
};
#undef valname

inline const char* getEspReasonName(esp_reset_reason_t reason) {
  for (const auto& rr : ESP_RESET_REASONS) {
    if (rr.code == reason) return rr.name;
  }

  return "Unknown";
}
