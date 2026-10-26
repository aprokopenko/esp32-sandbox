#include <Arduino.h>
#include <ArduinoLog.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <nvs_flash.h>
#include <esp32-hal-log.h>
#include "config.h"
#include "reset.h"
#include "version.h"
#include "partitions.h"
#include <esp_partition.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  Log.begin(LOG_LEVEL, &Serial);

  log_i("fw %s (%s) build %s", FW_VERSION, GIT_HASH, BUILD_TIME);
  log_i("reset reason: %d (%s)", esp_reset_reason(), getEspReasonName(esp_reset_reason()));
  log_i("free heap: %lu", (unsigned long)esp_get_free_heap_size());

  ESP_ERROR_CHECK(nvs_flash_init());

  // print partitions info
  traceEspPartitions();

  // init SPI interface
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS0_PIN);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  delay(500); // wait display
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Log.errorln("[SSD1306] Display allocation failed");
    for (;;);  // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  oled.display();

  delay(1000);  // Pause
  oled.clearDisplay();

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 10);
  oled.print("To do a crash send \nvia Serial:\n   'd', 'n' or 'a'");
  oled.display();

}

volatile float value;

void loop() {
  if (!Serial.available()) return;

  char cmd = Serial.read();
  switch (cmd) {
    case 'd': {                       // IntegerDivideByZero
      volatile uint32_t divisor = 0;
      value = 10000 / divisor;
      break;
    }
    case 'n': {                       // LoadProhibited (null pointer)
      volatile int* p = nullptr;
      log_i("%d", *p);
      break;
    }
    case 'a':                         // abort()
      abort();
      break;
  }
}
