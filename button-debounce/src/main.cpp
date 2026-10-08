#include <Arduino.h>
#include <ArduinoLog.h>
#include <OneButton.h>
#include <SimpleButton.h>
#include <vector>
#include "pins.h"

// pins declaration and globals
uint8_t btnLast = HIGH;

struct BtnEvent {
  uint32_t us;
  uint8_t lvl;
  bool pressed;
};

std::vector<BtnEvent> events;

OneButton oneBtn(ONE_BTN_PIN, true, true);  // pin, activeLow, enable internal pull-up
SimpleButton customBtn(CUSTOM_BTN_PIN);

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);

  pinMode(BTN_PIN, INPUT_PULLUP);
  events.reserve(64);

  oneBtn.attachClick([]() { Log.infoln("oneBtn click"); });
  oneBtn.attachDoubleClick([]() { Log.infoln("oneBtn double click"); });
  oneBtn.attachLongPressStart([]() { Log.infoln("oneBtn long press"); });

  customBtn.init();

  Log.noticeln("Setup done");
}

void loop() {
  oneBtn.tick();

  uint8_t btnNow = digitalRead(BTN_PIN);
  bool btnPressed = btnNow == LOW;

  // catch click events
  if (btnNow != btnLast) {
    events.push_back({micros(), btnNow, btnNow == LOW});
    btnLast = btnNow;
  }

  if (customBtn.pressed()) {
    events.push_back({micros(), false, true});
  }

  if (millis() % 1000 == 0 && events.size() > 0) {
    for (const auto& e: events) {
      Log.infoln("Level %d (pressed %T) at %u us", e.lvl, e.pressed, e.us);
    }

    Log.infoln("");
    events.clear();
  }
}
