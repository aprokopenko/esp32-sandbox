#pragma once
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "event.h"
#include "notification.h"
#include "drivers/transport.h"

namespace services {

// What to say: event -> text -> Transport::send() -> NotifyDone event.
// Runs in its own low-priority task, so a slow HTTPS request blocks nobody.
class Notifier {
public:
  Notifier(QueueHandle_t events, drivers::Transport& transport);
  void begin();

  // Never waits: a full queue drops the event and returns false.
  bool send(const Event& e);

private:
  static constexpr uint8_t TASK_PRIO = 2;
  static constexpr uint32_t TASK_STACK = 10240;  // TLS handshake needs a large stack
  static constexpr uint8_t TASK_CORE = 1;
  static constexpr uint8_t QUEUE_LENGTH = 10;

  QueueHandle_t _events;
  QueueHandle_t _queue = nullptr;
  drivers::Transport& _transport;
  bool _available = false;
  uint32_t _dropped = 0;

  static void xHandle(void* args);
  void handle();
  void format(const Event& e, Notification& n);
};

}  // namespace services
