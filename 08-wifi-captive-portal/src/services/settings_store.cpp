#include "settings_store.h"
#include <string.h>

// local dev defaults, not in git (see include/secrets.example.h)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif
#ifndef DEFAULT_WIFI_PASS
#define DEFAULT_WIFI_PASS ""
#endif
#ifndef DEFAULT_NOTIFY_URL
#define DEFAULT_NOTIFY_URL ""
#endif

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

  // nothing saved yet: fall back to the dev defaults (empty without secrets.h)
  if (_wifi.ssid[0] == '\0' && DEFAULT_WIFI_SSID[0] != '\0') {
    log_i("No saved WiFi settings, using defaults from secrets.h");
    strlcpy(_wifi.ssid, DEFAULT_WIFI_SSID, sizeof(_wifi.ssid));
    strlcpy(_wifi.pass, DEFAULT_WIFI_PASS, sizeof(_wifi.pass));
  }
  if (_notifications.url[0] == '\0' && DEFAULT_NOTIFY_URL[0] != '\0') {
    log_i("No saved notification URL, using default from secrets.h");
    strlcpy(_notifications.url, DEFAULT_NOTIFY_URL, sizeof(_notifications.url));
  }
}

bool SettingsStore::setWifi(const WifiSettings& wifi) {
  if (strcmp(wifi.ssid, "") == 0) {
      // empty string is not allowed
      return false;
    }

  if (!_nvs.begin(WIFI_NS, RW_MODE)) {
    return false;
  }

  // nothing changed = success; false only when a write fails
  bool ok = true;

  if (strcmp(_wifi.ssid, wifi.ssid) != 0) {
    ok = _nvs.putString("ssid", wifi.ssid) > 0;
  }

  if (ok && strcmp(_wifi.pass, wifi.pass) != 0) {
    if (strcmp(wifi.pass, "") == 0) {
      // open network: drop the old password
      ok = !_nvs.isKey("pass") || _nvs.remove("pass");
    } else {
      ok = _nvs.putString("pass", wifi.pass) > 0;
    }
  }

  _nvs.end();

  if (ok) {
    _wifi = wifi;
  }

  return ok;
}

bool SettingsStore::setNotifications(const NotificationSettings& notifications) {
  if (strncmp(notifications.url, "https://", 8) != 0) {
    // invalid url not allowed
    return false;
  }

  if (!_nvs.begin(NOTIF_NS, RW_MODE)) {
    return false;
  }

  // nothing changed = success; false only when a write fails
  bool ok = true;

  if (strcmp(_notifications.url, notifications.url) != 0) {
    ok = _nvs.putString("url", notifications.url) > 0;
  }

  _nvs.end();

  if (ok) {
    _notifications = notifications;
  }

  return ok;
}

}  //namespace services
