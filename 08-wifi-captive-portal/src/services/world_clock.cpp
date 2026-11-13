#include "world_clock.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>

namespace services {

WorldClock::WorldClock() {}

bool WorldClock::sync() {
  if (!_synced) {
    configTzTime(TZ, NTP_SRV1, NTP_SRV2, NTP_SRV3);
    _synced = true;
  }

  while (now() <= 1700000000) { // start of timestamp
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  return _synced;
}

time_t WorldClock::now() {
  return time(nullptr);
}

time_t WorldClock::fromMs(uint32_t ms) {
  uint32_t ageMs = millis() - ms;
  return now() - (ageMs / 1000);
}

String WorldClock::format(time_t* ts, const char *frm) {
  time_t nowTs = now();

  if (ts == nullptr) {
    ts = &nowTs;
  }
  struct tm t;
  localtime_r(ts, &t);

  char buf[32];
  size_t len = strftime(buf, sizeof(buf), frm, &t);

  return String(len ? buf : "");
}

}  //namespace services
