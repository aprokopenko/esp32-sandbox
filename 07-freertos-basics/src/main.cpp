#include <Arduino.h>
#include "config.h"
#include "button.h"
#include "display.h"
#include "radio.h"

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(DBG1_PIN, OUTPUT);
  digitalWrite(DBG1_PIN, LOW);
  pinMode(DBG2_PIN, OUTPUT);
  digitalWrite(DBG2_PIN, LOW);

  // first: the UI queue must exist before other modules post to it
  display::begin();
  buttonBegin();
  radio::begin();

  log_i("Setup completed.");

  // indicate loading finished
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  ButtonEvent btnEv;

  if (pdTRUE == xQueueReceive(xBtnEvents, &btnEv, portMAX_DELAY)) {
    log_d("\nmain caught btn event, ms: %li", btnEv.ms);

    bool isSingle = btnEv.mode == ButtonPressMode::Single;
    display::post(isSingle ? display::UiEvent::ClickSingle : display::UiEvent::ClickDouble);

    if (radio::isAvailable) {
      radio::sendClicksNotification(isSingle ? 1 : 2);
    }

    log_d("\n");
  }
}
