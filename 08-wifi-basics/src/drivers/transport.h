#pragma once
#include "notification.h"

namespace drivers {

// How a notification travels. Notifier knows only this interface.
class Transport {
public:
  virtual ~Transport() = default;

  // Blocks until delivered or failed.
  // Returns a protocol status (HTTP code for HTTPS), or a negative error.
  virtual int send(const Notification& n) = 0;
};

}  // namespace drivers
