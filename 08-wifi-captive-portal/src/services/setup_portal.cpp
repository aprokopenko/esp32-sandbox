#include "setup_portal.h"
#include <WiFi.h>
#include "helpers.h"
#include <string.h>
#include <algorithm>
#include <freertos/task.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "settings.h"
#include "event.h"
#include "event_factory.h"

namespace services {

SetupPortal::SetupPortal(QueueHandle_t events, services::SettingsStore& settings)
  : _events(events), _settings(settings) {}

void SetupPortal::begin() {
  log_i("Initializing Setup Portal...");

  if (!LittleFS.begin(false)) {
    PANIC("Unable to mount local file system. Terminating!");
  }

  scanNetworks();
  startAP();
  initRouting();

  if (! _dns.start(DNS_PORT, "*", SERVER_IP)) {
    PANIC("Unable to create Access Point DNSServer. Terminating!");
  }
  _server.enableDelay(false);
  _server.begin();

  BaseType_t status = xTaskCreatePinnedToCore(
    listenRequestCallback,
    "setupPortalServerHandler",
    TASK_STACK,
    this,
    TASK_PRIO,
    nullptr,  // task handler
    TASK_CORE
  );

  if (status != pdPASS) {
    PANIC("Unable to create Access Point server handler. Terminating!");
  }

  log_i("  setup portal is ready for connection.");
}

void SetupPortal::startAP() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(DEFAULT_AP_SSID)) {
    PANIC("Unable to create WiFi Accesss Point for setup portal. Terminating!");
  }
  if (!WiFi.softAPConfig(SERVER_IP, SERVER_IP, IPAddress(255, 255, 255, 0))) {
    PANIC("Unable to configure WiFi Accesss Point for setup portal. Terminating!");
  }
}

void SetupPortal::scanNetworks() {
  log_i("Scanning WiFi networks...");
  WiFi.disconnect();

  _networks.clear();
  int cnt = WiFi.scanNetworks();

  if (cnt <= 0) {
    log_w("No WiFi networks available");
    return;
  }

  log_d("  found %d networks, scanning...", cnt);

  _networks.reserve(cnt);
  for (int i = 0; i < cnt; i++) {
    if (WiFi.SSID(i).c_str()[0] == '\0') {
      // skip hidden networks
      continue;
    }

    NetInfo ni = NetInfo{
      .rssi = WiFi.RSSI(i),
      .secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN
    };
    strlcpy(ni.ssid, WiFi.SSID(i).c_str(), sizeof(ni.ssid));

    log_d("  found network [%s], secure [%d]", ni.ssid, ni.secure);
    log_d("  [%s] bssid %s ch %d rssi %d secure %d",
      ni.ssid, WiFi.BSSIDstr(i).c_str(), WiFi.channel(i), ni.rssi, ni.secure);

    // one network can have several access points (mesh, office):
    // keep one entry per SSID, with the strongest signal
    auto same = std::find_if(
      _networks.begin(),
      _networks.end(),
      [&ni](const NetInfo& n) { return strcmp(n.ssid, ni.ssid) == 0; }
    );

    if (same == _networks.end()) {
      _networks.push_back(ni);  // new SSID
    } else if (ni.rssi > same->rssi) {
      *same = ni;  // same SSID, stronger access point
    }
  }

  // sort networks by rssi
  std::sort(
    _networks.begin(),
    _networks.end(),
    [](const NetInfo& a, const NetInfo& b) { return a.rssi > b.rssi; }
  );

  log_i("  saved %zu network(s).", _networks.size());
  WiFi.scanDelete();
}

void SetupPortal::listenRequestCallback(void* args) {
  static_cast<SetupPortal*>(args)->listenRequest();
}

void SetupPortal::listenRequest() {
  for (;;) {
    _dns.processNextRequest();
    _server.handleClient();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void SetupPortal::initRouting() {

  _server.serveStatic("/setup.htm", LittleFS, "/setup_portal/setup.html");
  _server.on("/networks.json", HTTP_GET, [this]() { handleGetNetworksJson(); });
  _server.on("/setup.htm", HTTP_POST, [this]() { handlePostSave(); });

  // service routes
  _server.on("/", HTTP_GET, [this]() {
    _server.sendHeader("Location", "setup.htm");
    _server.send(302, "text/plain", "Redirect");
  });

  _server.on("/success.txt", [this]() { _server.send(200, "text/plain", ""); }); // firefox some requested utl
  _server.on("/favicon.ico", [this]() { _server.send(404, "text/plain", "Not Found."); }); // webpage icon

  _server.onNotFound([this]() {
    _server.sendHeader("Location", "http://" + SERVER_IP.toString());
    _server.send(302, "text/plain", "Redirect");

    log_d(
      "404 %s %s",
      (_server.method() == HTTP_GET) ? "GET" : "POST",
      _server.uri().c_str()
    );
  });
}

void SetupPortal::handleGetNetworksJson() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  for(const NetInfo& n : _networks) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = n.ssid;
    o["rssi"] = n.rssi;
    o["secure"] = n.secure;
  }

  String out;
  serializeJson(doc, out);
  log_d("Json: %s", out.c_str());

  _server.send(200, "application/json", out);
}

void SetupPortal::handlePostSave() {
  String ssid = _server.arg("ssid");
  String pass = _server.arg("password");
  String url = _server.arg("url");
  url.trim();

  log_d("Form submitted: ssid [%s], pass [%u chars], url [%s]", ssid.c_str(), pass.length(), url.c_str());

  // check everything before the first write:
  // a wrong field must not leave half of the settings saved
  const char* error = nullptr;
  if (ssid.isEmpty()) {
    error = "Choose a network or enter its name.";
  } else if (ssid.length() >= sizeof(WifiSettings::ssid)) {
    error = "The network name is longer than 32 characters.";
  } else if (!pass.isEmpty() && (pass.length() < MIN_PASS_LEN || pass.length() > MAX_PASS_LEN)) {
    error = "A Wi-Fi password has 8 to 63 characters.";
  } else if (!url.startsWith("https://") || url.length() <= strlen("https://")) {
    error = "The URL must start with https://";
  } else if (url.length() >= sizeof(NotificationSettings::url)) {
    error = "The URL is too long.";
  }

  if (error) {
    log_w("Settings rejected: %s", error);
    _server.send(422, "text/plain", error);  // the page shows this text under the title
    return;
  }

  WifiSettings wifi{};
  strlcpy(wifi.ssid, ssid.c_str(), sizeof(wifi.ssid));
  strlcpy(wifi.pass, pass.c_str(), sizeof(wifi.pass));

  NotificationSettings notif{};
  strlcpy(notif.url, url.c_str(), sizeof(notif.url));

  // the values are valid here, so false means NVS failed to write
  if (!_settings.setWifi(wifi) || !_settings.setNotifications(notif)) {
    log_e("Unable to write settings to NVS");
    _server.send(500, "text/plain", "The board could not save the settings. Try again.");
    return;
  }

  log_i("Settings saved.");
  _server.send(200, "text/plain", "Saved.");

  // send event
  Event e = createEvent(EventType_t::SettingsSaved);
  xQueueSend(_events, &e, portMAX_DELAY);
}

}  //namespace services
