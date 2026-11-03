#include "radio.h"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RadioLib.h>
#include <optional>
#include "config.h"
#include "display.h"
#include "lora.h"

namespace radio {

bool isAvailable = false;

namespace {
  QueueHandle_t xLoRa;
  SemaphoreHandle_t xTxSemaphore;

  constexpr uint16_t X_LORA_STACK_SIZE = 3072;
  constexpr uint8_t X_LORA_PRIO = 5;

  SX1276 chip = new Module(CS0_PIN, DIO0_PIN, NRST_PIN, RADIOLIB_NC);

  // save transmission state between loops
  int transmissionState = RADIOLIB_ERR_NONE;

  const char* prepareClicksMessage(unsigned int clicks) {
    if (clicks == 1) {
      return "SINGLE CLICK";
    }
    if (clicks == 2) {
      return "DOUBLE CLICK";
    }

    return "MANY CLICKS";
  }

  IRAM_ATTR void notifyPacketSent(void) {
    auto pxHigherPrioTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(xTxSemaphore, &pxHigherPrioTaskWoken);

    if (pxHigherPrioTaskWoken == pdTRUE) {
      portYIELD_FROM_ISR();
    }
  }

  std::optional<RadioLibTime_t> startTransmission(const char* msg) {
    size_t len = strlen((char*)msg);
    log_d("[SX1276] Start transmission message (%uB): %s", len, (char*)msg);

    transmissionState = chip.startTransmit(msg);
    if (transmissionState != RADIOLIB_ERR_NONE) {
      log_e("[SX1276]    send failed");

      return std::nullopt;
    }

    return chip.getTimeOnAir(len);
  }

  void finishTransmission() {
    if (transmissionState == RADIOLIB_ERR_NONE) {
      log_d("[SX1276]   transmission finished!");
    } else {
      log_e("[SX1276]   transmission failed! Code %d", transmissionState);
    }

    // clean up after transmission is finished this will ensure transmitter is disabled
    chip.finishTransmit();
  }

  void processRadioQueue(void* args) {
    int clicks;
    static UBaseType_t minFree = UINT32_MAX;

    for (;;) {
      if (pdTRUE != xQueueReceive(xLoRa, &clicks, portMAX_DELAY)) {
        log_e("Lora queue is not responding");
        continue;
      }

      xSemaphoreTake(xTxSemaphore, 0); // clear semaphore anyway
      std::optional<RadioLibTime_t> onAir = startTransmission(prepareClicksMessage(clicks));
      if (!onAir) {
        display::post(display::UiEvent::TxFailed, transmissionState);
        continue;
      }
      digitalWrite(DBG2_PIN, HIGH);

      if (pdTRUE != xSemaphoreTake(xTxSemaphore, pdMS_TO_TICKS(onAir.value() / 1000 + 20))) {
        log_e("[SX1276]    missed transmitted interruption");
        transmissionState = RADIOLIB_ERR_TX_TIMEOUT;
      }

      finishTransmission();

      digitalWrite(DBG2_PIN, LOW);

      // after DBG2 LOW: the UI is not part of the measured path
      if (transmissionState == RADIOLIB_ERR_NONE) {
        display::post(display::UiEvent::TxDone);
      } else {
        display::post(display::UiEvent::TxFailed, transmissionState);
      }

      // stack measure
      UBaseType_t freeNow = uxTaskGetStackHighWaterMark(nullptr);
      if (freeNow < minFree) {
        minFree = freeNow;
        log_i("[RADIO STACK] min free %u B", freeNow);
      }
    }
  }
}  //namespace

void begin() {
  pinMode(DBG2_PIN, OUTPUT);
  digitalWrite(DBG2_PIN, LOW);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS0_PIN);

  log_i("[SX1276] Initializing LoRa...");
  ConfigLoRa_t config;
  config.frequency = LORA_FREQUENCY_MHZ;
  config.bandwidth = LORA_BANDWIDTH_KHZ;
  config.spreadingFactor = LORA_SPREADING_FACTOR;
  config.codingRate = LORA_CODING_RATE;
  config.syncWord = LORA_SYNC_WORD;
  config.power = LORA_POWER_DBM;
  config.preambleLength = LORA_PREAMBLE_LENGTH;

  int state = chip.begin(config);

  if (state != RADIOLIB_ERR_NONE) {
    log_e("[SX1276]   failed, code %d", state);
    return;
  }

  log_i("[SX1276]   success!");

  // init queue and tasks
  xTxSemaphore = xSemaphoreCreateBinary();
  if (xTxSemaphore == nullptr) {
    log_e("Unable to create semaphore");
    return;
  }

  xLoRa = xQueueCreate(10, sizeof(unsigned int));
  if (xLoRa == nullptr) {
    log_e("Unable to create radio queue");
    return;
  }

  BaseType_t status = xTaskCreatePinnedToCore(
    processRadioQueue,
    "processRadioQueue",
    X_LORA_STACK_SIZE,
    nullptr,  // params
    X_LORA_PRIO,
    nullptr,        // pointer
    0
  );
  if (status != pdPASS) {
    log_e("Unable to create queue processing task. Code %d", status);
    return;
  }

  // init interruption
  chip.setPacketSentAction(notifyPacketSent);

  isAvailable = true;
}

BaseType_t sendClicksNotification(unsigned int clicks) {
  if (! isAvailable) {
    return pdFALSE;
  }

  BaseType_t status = xQueueSend(xLoRa, &clicks, pdMS_TO_TICKS(1000));
  if(status != pdTRUE) {
    log_e("radio queue is busy");
  }

  return status;
}

}  // namespace radio
