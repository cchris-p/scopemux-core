#ifndef SCOPEMUX_LOGGING_H
#define SCOPEMUX_LOGGING_H

#include <stdbool.h>
#include <stdio.h>

// Safely handle NULL strings in format specifiers
#define SAFE_STR(x) ((x) ? (x) : "(null)")

// Log levels
typedef enum { LOG_DEBUG = 0, LOG_INFO, LOG_WARNING, LOG_ERROR } LogLevel;

// Centralized logging API
bool log_init(LogLevel level, const char *log_path);
void log_cleanup(void);
void log_set_level(LogLevel level);

/**
 * @brief Log a message at the specified level.
 *
 * @param level Log level
 * @param format Format string
 * @param ... Additional arguments
 */
void log_message(LogLevel level, const char *format, ...);

void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warning(const char *format, ...);
void log_error(const char *format, ...);

// Optional: Per-file logging toggle pattern
// static int file_logging_enabled = 1; // in your .c file
// if (file_logging_enabled) log_debug(...);

#endif // SCOPEMUX_LOGGING_H
