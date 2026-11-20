#include "ir_keyboard.h"
#include <Arduino.h>
#include <inttypes.h>

namespace drivers {

IRKeyboard::IRKeyboard(uint8_t pin, uint8_t handlerPriority, uint8_t handlerCore)
  : _pin(pin), _handlerPriority(handlerPriority), _handlerCore(handlerCore) {}

void IRKeyboard::begin() {
  pinMode(_pin, INPUT_PULLUP);

  BaseType_t status = xTaskCreatePinnedToCore(
    irHandlerCallback,
    "irReceiver",
    HANDLER_STACK,
    this,
    _handlerPriority,
    nullptr,
    _handlerCore
  );
  if (status != pdPASS) {
    log_e("Unable to start IR signal handler.");
  }
}

void IRKeyboard::irHandlerCallback(void *arg) {
  static_cast<IRKeyboard *>(arg)->irHandler();
}

void IRKeyboard::beginRmtRx() {
  rmt_rx_channel_config_t rxCfg = {
    .gpio_num = static_cast<gpio_num_t>(_pin),
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = IR_RESOLUTION_HZ,
    .mem_block_symbols = 64,  // amount of RMT symbols that the channel can store at a time
  };

  ESP_ERROR_CHECK(rmt_new_rx_channel(&rxCfg, &_rmtChl));

  _rxQueue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
  assert(_rxQueue);

  rmt_rx_event_callbacks_t callbacks = {
    .on_recv_done = rmtRxIsrCallback
  };
  ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(_rmtChl, &callbacks, this));

  ESP_ERROR_CHECK(rmt_enable(_rmtChl));
  log_i("RMT channel configured.");
}

IRAM_ATTR bool IRKeyboard::rmtRxIsrCallback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *arg) {
  return static_cast<IRKeyboard *>(arg)->rmtRxIsr(edata);
}

IRAM_ATTR bool IRKeyboard::rmtRxIsr(const rmt_rx_done_event_data_t *edata) {
  BaseType_t woken = pdFALSE;
  // send the received RMT symbols to the parser task
  xQueueSendFromISR(_rxQueue, edata, &woken);

  return woken == pdTRUE;
}

void IRKeyboard::irHandler() {
  beginRmtRx();

  rmt_receive_config_t rxCfg = {
    .signal_range_min_ns = NEC_MIN_NS,  // the shortest duration for NEC signal is 560us, 1250ns < 560us, valid signal won't be treated as noise
    .signal_range_max_ns = NEC_MAX_NS,  // the longest duration for NEC signal is 9000us, 12000000ns > 9000us, the receive won't stop early
  };

  rmt_symbol_word_t buf[64];  // 64 symbols should be sufficient for a standard NEC frame
  rmt_rx_done_event_data_t rxData;

  // ready to receive
  ESP_ERROR_CHECK(rmt_receive(_rmtChl, buf, sizeof(buf), &rxCfg));

  log_i("RMT listening...");

  for (;;) {
    xQueueReceive(_rxQueue, &rxData, portMAX_DELAY);


    if (rxData.num_symbols < 34) {
      // skip repeat signals for long press
      ESP_ERROR_CHECK(rmt_receive(_rmtChl, buf, sizeof(buf), &rxCfg));
      continue;
    }

    log_d("IR items received:");

    uint32_t package = 0x00;
    for (size_t i = 1; i <= 32; i++) {
      auto item = rxData.received_symbols[i];
      uint32_t inBit = item.duration1 > RMT_EDGE_LVL;
      package |= inBit << (i - 1);
      log_v("  %u   |   %u   |   %u   |   %u", item.level0, item.duration0, item.level1, item.duration1);
    }

    uint8_t addr = (package >> (8 * 0)) & 0xFF;    // take 1st part
    uint8_t cmd = (package >> (8 * 2)) & 0xFF;     // take 3rd part
    uint8_t notCmd = (package >> (8 * 3)) & 0xFF;  // take 4th part

    log_d("Package: %08" PRIx32 ", addr: 0x%0x cmd: 0x%0x", package, addr, cmd);

    // wait next package
    ESP_ERROR_CHECK(rmt_receive(_rmtChl, buf, sizeof(buf), &rxCfg));

    if ((cmd ^ notCmd) != 0xFF) {
      log_w("Command checksum failed (cmd/~cmd: 0x%0x / 0x%0x), skipping", cmd, notCmd);
      continue;
    }

    // fire callback
    if (_onRx) {
      _onRx(parseNecCmd(cmd));
    }
  }
}

RmtKeyboardKey IRKeyboard::parseNecCmd(uint8_t necCmd) {
  RmtKeyboardKey k;

  switch (necCmd) {
    case 0x46: k = RmtKeyboardKey::Up; break;
    case 0x15: k = RmtKeyboardKey::Down; break;
    case 0x4A: k = RmtKeyboardKey::Sharp; break;
    default: k = RmtKeyboardKey::Undefined; break;
  }

  return k;
}

void IRKeyboard::onRx(kbCallback_t cb) {
  _onRx = cb;
}

}  //namespace drivers
