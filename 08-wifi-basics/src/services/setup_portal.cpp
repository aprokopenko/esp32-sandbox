#include "setup_portal.h"
#include "WiFi.h"
#include "helpers.h"
#include <string.h>
#include <freertos/task.h>

namespace services {

// namespace {

//   struct NetInfo {
//     char ssid[33];
//     int32_t rssi;
//     bool open;
//   };

//   std::vector<NetInfo> networks;

// } //namespace

SetupPortal::SetupPortal(QueueHandle_t events, services::SettingsStore& settings)
  : _events(events), _settings(settings) {}

void SetupPortal::begin() {
  log_i("Initializing Setup Portal...");
  BaseType_t status = xTaskCreatePinnedToCore(
    handleRequestCallback,
    "setupPortalServerHandler",
    TASK_STACK,
    this,
    TASK_PRIO,
    nullptr,  // task handler
    TASK_CORE);

  if (status != pdTRUE) {
    PANIC("Unable to create Access Point server handler. Terminating!");
  }

  scanNetworks();
  startAP();
  initRouting();

  _dns.start(DNS_PORT, "*", SERVER_IP);
  _server.enableDelay(false);
  _server.begin();

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

void SetupPortal::initRouting() {
  _server.on("/", HTTP_GET, [this]() {
    _server.sendHeader("Location", "setup.htm");
    _server.send(301, "text/plain", "Redirect");
  });

  _server.on("/setup.htm", HTTP_GET, [this]() {
    _server.send(200, "text/plain", "hello world");
  });
  _server.on("/networks.json", HTTP_GET, [this]() {
    _server.send(200, "text/plain", "[]");
  });

  _server.on("/success.txt", [this]() { _server.send(200, "text/plain", ""); }); // firefox some requested utl
  _server.on("/favicon.ico", [this]() { _server.send(404, "text/plain", "Not Found."); }); // webpage icon

  _server.onNotFound([this]() {
    _server.sendHeader("Location", "http://" + SERVER_IP.toString());
    _server.send(301, "text/plain", "Redirect");

    log_d(
      "404 %s %s",
      (_server.method() == HTTP_GET) ? "GET" : "POST",
      _server.uri()
    );
  });
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

    log_d("  found network [%s], secure [%zu]", ni.ssid, !ni.open);

    _networks.push_back(ni);
  }

  // sort networks by rssi
  std::sort(
    _networks.begin(),
    _networks.end(),
    [](const NetInfo& a, const NetInfo& b) { return a.rssi > b.rssi; }
  );

  std::unique(
    _networks.begin(),
    _networks.end(),
    [](const NetInfo& a, const NetInfo& b) { return a.ssid != b.ssid; }
  );

  log_i("  saved %u network(s).", _networks.size());
}

void SetupPortal::handleRequestCallback(void* args) {
  static_cast<SetupPortal*>(args)->handleRequest();
}

void SetupPortal::handleRequest() {
  for (;;) {
    _dns.processNextRequest();
    _server.handleClient();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

}  //namespace services
