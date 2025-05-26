/**
 * @file logging.c
 * @brief Implementation of logging utilities for ScopeMux
 * 
 * This module provides utilities for logging messages at different
 * levels (debug, info, warning, error).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// Define log levels
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// Current log level
static LogLevel current_log_level = LOG_INFO;

// Log file
static FILE* log_file = NULL;

/**
 * @brief Initialize the logging system
 * 
 * @param level Initial log level
 * @param log_path Path to log file (NULL for stderr)
 * @return bool True on success, false on failure
 */
bool log_init(LogLevel level, const char* log_path) {
    current_log_level = level;
    
    if (log_path != NULL) {
        log_file = fopen(log_path, "a");
        if (log_file == NULL) {
            fprintf(stderr, "Failed to open log file: %s\n", log_path);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Clean up the logging system
 */
void log_cleanup(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}

/**
 * @brief Set the current log level
 * 
 * @param level New log level
 */
void log_set_level(LogLevel level) {
    current_log_level = level;
}

/**
 * @brief Log a message at the specified level
 * 
 * @param level Log level
 * @param format Format string
 * @param ... Additional arguments
 */
void log_message(LogLevel level, const char* format, ...) {
    if (level < current_log_level) {
        return;
    }
    
    // Get current time
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    
    // Determine level string
    const char* level_str = "UNKNOWN";
    switch (level) {
        case LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case LOG_INFO:
            level_str = "INFO";
            break;
        case LOG_WARNING:
            level_str = "WARNING";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            break;
    }
    
    // Format message
    va_list args;
    va_start(args, format);
    
    // Determine output file
    FILE* output = (log_file != NULL) ? log_file : stderr;
    
    // Print prefix
    fprintf(output, "[%s] [%s] ", time_str, level_str);
    
    // Print message
    vfprintf(output, format, args);
    fprintf(output, "\n");
    
    va_end(args);
    
    // Flush output
    fflush(output);
}

/**
 * @brief Log a debug message
 * 
 * @param format Format string
 * @param ... Additional arguments
 */
void log_debug(const char* format, ...) {
    if (LOG_DEBUG < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // Determine output file
    FILE* output = (log_file != NULL) ? log_file : stderr;
    
    // Get current time
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    
    // Print prefix
    fprintf(output, "[%s] [DEBUG] ", time_str);
    
    // Print message
    vfprintf(output, format, args);
    fprintf(output, "\n");
    
    va_end(args);
    
    // Flush output
    fflush(output);
}

/**
 * @brief Log an info message
 * 
 * @param format Format string
 * @param ... Additional arguments
 */
void log_info(const char* format, ...) {
    if (LOG_INFO < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // Determine output file
    FILE* output = (log_file != NULL) ? log_file : stderr;
    
    // Get current time
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    
    // Print prefix
    fprintf(output, "[%s] [INFO] ", time_str);
    
    // Print message
    vfprintf(output, format, args);
    fprintf(output, "\n");
    
    va_end(args);
    
    // Flush output
    fflush(output);
}

/**
 * @brief Log a warning message
 * 
 * @param format Format string
 * @param ... Additional arguments
 */
void log_warning(const char* format, ...) {
    if (LOG_WARNING < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // Determine output file
    FILE* output = (log_file != NULL) ? log_file : stderr;
    
    // Get current time
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    
    // Print prefix
    fprintf(output, "[%s] [WARNING] ", time_str);
    
    // Print message
    vfprintf(output, format, args);
    fprintf(output, "\n");
    
    va_end(args);
    
    // Flush output
    fflush(output);
}

/**
 * @brief Log an error message
 * 
 * @param format Format string
 * @param ... Additional arguments
 */
void log_error(const char* format, ...) {
    if (LOG_ERROR < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    // Determine output file
    FILE* output = (log_file != NULL) ? log_file : stderr;
    
    // Get current time
    time_t now;
    time(&now);
    struct tm* local_time = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    
    // Print prefix
    fprintf(output, "[%s] [ERROR] ", time_str);
    
    // Print message
    vfprintf(output, format, args);
    fprintf(output, "\n");
    
    va_end(args);
    
    // Flush output
    fflush(output);
}
