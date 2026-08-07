/**
 * @file language.h
 * @brief Language type definitions and utilities for ScopeMux
 *
 * This header defines the supported programming languages and associated
 * utilities for language detection and handling.
 */

#ifndef SCOPEMUX_LANGUAGE_H
#define SCOPEMUX_LANGUAGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumeration of supported programming languages
 */
typedef enum {
  LANG_UNKNOWN = 0,
  LANG_C = 1,
  LANG_CPP = 2,
  LANG_PYTHON = 3,
  LANG_JAVASCRIPT = 4,
  LANG_TYPESCRIPT = 5,
  /* Add more languages as needed */
  LANG_MAX
} Language;

/**
 * Get the string representation of a language
 * @param lang The language enumeration value
 * @return A string representation of the language
 */
const char *language_to_string(Language lang);

/**
 * Parse a language string into the corresponding enumeration value
 * @param lang_str The language string
 * @return The corresponding Language enumeration value, or LANG_UNKNOWN if not recognized
 */
Language language_from_string(const char *lang_str);

/**
 * Detect language from file extension
 * @param file_path The path to the file
 * @return The detected Language, or LANG_UNKNOWN if not detected
 */
Language language_detect_from_extension(const char *file_path);

/**
 * Get the file extension for a given language
 * @param lang The language to get the extension for
 * @return The primary file extension for the language (without dot), or NULL if unknown
 */
const char *language_get_extension(Language lang);

/**
 * Check if a language supports interfile references
 * @param lang The language to check
 * @return true if the language supports interfile references, false otherwise
 */
bool language_supports_interfile_references(Language lang);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_LANGUAGE_H */
