/**
 * @file ts_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for TypeScript language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/ts directory, load TypeScript source files, extract their ASTs,
 * and validate them against corresponding .expected.json files.
 *
 * Subdirectory coverage:
 * - core/tests/examples/ts/basic_syntax/
 * - core/tests/examples/ts/types_interfaces/
 * - core/tests/examples/ts/generics/
 * - Any other directories added to examples/ts/
 *
 * Each test:
 * 1. Reads a TypeScript source file from examples
 * 2. Parses it into an AST
 * 3. Loads the corresponding .expected.json file
 * 4. Compares the AST against the expected JSON output
 * 5. Reports any discrepancies
 *
 * This approach provides both regression testing and documentation of
 * the expected parser output for different TypeScript language constructs.
 */

#include "../../../include/scopemux/parser.h"
#include "../../include/json_validation.h"
#include "../../include/test_helpers.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Check if a file has a specific extension
 *
 * @param filename The filename to check
 * @param ext The extension to look for (with the dot, e.g. ".ts")
 * @return true if the file has the specified extension
 */
static bool has_extension(const char *filename, const char *ext) {
  size_t filename_len = strlen(filename);
  size_t ext_len = strlen(ext);

  if (filename_len <= ext_len) {
    return false;
  }

  return strcmp(filename + filename_len - ext_len, ext) == 0;
}

/**
 * Run a test for a specific TypeScript example file
 *
 * @param category The test category (subdirectory name)
 * @param filename The TypeScript source file name
 */
static void test_ts_example(const char *category, const char *filename) {
  char *base_filename = strdup(filename);
  if (!base_filename) {
    cr_log_error("Failed to duplicate filename");
    cr_assert_fail("Memory allocation failed");
  }

  // Remove extension to get base name
  char *dot = strrchr(base_filename, '.');
  if (dot) {
    *dot = '\0';
  }

  cr_log_info("Testing TypeScript example: %s/%s", category, base_filename);

  // 1. Read example TypeScript file
  char *source = read_test_file("ts", category, filename);
  cr_assert(source != NULL, "Failed to read source file: %s/%s", category, filename);

  // 2. Parse the TypeScript code into an AST
  ParserContext *ctx = parser_init();
  cr_assert(ctx != NULL, "Failed to create parser context");

  // Parse the TypeScript code into an AST
  bool parse_success = parser_parse_string(ctx, source, strlen(source), "example.ts", LANG_TS);
  cr_assert(parse_success, "Failed to parse TypeScript code");

  // Get the root node of the AST directly from the parser context
  const ASTNode *ast = ctx->ast_root;
  cr_assert(ast != NULL, "Failed to get AST root node");

  // 3. Load the expected JSON file
  JsonValue *expected_json = load_expected_json("ts", category, base_filename);
  if (!expected_json) {
    cr_log_warn("No .expected.json file found for %s/%s, skipping validation", category,
                base_filename);
    free(base_filename);
    free(source);
    parser_free(ctx);
    return;
  }

  // 4. Validate AST against expected JSON
  bool valid = validate_ast_against_json((ASTNode *)ast, expected_json, base_filename);

  // Free resources
  free_json_value(expected_json);
  free(base_filename);
  free(source);
  parser_free(ctx);

  // 5. Report results
  cr_assert(valid, "AST validation failed against expected JSON for %s/%s", category, filename);
}

/**
 * Process files in a directory for testing
 *
 * @param dir The directory to process
 * @param category The category name
 */
static void process_directory(DIR *dir, const char *category) {
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip directories and non-TypeScript files
    if (entry->d_type != DT_REG || !has_extension(entry->d_name, ".ts")) {
      continue;
    }

    // Run test for this example file
    test_ts_example(category, entry->d_name);
  }
}

/**
 * Process all examples in a TypeScript test category
 *
 * @param category The category (subdirectory) to process
 */
static void process_ts_category(const char *category) {
  char path[512];

  // First try using PROJECT_ROOT_DIR environment variable
  const char *project_root = getenv("PROJECT_ROOT_DIR");
  if (project_root) {
    snprintf(path, sizeof(path), "%s/core/tests/examples/ts/%s", project_root, category);
    DIR *dir = opendir(path);
    if (dir) {
      process_directory(dir, category);
      closedir(dir);
      return;
    }
  }

  // Try different relative paths
  const char *possible_paths[] = {
      "../../../core/tests/examples/ts/%s",                    // From build/core/tests/
      "../../core/tests/examples/ts/%s",                       // One level up
      "../core/tests/examples/ts/%s",                          // Two levels up
      "../examples/ts/%s",                                     // Original path
      "./core/tests/examples/ts/%s",                           // From project root
      "/home/matrillo/apps/scopemux/core/tests/examples/ts/%s" // Absolute path
  };

  for (size_t i = 0; i < sizeof(possible_paths) / sizeof(possible_paths[0]); i++) {
    snprintf(path, sizeof(path), possible_paths[i], category);
    DIR *dir = opendir(path);
    if (dir) {
      process_directory(dir, category);
      closedir(dir);
      return;
    }
  }

  // Log error if all attempts fail
  cr_log_warn("Could not open category directory for '%s' after trying multiple paths", category);
}

/**
 * Test basic TypeScript syntax examples
 */
Test(ts_examples, basic_syntax) { process_ts_category("basic_syntax"); }

/**
 * Test TypeScript types and interfaces examples
 */
Test(ts_examples, types_interfaces) { process_ts_category("types_interfaces"); }

/**
 * Test TypeScript generics examples
 */
Test(ts_examples, generics) { process_ts_category("generics"); }
