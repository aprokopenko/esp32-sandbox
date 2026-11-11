#pragma once
#include <stdint.h>
#include <stddef.h>
#include "drivers/transport.h"

namespace drivers {

// Sends the text in an HTTPS POST body (works with ntfy.sh).
class HttpsTransport : public Transport {
public:
  static constexpr int ERR_NO_URL = -100;
  static constexpr int ERR_BAD_URL = -101;

  explicit HttpsTransport(const char* url);
  int send(const Notification& n) override;

private:
  static constexpr size_t MAX_URL_LEN = 200;
  static constexpr uint16_t TIMEOUT_MS = 10000;

  char _url[MAX_URL_LEN];
};

}  // namespace drivers
