#pragma once
#include <Preferences.h>
#include "settings.h"

namespace services {

class SettingsStore {
public:
  SettingsStore();
  void load();
  bool setWifi(const WifiSettings& wifi);
  bool setNotifications(const NotificationSettings& notifications);

private:
  static constexpr auto WIFI_NS = "wifi";
  static constexpr auto NOTIF_NS = "notify";

  Preferences _nvs;
  WifiSettings _wifi = {};
  NotificationSettings _notifications = {};

public:
  const WifiSettings& wifi() const {
    return _wifi;
  }
  const NotificationSettings& notifications() const {
    return _notifications;
  }
};

}  //namespace services
