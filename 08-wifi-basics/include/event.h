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
};

struct Event {
  EventType_t type;
  uint32_t t_ms;
  int32_t val;
};
