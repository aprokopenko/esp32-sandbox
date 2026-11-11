#include "net.h"
#include "event.h"
#include "event_factory.h"
#include <WiFi.h>

namespace services {

Net::Net(QueueHandle_t events, const WifiSettings &settings)
  : _events(events), _settings(settings) {}

void Net::connect() {
  // TODO: add events when WiFi goes down to try reconnect.

  if (_settings.ssid[0] == '\0') {
    log_w("No WiFi settings found.");
    Event e = createEvent(EventType_t::WiFiFailed);
    xQueueSend(_events, &e, portMAX_DELAY);

    return;
  }

  log_i("Establishing WiFi connection...");

  WiFi.disconnect();
  WiFi.begin(_settings.ssid, _settings.pass);
  size_t waiting = 0;
  uint16_t delay = 300;

  while (WiFi.status() != WL_CONNECTED && waiting < MAX_CONNECTION_TIMEOUT_MS) {
    log_i(".");
    vTaskDelay(pdMS_TO_TICKS(delay));
    waiting += delay;
  }

  if (!WiFi.isConnected()) {
    log_w("Unable to connect to WiFi with existing settings.");
    Event e = createEvent(EventType_t::WiFiFailed);
    xQueueSend(_events, &e, portMAX_DELAY);

    return;
  }

  log_i("WiFi connection successful. IP address: %s", WiFi.localIP().toString());
  Event e = createEvent(EventType_t::WiFiUp);
  xQueueSend(_events, &e, portMAX_DELAY);
}

}  //namespace services
