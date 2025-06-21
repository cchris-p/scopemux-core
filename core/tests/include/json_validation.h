#ifndef SCOPEMUX_JSON_VALIDATION_H
#define SCOPEMUX_JSON_VALIDATION_H

#include "../../include/scopemux/parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Simple JSON structure to hold parsed values
 */
typedef struct JsonValue {
  enum { JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_NUMBER, JSON_BOOLEAN, JSON_NULL } type;

  union {
    struct {
      char **keys;
      struct JsonValue **values;
      size_t size;
    } object;

    struct {
      struct JsonValue **items;
      size_t size;
    } array;

    char *string;
    double number;
    bool boolean;
  } value;
} JsonValue;

/**
 * Load and parse an expected JSON file for validation
 *
 * @param language Language subdirectory name
 * @param category Category subdirectory name
 * @param file_name Test filename (without .expected.json extension)
 * @return Pointer to parsed JSON structure, or NULL on failure
 */
JsonValue *load_expected_json(const char *language, const char *category, const char *file_name);

/**
 * Free memory allocated for JSON structure
 *
 * @param json JSON value to free
 */
void free_json_value(JsonValue *json);

/**
 * Parse a JSON string
 *
 * @param json_str JSON string to parse
 * @return Pointer to parsed JSON structure, or NULL on failure
 */
JsonValue *parse_json_string(const char *json_str);

/**
 * Validate AST against expected JSON
 *
 * @param node AST node to validate
 * @param expected Expected JSON definition
 * @param node_path Path to current node (for error reporting)
 * @return true if validation passes, false otherwise
 */
bool validate_ast_against_json(ASTNode *node, JsonValue *expected, const char *node_path);

/**
 * Print JSON structure for debugging
 *
 * @param json JSON value to print
 * @param level Indentation level
 */
void print_json_value(JsonValue *json, int level);

#endif /* SCOPEMUX_JSON_VALIDATION_H */
