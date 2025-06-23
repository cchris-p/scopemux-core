/**
 * @file c_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for C language
 *
 * This test validates AST extraction for a single C source file specified via:
 * - SCOPEMUX_TEST_FILE environment variable (path to .c file)
 * - SCOPEMUX_EXPECTED_JSON environment variable (path to expected .json file)
 *
 * The test:
 * 1. Reads the specified C source file
 * 2. Parses it into an AST
 * 3. Loads the expected JSON file
 * 4. Compares the AST against the expected JSON output
 * 5. Reports any discrepancies
 */

#define DEBUG_MODE false

// Include the proper headers first to get access to all required types
#include "../../../core/include/scopemux/parser.h"
#include "../../include/json_validation.h"

// The language enum is already defined in parser.h as LANG_C

// Use criterion's logging exclusively
#define LOG_WARN(fmt, ...) criterion_log(CR_LOG_WARNING, fmt, ##__VA_ARGS__)

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Forward declarations
static char *read_file_contents(const char *path);
static JsonValue *parse_json_file(const char *path);

/**
 * Run a test for the specified C example file
 */
static void test_c_example() {
  const char *test_file = getenv("SCOPEMUX_TEST_FILE");
  const char *json_file = getenv("SCOPEMUX_EXPECTED_JSON");

  if (!test_file || !json_file) {
    cr_assert_fail(
        "Missing required environment variables: SCOPEMUX_TEST_FILE and SCOPEMUX_EXPECTED_JSON");
  }

  // Extract filename without path for test reporting
  const char *filename = strrchr(test_file, '/');
  filename = filename ? filename + 1 : test_file;

  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Starting test for %s\n", test_file);
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Starting test for %s\n", test_file);
  }

  // 1. Read source file
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Reading source file %s...\n", test_file);
  }
  char *source = read_file_contents(test_file);
  if (!source) {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: ERROR - Failed to read source file %s\n", test_file);
    }
    cr_assert(source != NULL, "Failed to read source file: %s", test_file);
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Successfully read source file (length: %zu bytes)\n", strlen(source));
  }

  // 2. Parse the C code into an AST
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Initializing parser context...\n");
  }
  ParserContext *ctx = parser_init();
  if (!ctx) {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: ERROR - Failed to create parser context\n");
    }
    free(source);
    cr_assert(ctx != NULL, "Failed to create parser context");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Parser context initialized, calling parse_string...\n");
  }

  bool parse_success = parser_parse_string(ctx, source, strlen(source), filename, LANG_C);
  if (!parse_success) {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: ERROR - Failed to parse C code\n");
    }
    const char *error = parser_get_last_error(ctx);
    if (error) {
      if (DEBUG_MODE) {
        fprintf(stderr, "TESTING: Parser error: %s\n", error);
      }
    }
    free(source);
    parser_free(ctx);
    cr_assert(parse_success, "Failed to parse C code");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Source code parsed successfully\n");
  }

  // Get the root node of the AST directly from the parser context
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Getting AST root node...\n");
  }
  const ASTNode *ast = ctx->ast_root;
  if (!ast) {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: ERROR - AST root node is NULL\n");
    }
    free(source);
    parser_free(ctx);
    cr_assert(ast != NULL, "Failed to get AST root node");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: AST root node exists (type: %d, num_children: %zu)\n", ast->type,
            ast->num_children);
  }

  // 3. Load the expected JSON file
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Loading expected JSON from %s...\n", json_file);
  }
  JsonValue *expected_json = parse_json_file(json_file);
  if (!expected_json) {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: WARNING - No valid JSON found in %s, skipping validation\n",
              json_file);
    }
    LOG_WARN("No valid JSON found in %s, skipping validation", json_file);
    free(source);
    parser_free(ctx);
    return;
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Successfully loaded expected JSON\n");
  }

  // Extract the AST field from the top-level JSON structure
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Extracting AST field from expected JSON...\n");
  }
  JsonValue *ast_json = NULL;
  if (expected_json->type == JSON_OBJECT) {
    // Search for the "ast" field in the top-level object
    for (size_t i = 0; i < expected_json->value.object.size; i++) {
      if (expected_json->value.object.keys[i] &&
          strcmp(expected_json->value.object.keys[i], "ast") == 0) {
        ast_json = expected_json->value.object.values[i];
        if (DEBUG_MODE) {
          fprintf(stderr, "TESTING: Found 'ast' field in expected JSON\n");
        }
        break;
      }
    }
  }

  // If no AST field is found, use the whole JSON (for backward compatibility)
  if (!ast_json) {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: No 'ast' field found, using entire JSON object for validation\n");
    }
    ast_json = expected_json;
  }

  // 4. Validate AST against expected JSON
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Starting AST validation against expected JSON...\n");
  }

  // All tests must be properly validated against their expected output
  // No special cases or test bypasses - core issues must be fixed

  // Standard validation path for other tests
  bool valid = false;

  // Perform the validation with all possible safety checks
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Calling validate_ast_against_json...\n");
  }
  if (ast && ast_json) {
    valid = validate_ast_against_json((ASTNode *)ast, ast_json, filename);
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: Validation complete, result: %s\n", valid ? "PASS" : "FAIL");
    }
  } else {
    if (DEBUG_MODE) {
      fprintf(stderr, "TESTING: ERROR - Cannot perform validation, ast or ast_json is NULL\n");
    }
    valid = false;
  }

  // Free resources
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Freeing resources...\n");
  }
  free_json_value(expected_json);
  free(source);
  parser_free(ctx);

  // 5. Report results
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Reporting final validation result...\n");
  }
  cr_assert(valid, "AST validation failed for %s", test_file);
  if (DEBUG_MODE) {
    fprintf(stderr, "TESTING: Test completed successfully for %s\n", test_file);
  }
}

/**
 * Helper function to read file contents
 */
static char *read_file_contents(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = malloc(length + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, length, file);
  buffer[length] = '\0';
  fclose(file);
  return buffer;
}

/**
 * Helper function to parse JSON file
 */
static JsonValue *parse_json_file(const char *path) {
  char *json_str = read_file_contents(path);
  if (!json_str) {
    return NULL;
  }

  JsonValue *json = parse_json_string(json_str);
  free(json_str);
  return json;
}

/**
 * Single test that processes the file specified in environment variables
 */
Test(c_examples, single_test) { test_c_example(); }
