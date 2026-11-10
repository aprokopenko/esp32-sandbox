#include <Arduino.h>
#include "config.h"
#include "helpers.h"
#include "app/app.h"
#include "drivers/led.h"
#include "services/settings_store.h"
#include "services/net.h"
#include "services/setup_portal.h"

static QueueHandle_t xEvents;

void setup() {
  Serial.begin(115200);

  pinMode(DBG1_PIN, OUTPUT);
  digitalWrite(DBG1_PIN, LOW);
  pinMode(DBG2_PIN, OUTPUT);
  digitalWrite(DBG2_PIN, LOW);

  log_i("App initialization...");
  xEvents = app::App::createQueue();
  if (xEvents == nullptr) {
    HALT("Unable to create App queue. Terminating!");
  }

  // init services and drivers
  static drivers::Led led(LED_PIN);
  static services::SettingsStore settings;
  settings.load();

  static services::Net net(xEvents, settings.wifi());
  static services::SetupPortal setup(xEvents, settings);

  // create app and load
  static app::App app(xEvents, led, net, setup);
  app.bootstrap();
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}
