#pragma once
#include <time.h>

// What to say, shared by Notifier and the transports.
// A fixed buffer, not String: safe to copy byte by byte (through a queue).
struct Notification {
  char text[96];
  time_t epoch;  // when it happened, wall-clock time (0 = unknown)
};
