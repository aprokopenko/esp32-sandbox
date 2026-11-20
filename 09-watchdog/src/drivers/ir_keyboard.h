#pragma once
#include <stdint.h>
#include <driver/rmt_rx.h>
#include <functional>
#include "rmt_keyboard.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

namespace drivers {

class IRKeyboard : public RmtKeyboard {
public:
  explicit IRKeyboard(uint8_t pin, uint8_t handlerPriority = 3, uint8_t handlerCore = 0);
  void begin() override;
  void onRx(kbCallback_t cb) override;

private:
  static constexpr uint32_t IR_RESOLUTION_HZ = 1000000; // 1MHz
  static constexpr uint16_t RMT_EDGE_LVL = 1100;
  static constexpr uint32_t NEC_MIN_NS = 1250;
  static constexpr uint32_t NEC_MAX_NS = 12000000;
  static constexpr uint32_t HANDLER_STACK = 4096;

  uint8_t _pin;
  uint8_t _handlerPriority;
  uint8_t _handlerCore;
  QueueHandle_t _rxQueue = nullptr;
  rmt_channel_handle_t _rmtChl = nullptr;
  kbCallback_t _onRx = nullptr;

  static bool rmtRxIsrCallback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *arg);
  bool rmtRxIsr(const rmt_rx_done_event_data_t *edata);
  static void irHandlerCallback(void *arg);
  void beginRmtRx();
  void irHandler();
  RmtKeyboardKey parseNecCmd(uint8_t necCmd);
};

} //namespace drivers
