#include <Arduino.h>
#include <ArduinoLog.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include "config.h"
#include "lora.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SX1276 radio = new Module(CS0_PIN, DIO0_PIN, NRST_PIN, RADIOLIB_NC);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP32)
  IRAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  receivedFlag = true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  Log.begin(LOG_LEVEL, &Serial);

  // init SPI interface
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS0_PIN);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  delay(500); // wait display
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Log.errorln("[SSD1306] Display allocation failed");
    for (;;);  // Don't proceed, loop forever
  }

  Log.infoln("[SX1276] Initializing...");
  ConfigLoRa_t config;
  config.frequency       = LORA_FREQUENCY_MHZ;
  config.bandwidth       = LORA_BANDWIDTH_KHZ;
  config.spreadingFactor = LORA_SPREADING_FACTOR;
  config.codingRate      = LORA_CODING_RATE;
  config.syncWord        = LORA_SYNC_WORD;
  config.power           = LORA_POWER_DBM;
  config.preambleLength  = LORA_PREAMBLE_LENGTH;

  int state = radio.begin(config);
  if (state == RADIOLIB_ERR_NONE) {
    Log.infoln("   success!");
  } else {
    Log.errorln("   failed, code %d", state);
    for (;;);  // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  oled.display();

  delay(1000);  // Pause
  oled.clearDisplay();

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketReceivedAction(setFlag);

  // Start listening
  Log.infoln("[SX1276] Starting to listen ... ");
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Log.infoln(F("[SX1276]    success!"));
  } else {
    Log.errorln("[SX1276]    failed! code %d", state);
    while (true) { delay(10); }
  }

  Log.traceln("Printing loading msg on OLED.");
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 30);
  oled.print("Listening LoRa...");
  oled.display();
}

void loop() {
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;

    String msg;
    int state = radio.readData(msg);

    oled.clearDisplay();
    oled.setCursor(0, 30);

    if (state == RADIOLIB_ERR_NONE) {
      float rssi = radio.getRSSI();
      float snr = radio.getSNR();

      Log.traceln("[SX1276] Packet received! RSSI: %FdBm / SNR: %F dB", rssi, snr);
      Log.traceln("[SX1276]    Message: %s", msg.c_str());

      oled.print(msg.c_str());
      oled.display();
    } else {
      Log.errorln("[SX1276] Packet receiving failed! Code %d", state);

      oled.print("Packet receiving failed!");
      oled.display();
    }
  }
}
