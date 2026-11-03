#include "display.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <atomic>
#include <cinttypes>
#include "config.h"

namespace display {

namespace {
  constexpr uint8_t SCREEN_WIDTH = 128;
  constexpr uint8_t SCREEN_HEIGHT = 64;
  constexpr int8_t OLED_RESET = -1;
  constexpr uint32_t I2C_CLOCK_HZ = 400000;  // full frame ≈ 23 ms

  constexpr uint8_t X_UI_QUEUE_LEN = 10;
  constexpr uint16_t X_DISPLAY_STACK_SIZE = 3072;
  // lowest app priority: drawing must never delay the button -> radio path.
  // Not 0: that would share time slices with the idle task.
  constexpr uint8_t X_DISPLAY_PRIO = 2;
  // core 1, next to the Arduino loop; the radio task runs on core 0
  constexpr BaseType_t X_DISPLAY_CORE = 1;

  struct UiMessage {
    UiEvent type;
    int32_t value;
  };

  // state owned by the display task only
  struct UiState {
    uint32_t singleClicks = 0;
    uint32_t doubleClicks = 0;
    uint32_t sent = 0;
    uint32_t failed = 0;
    int32_t lastError = 0;
  };

  QueueHandle_t xUiEvents = nullptr;

  // written by any producer task, read by the display task
  std::atomic<uint32_t> uiDrops{ 0 };

  Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  void scanWire() {
    log_i("Scanning I2C...");

    for (uint8_t address = 1; address < 127; address++) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission() == 0) {
        log_i("- found device on 0x%02x", address);
      }
    }
  }

  bool initOled() {
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(I2C_CLOCK_HZ);

    scanWire();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
      log_e("SSD1306 init failed, UI events will be ignored");
      return false;
    }

    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);

    return true;
  }

  void apply(UiState& state, const UiMessage& msg) {
    switch (msg.type) {
      case UiEvent::ClickSingle:
        state.singleClicks++;
        break;
      case UiEvent::ClickDouble:
        state.doubleClicks++;
        break;
      case UiEvent::TxDone:
        state.sent++;
        break;
      case UiEvent::TxFailed:
        state.failed++;
        state.lastError = msg.value;
        break;
    }
  }

  void draw(const UiState& state) {
    oled.clearDisplay();
    oled.setCursor(0, 0);

    oled.println("Button -> LoRa");
    oled.println();
    oled.printf("Single:  %" PRIu32 "\n", state.singleClicks);
    oled.printf("Double:  %" PRIu32 "\n", state.doubleClicks);
    oled.printf("Sent:    %" PRIu32 "\n", state.sent);
    oled.printf("Failed:  %" PRIu32, state.failed);
    if (state.failed > 0) {
      oled.printf(" (%" PRId32 ")", state.lastError);
    }
    oled.println();
    oled.printf("UI drops: %" PRIu32 "\n", static_cast<uint32_t>(uiDrops.load()));

    oled.display();
  }

  void processUiQueue(void* args) {
    // init inside the task, so setup() is not blocked by I2C and this task is the only Wire user
    bool oledReady = initOled();
    UiState state;

    if (oledReady) {
      draw(state);
    }

    UiMessage msg;

    static UBaseType_t minFree = UINT32_MAX;

    for (;;) {
      if (pdTRUE != xQueueReceive(xUiEvents, &msg, portMAX_DELAY)) {
        continue;
      }
      apply(state, msg);

      // drain everything that is already queued: one redraw for a burst of events
      while (pdTRUE == xQueueReceive(xUiEvents, &msg, 0)) {
        apply(state, msg);
      }

      if (oledReady) {
        draw(state);
      }

      UBaseType_t freeNow = uxTaskGetStackHighWaterMark(nullptr);
      if (freeNow < minFree) {
        minFree = freeNow;
        log_i("[DISPLAY STACK] min free %u B", freeNow);
      }
    }
  }
}  // namespace

void begin() {
  xUiEvents = xQueueCreate(X_UI_QUEUE_LEN, sizeof(UiMessage));
  if (xUiEvents == nullptr) {
    log_e("Unable to create UI queue");
    return;
  }

  BaseType_t status = xTaskCreatePinnedToCore(
    processUiQueue,
    "processUiQueue",
    X_DISPLAY_STACK_SIZE,
    nullptr,  // params
    X_DISPLAY_PRIO,
    nullptr,  // handle
    X_DISPLAY_CORE);
  if (status != pdPASS) {
    log_e("Unable to create display task. Code %d", status);
  }
}

void post(UiEvent type, int32_t value) {
  if (xUiEvents == nullptr) {
    return;
  }

  UiMessage msg = {
    .type = type,
    .value = value,
  };

  // timeout 0: the UI must never block a producer
  if (pdTRUE != xQueueSend(xUiEvents, &msg, 0)) {
    uiDrops++;
  }
}

}  // namespace display
