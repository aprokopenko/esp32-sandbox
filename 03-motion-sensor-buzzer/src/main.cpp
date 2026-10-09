#include <Arduino.h>
#include <ArduinoLog.h>
#include "pins.h"
#include <MotionSensor.h>
#include <PassiveBuzzer.h>
#include <NotePicker.h>

constexpr uint16_t READY_BEEP_FREQ = 2000;
constexpr uint32_t READY_BEEP_MS = 300;
constexpr uint32_t NOTE_MS = 500;

MotionSensor ms(SENSOR_PIN);
PassiveBuzzer buzz(BUZZER_PIN);
NotePicker notes;

void setup() {
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);

  ms.init();
  buzz.init();

  Log.noticeln("Setup completed. Waiting for sensor warm-up...");
}

void loop() {
  buzz.tick();

  if (ms.isJustReady()) {
    Log.noticeln("Sensor ready.");
    buzz.playNote(READY_BEEP_FREQ, READY_BEEP_MS);
  }

  if (ms.isJustMoved()) {
    const Note& note = notes.pickRandom();
    Log.infoln("Motion detected at %ds, playing %s (%d Hz)", millis() / 1000, note.name, note.freq);
    buzz.playNote(note.freq, NOTE_MS);
  }
}
