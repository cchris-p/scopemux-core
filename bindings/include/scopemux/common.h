/**
 * @file common.h
 * @brief Common utilities and type definitions for ScopeMux
 * 
 * This module provides common utilities and type definitions used
 * throughout the ScopeMux codebase.
 */

#ifndef SCOPEMUX_COMMON_H
#define SCOPEMUX_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * @brief Error handling functions
 */
char* set_error(char** error_buffer, const char* format, ...);
void free_error(char** error_buffer);
bool check_error(bool condition, char** error_buffer, const char* format, ...);

/**
 * @brief Memory management functions
 */
void* safe_malloc(size_t size);
void* safe_realloc(void* ptr, size_t size);
void safe_free(void* ptr);
char* safe_strdup(const char* str);
void* memory_pool_init(size_t size);
void* memory_pool_alloc(void* pool, size_t size);
void memory_pool_free(void* pool);

/**
 * @brief Logging functions
 */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

bool log_init(LogLevel level, const char* log_path);
void log_cleanup(void);
void log_set_level(LogLevel level);
void log_message(LogLevel level, const char* format, ...);
void log_debug(const char* format, ...);
void log_info(const char* format, ...);
void log_warning(const char* format, ...);
void log_error(const char* format, ...);

#endif /* SCOPEMUX_COMMON_H */
