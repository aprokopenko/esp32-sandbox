#include "setup_portal.h"
#include "WiFi.h"
#include "helpers.h"
#include <string.h>
#include <algorithm>
#include <freertos/task.h>

namespace services {

SetupPortal::SetupPortal(QueueHandle_t events, services::SettingsStore& settings)
  : _events(events), _settings(settings) {}

void SetupPortal::begin() {
  log_i("Initializing Setup Portal...");

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

  if (status != pdTRUE) {
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
      .open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN
    };
    strlcpy(ni.ssid, WiFi.SSID(i).c_str(), sizeof(ni.ssid));

    log_d("  found network [%s], secure [%d]", ni.ssid, !ni.open);

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

  _server.on("/setup.htm", HTTP_GET, [this]() { handleGetSetupPage(); });
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
      _server.uri()
    );
  });
}

void SetupPortal::handleGetSetupPage() {
  _server.send(200, "text/plain", "hello world");
}

void SetupPortal::handleGetNetworksJson() {
  _server.send(200, "text/plain", "[]");
}

void SetupPortal::handlePostSave() {
  _server.send(200, "text/plain", "saved world");
}



}  //namespace services
