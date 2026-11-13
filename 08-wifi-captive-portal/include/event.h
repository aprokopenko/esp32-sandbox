#pragma once
#include <stdint.h>

enum class EventType_t {
  Initialized,
  ButtonPressed,
  WiFiUp,
  WiFiDown,
  WiFiFailed,
  TimeSynced,
  SettingsSaved,
  NotifyDone,  // val: HTTP status, or a negative error
};

struct Event {
  EventType_t type;
  uint32_t t_ms;
  int32_t val;
};

enum class ButtonEvent_t {
  Single,
  Double,
  Long
};
