#pragma once

// Local defaults for development.
// Copy to secrets.h (not in git) and fill in:  cp include/secrets.example.h include/secrets.h
// Used only while NVS has no saved settings; the setup portal overrides them.

#define DEFAULT_WIFI_SSID  ""
#define DEFAULT_WIFI_PASS  ""
#define DEFAULT_NOTIFY_URL ""  // e.g. "https://ntfy.sh/<random-topic>"
