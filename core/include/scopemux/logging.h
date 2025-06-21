#ifndef SCOPEMUX_LOGGING_H
#define SCOPEMUX_LOGGING_H

#include <stdio.h>

// Global toggle for logging. Define this in one .c file (e.g., main or test harness)
extern int logging_enabled;

#define LOG_DEBUG(fmt, ...)                                                                        \
  do {                                                                                             \
    if (logging_enabled)                                                                           \
      fprintf(stderr, "DIRECT DEBUG: " fmt "\n", ##__VA_ARGS__);                                   \
  } while (0)

#define LOG_ERROR(fmt, ...) fprintf(stderr, "DIRECT ERROR: " fmt "\n", ##__VA_ARGS__)

#endif // SCOPEMUX_LOGGING_H
