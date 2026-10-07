#include <Arduino.h>
#include <ArduinoLog.h>
#include <OneButton.h>
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

OneButton btn2(BTN2_PIN, true, true);  // pin, activeLow, enable internal pull-up

void setup()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);

  pinMode(BTN_PIN, INPUT_PULLUP);
  events.reserve(64);

  btn2.attachClick([]() { Log.infoln("btn2 click"); });
  btn2.attachDoubleClick([]() { Log.infoln("btn2 double click"); });
  btn2.attachLongPressStart([]() { Log.infoln("btn2 long press"); });

  Log.noticeln("Setup done");
}

void loop() {
  btn2.tick();

  uint8_t btnNow = digitalRead(BTN_PIN);
  bool btnPressed = btnNow == LOW;

  // catch click events
  if (btnNow != btnLast) {
    events.push_back({micros(), btnNow, btnNow == LOW});
    btnLast = btnNow;
  }

  if (millis() % 1000 == 0 && events.size() > 0) {
    for (const auto& e: events) {
      Log.infoln("Level %d (pressed %T) at %u us", e.lvl, e.pressed, e.us);
    }

    Log.infoln("");
    events.clear();
  }
}
