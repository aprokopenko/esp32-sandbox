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
  rmt_config_t cfg = RMT_DEFAULT_CONFIG_RX(static_cast<gpio_num_t>(_pin), RMT_CHL);
  ESP_ERROR_CHECK(rmt_config(&cfg));
  ESP_ERROR_CHECK(rmt_driver_install(RMT_CHL, RMT_BUF_SIZE, 0));

  ESP_ERROR_CHECK(rmt_get_ringbuf_handle(RMT_CHL, &_rb));
  assert(_rb != NULL);

  ESP_ERROR_CHECK(rmt_rx_start(RMT_CHL, true));
  log_i("RMT listening started.");
}

void IRKeyboard::irHandler() {
  beginRmtRx();

  size_t len = 0;
  rmt_item32_t *items = nullptr;

  for (;;) {
    items = (rmt_item32_t *)xRingbufferReceive(_rb, &len, portMAX_DELAY);

    if (items) {
      size_t cnt = len / sizeof(rmt_item32_t);  // each item 4 bytes

      if (cnt < 34) {
        // skip repeat signals for long press
        vRingbufferReturnItem(_rb, (void *) items);
        continue;
      }

      log_d("IR items received:");

      uint32_t package = 0x00;
      for (size_t i = 1; i <= 32; i++) {
        auto item = items[i];
        uint32_t inBit = item.duration1 > RMT_EDGE_LVL;
        package |= inBit << (i-1);
        log_v("  %lu   |   %lu   |   %lu   |   %lu", item.level0, item.duration0, item.level1, item.duration1);
      }

      uint8_t addr = (package >> (8 * 0)) & 0xFF; // take 1st part
      uint8_t cmd = (package >> (8 * 2)) & 0xFF; // take 3rd part
      uint8_t notCmd = (package >> (8 * 3)) & 0xFF; // take 4th part

      vRingbufferReturnItem(_rb, (void *) items);

      log_d("Package: %08" PRIx32 ", addr: 0x%0x cmd: 0x%0x", package, addr, cmd);

      if ( (cmd ^ notCmd) != 0xFF ) {
        log_w("Command checksum failed (cmd/~cmd: 0x%0x / 0x%0x), skipping", cmd, notCmd);
        continue;
      }

      // fire callback
      if (_onRx) {
        _onRx(parseNecCmd(cmd));
      }
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
