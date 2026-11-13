#pragma once
#include <time.h>
#include <WString.h>
#include <stdint.h>

namespace services {

class WorldClock {
public:
  WorldClock();
  bool sync();
  time_t now();
  time_t fromMs(uint32_t ms);
  String format(time_t* ts = nullptr, const char* frm = "%Y-%m-%d %H:%M:%S");
private:
  static constexpr const char* TZ = "EET-2EEST,M3.5.0/3,M10.5.0/4";
  static constexpr const char* NTP_SRV1 = "pool.ntp.org";
  static constexpr const char* NTP_SRV2 = "time.ntp.org.ua";
  static constexpr const char* NTP_SRV3 = "time1.google.com";

  bool _synced = false;
};

}  //namespace services
