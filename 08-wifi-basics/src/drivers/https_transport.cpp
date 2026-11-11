#include "https_transport.h"
#include <string.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

namespace drivers {

HttpsTransport::HttpsTransport(const char* url) {
  strlcpy(_url, url ? url : "", sizeof(_url));
}

int HttpsTransport::send(const Notification& n) {
  if (_url[0] == '\0') {
    return ERR_NO_URL;
  }

  // TLS buffers (~40-50 KB) live on the heap only during this call
  WiFiClientSecure client;
  client.setInsecure();  // prototype: the traffic is encrypted, but the server is not verified

  HTTPClient http;
  http.setTimeout(TIMEOUT_MS);
  if (!http.begin(client, _url)) {
    return ERR_BAD_URL;
  }

  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  // HTTPClient takes a non-const pointer, but only reads the payload
  uint8_t* payload = reinterpret_cast<uint8_t*>(const_cast<char*>(n.text));
  int code = http.POST(payload, strlen(n.text));

  http.end();
  return code;  // HTTP status, or a negative HTTPC_ERROR_* code
}

}  // namespace drivers
