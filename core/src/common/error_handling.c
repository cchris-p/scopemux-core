/**
 * @file error_handling.c
 * @brief Implementation of error handling utilities for ScopeMux
 *
 * This module provides utilities for error handling, including
 * error message management and error code definitions.
 */

#include "../../core/include/scopemux/common.h"
#include "../../core/include/scopemux/memory_debug.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define a maximum error message length
#define MAX_ERROR_LENGTH 1024

/**
 * @brief Set an error message in a buffer
 *
 * @param error_buffer Buffer to store the error message
 * @param format Format string
 * @param ... Additional arguments
 * @return The error message
 */
char *set_error(char **error_buffer, const char *format, ...) {
  // Allocate or reallocate the error buffer if needed
  if (*error_buffer == NULL) {
    *error_buffer = MALLOC(MAX_ERROR_LENGTH, "error_buffer");
    if (*error_buffer == NULL) {
      return NULL; // Out of memory
    }
  }

  // Format the error message
  va_list args;
  va_start(args, format);
  vsnprintf(*error_buffer, MAX_ERROR_LENGTH, format, args);
  va_end(args);

  return *error_buffer;
}

/**
 * @brief Free an error buffer
 *
 * @param error_buffer Buffer to free
 */
void free_error(char **error_buffer) {
  if (*error_buffer != NULL) {
    FREE(*error_buffer);
    *error_buffer = NULL;
  }
}

/**
 * @brief Check if a condition is true, and set an error if not
 *
 * @param condition Condition to check
 * @param error_buffer Buffer to store the error message
 * @param format Format string
 * @param ... Additional arguments
 * @return bool True if the condition is true, false otherwise
 */
bool check_error(bool condition, char **error_buffer, const char *format, ...) {
  if (!condition) {
    // Format the error message
    va_list args;
    va_start(args, format);
    set_error(error_buffer, format, args);
    va_end(args);
    return false;
  }
  return true;
}
