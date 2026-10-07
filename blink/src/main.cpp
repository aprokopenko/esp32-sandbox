#include <Arduino.h>
#include <esp_timer.h>

// Onboard LED on most ESP32 DevKit boards (GPIO 2)
constexpr uint8_t LED_PIN = 2;
constexpr uint64_t BLINK_PERIOD_US = 1000 * 2000;  // toggle every 2 s

static esp_timer_handle_t blinkTimer;

// Runs in the esp_timer task context, not an ISR, so GPIO calls are safe here
static void onBlinkTimer(void *) {
  static bool ledOn = false;
  ledOn = !ledOn;
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  const esp_timer_create_args_t args = {
      .callback = &onBlinkTimer,
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "blink",
  };
  ESP_ERROR_CHECK(esp_timer_create(&args, &blinkTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(blinkTimer, BLINK_PERIOD_US));
}

void loop() {
  // Nothing to do: blinking is driven entirely by the timer
  vTaskDelay(portMAX_DELAY);
}
