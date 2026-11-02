#include "button.h"
#include <Arduino.h>
#include "config.h"

QueueHandle_t xBtnEvents = nullptr;

/**
 * PRIVATE
 */
namespace {

constexpr uint16_t X_BTN_ISR_STACK = 8192;  // TODO: recalculate later
constexpr uint8_t X_BTN_ISR_PRIO = 3;
constexpr unsigned int BTN_DEBOUNCE_MS = 30;
constexpr unsigned int BTN_DOUBLE_MS = 350;
constexpr unsigned int BTN_EVENT_REPORT_MAX_WAIT_MS = 5000;  // 5s

constexpr uint32_t RTC_ATTR_MAGIC = 7243769;
RTC_NOINIT_ATTR uint32_t rtcAttrMagic;
RTC_NOINIT_ATTR uint32_t btnIsrRejections;
RTC_NOINIT_ATTR uint32_t btnEventRejections;

struct ButtonRawEdge {
  int64_t us;
  int value;
};

QueueHandle_t xBtnIsr;

constexpr gpio_num_t GPIO_DBG1_PIN = static_cast<gpio_num_t>(DBG1_PIN);

IRAM_ATTR void buttonChanged() {
  gpio_set_level(GPIO_DBG1_PIN, HIGH);
  ButtonRawEdge bt = {
    .us = esp_timer_get_time(),
    .value = digitalRead(BTN_PIN),
  };

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (pdTRUE != xQueueSendFromISR(xBtnIsr, &bt, &xHigherPriorityTaskWoken)) {
    btnIsrRejections++;
  }

  if (pdTRUE == xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

enum class RawEdgeTransition {
  Undefined,
  Press,
  Release,
  Glitch,
};

struct RawEdgeState {
  int64_t us;
  RawEdgeTransition transition;
};

enum class ButtonState {
  Idle,
  Processing,  // Pressed once and we're waiting second press or timeout
  SinglePressed,
  DoublePressed,
};

void defineButtonEdgeTransition(const ButtonRawEdge &e, RawEdgeTransition &trans) {
  static int64_t lastEdgeUs = 0;
  static int lastEdgeLevel = HIGH;

  if (e.us == lastEdgeUs) {
    // this means we return event to queue cause it was busy and don't need to change transition
    return;
  }

  trans = RawEdgeTransition::Undefined;

  if ((e.us - lastEdgeUs) / 1000 <= BTN_DEBOUNCE_MS) {
    trans = RawEdgeTransition::Glitch;
    lastEdgeLevel = e.value;
  }

  if (lastEdgeLevel == HIGH && e.value == LOW) {
    trans = RawEdgeTransition::Press;
  }

  if (lastEdgeLevel == LOW && e.value == HIGH) {
    trans = RawEdgeTransition::Release;
  }

  lastEdgeUs = e.us;
  lastEdgeLevel = e.value;
}

TickType_t safePdUS_TO_TICKS(int64_t us) {
  if (us <= 0) return 1;

  return pdMS_TO_TICKS(us / 1000) + 1;
}

void processButtonChange(void *args) {
  ButtonRawEdge e;
  uint32_t waitTicks = portMAX_DELAY;
  ButtonState state = ButtonState::Idle;
  RawEdgeTransition lastTransition = RawEdgeTransition::Undefined;

  int64_t firstPressUs = 0;
  int64_t deadlineUs = 0;
  int64_t lastPressUs = 0;

  for (;;) {
    BaseType_t eventReceived = xQueueReceive(xBtnIsr, &e, waitTicks);

    if (eventReceived == pdTRUE) {
      log_d("btn event %lld, val %d", e.us, (bool)e.value);

      defineButtonEdgeTransition(e, lastTransition);

      // ignore high signals, we wait for falling edge
      if (lastTransition == RawEdgeTransition::Press) {
        if (state == ButtonState::Idle) {
          // click starts
          state = ButtonState::Processing;
          firstPressUs = e.us;
          lastPressUs = e.us;
          deadlineUs = firstPressUs + (BTN_DOUBLE_MS * 1000);
        } else {
          // check deadline
          if (e.us <= deadlineUs) {
            state = ButtonState::DoublePressed;
            lastPressUs = e.us;
          } else {
            // we had busy CPU and this is new single click, return this to queue
            xQueueSendToFront(xBtnIsr, &e, pdMS_TO_TICKS(1000));
            state = ButtonState::SinglePressed;
          }
        }
      }
    }

    int64_t nowUs = esp_timer_get_time();
    int64_t activeUs = (eventReceived == pdTRUE) ? e.us : nowUs;

    if (state == ButtonState::Processing && activeUs > deadlineUs) {
      state = ButtonState::SinglePressed;
    }

    // calculate wait time
    if (state == ButtonState::Processing) {
      waitTicks = safePdUS_TO_TICKS(deadlineUs - nowUs);
    }

    // notify
    if (state != ButtonState::Idle && state != ButtonState::Processing) {
      ButtonEvent btnEv = {
        .ms = (int32_t) (firstPressUs / 1000ULL),
        .mode = (state == ButtonState::SinglePressed) ? ButtonPressMode::Single : ButtonPressMode::Double,
      };
      log_d("button event detected. mode: %d, ms: %li", btnEv.mode, btnEv.ms);

      // clean up
      state = ButtonState::Idle;
      firstPressUs = deadlineUs = lastPressUs = 0;
      waitTicks = portMAX_DELAY;

      // set to queue
      if (pdFALSE == xQueueSend(xBtnEvents, &btnEv, pdMS_TO_TICKS(BTN_EVENT_REPORT_MAX_WAIT_MS))) {
        btnEventRejections++;
        log_e("Button event xQueueSend failed.");
      }
    }
  }
}

}  // namespace


/**
 * PUBLIC
 */

void buttonBegin() {
  pinMode(BTN_PIN, INPUT_PULLUP);

  // init RTC attrs
  if (rtcAttrMagic != RTC_ATTR_MAGIC) {
    rtcAttrMagic = RTC_ATTR_MAGIC;
    btnEventRejections = 0;
    btnIsrRejections = 0;
  }

  // create queue and task
  xBtnIsr = xQueueCreate(40, sizeof(struct ButtonRawEdge));
  if (xBtnIsr == nullptr) {
    log_e("Unable to create button interruptions queue");
    return;
  }

  xBtnEvents = xQueueCreate(3, sizeof(struct ButtonEvent));
  if (xBtnEvents == nullptr) {
    log_e("Unable to create button processed events queue");
    return;
  }

  BaseType_t status = xTaskCreatePinnedToCore(
    processButtonChange,
    "processButtonChange",
    X_BTN_ISR_STACK,
    nullptr,  // params
    X_BTN_ISR_PRIO,
    nullptr,        // pointer
    tskNO_AFFINITY  // TODO: define core later
  );
  if (status != pdPASS) {
    log_e("Unable to create button processing task. Code %d", status);
    return;
  }

  // register interrupt (has to be after queue/task init)
  attachInterrupt(BTN_PIN, buttonChanged, CHANGE);
}
