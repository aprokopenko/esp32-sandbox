#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "button.h"
#include "radio.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void scanWire() {
  Log.warningln("Scanning I2C...");

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Log.infoln("- found device on %x", address);
    }
  }
}

void loadDisplay() {
  // Wait for display
  delay(500);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Log.errorln("SSD1306 allocation failed");
    for (;;);  // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  oled.display();

  delay(2000);  // Pause for 2 seconds
  oled.clearDisplay();

  Log.infoln("Printing \"Hello\" on OLED.");
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(30, 30);
  oled.print("Hello world!");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);
  Wire.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(DBG1_PIN, OUTPUT);
  digitalWrite(DBG1_PIN, LOW);
  pinMode(DBG2_PIN, OUTPUT);
  digitalWrite(DBG2_PIN, LOW);

  Log.infoln("Setup completed.");

  scanWire();
  loadDisplay();

  buttonBegin();
  radio::begin();

  // indicate loading finished
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  ButtonEvent btnEv;

  if (pdTRUE == xQueueReceive(xBtnEvents, &btnEv, portMAX_DELAY)) {
    log_d("\nmain caught btn event, ms: %li", btnEv.ms);

    if (radio::isAvailable) {
      radio::sendClicksNotification(btnEv.mode == ButtonPressMode::Single ? 1 : 2);
    }

    log_d("\n");
  }
}
