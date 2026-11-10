#include "settings_store.h"
#include <string.h>

namespace services {

namespace {
  constexpr bool RO_MODE = true;
  constexpr bool RW_MODE = false;
}  //namespace

SettingsStore::SettingsStore() {}

void SettingsStore::load() {
  // load wifi settings
  if (_nvs.begin(WIFI_NS, RO_MODE)) {
    if (_nvs.isKey("ssid")) {
      _nvs.getString("ssid", _wifi.ssid, sizeof(_wifi.ssid));
    }
    if (_nvs.isKey("pass")) {
      _nvs.getString("pass", _wifi.pass, sizeof(_wifi.pass));
    }
    _nvs.end();
  }

  // load notifications settings
  if (_nvs.begin(NOTIF_NS, RO_MODE)) {
    if (_nvs.isKey("url")) {
      _nvs.getString("url", _notifications.url, sizeof(_notifications.url));
    }
    _nvs.end();
  }
}

bool SettingsStore::setWifi(const WifiSettings& wifi) {
  if (strcmp(wifi.ssid, "") == 0) {
      // empty string is not allowed
      return false;
    }

  _nvs.begin(WIFI_NS, RW_MODE);
  size_t bytes = 0;

  if (strcmp(_wifi.ssid, wifi.ssid) != 0 ) {
    bytes = _nvs.putString("ssid", wifi.ssid);
  }

  if (bytes && strcmp(_wifi.pass, wifi.pass) != 0) {
    if (strcmp(wifi.pass, "") == 0 && _nvs.isKey("pass")) {
      bytes = (size_t)_nvs.remove("pass");
    }
    if (strcmp(wifi.pass, "") != 0) {
      bytes = _nvs.putString("pass", wifi.pass);
    }
  }

  _nvs.end();

  if (bytes > 0) {
    _wifi = wifi;
    return true;
  }

  return false;
}

bool SettingsStore::setNotifications(const NotificationSettings& notifications) {
  if (strncmp(notifications.url, "https://", 8) != 0) {
    // invalid url not allowed
    return false;
  }

  _nvs.begin(NOTIF_NS, RW_MODE);
  size_t bytes = 0;

  if (strcmp(_notifications.url, notifications.url) != 0) {
    bytes = _nvs.putString("url", notifications.url);
  }

  _nvs.end();

  if (bytes > 0) {
    _notifications = notifications;
    return true;
  }

  return false;
}

}  //namespace services
