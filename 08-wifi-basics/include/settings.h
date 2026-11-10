#pragma once

struct WifiSettings {
  char ssid[33];
  char pass[65];
};

struct NotificationSettings {
  char url[200];
};
