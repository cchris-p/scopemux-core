/**
 * @file cpp_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for C++ language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/cpp directory, load C++ source files, extract their ASTs,
 * and validate them against corresponding .expected.json files.
 *
 * Subdirectory coverage:
 * - core/tests/examples/cpp/basic_syntax/
 * - core/tests/examples/cpp/templates/
 * - core/tests/examples/cpp/classes/
 * - core/tests/examples/cpp/namespaces/
 * - core/tests/examples/cpp/stl/
 * - core/tests/examples/cpp/modern_cpp/
 * - Any other directories added to examples/cpp/
 *
 * Each test:
 * 1. Reads a C++ source file from examples
 * 2. Parses it into an AST
 * 3. Loads the corresponding .expected.json file
 * 4. Compares the AST against the expected JSON output
 * 5. Reports any discrepancies
 *
 * This approach provides both regression testing and documentation of
 * the expected parser output for different C++ language constructs.
 */

// NOTE: The following include path is correct for this test file location.
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

// Function prototypes for non-static functions defined in this file
// (none needed; all helper functions in this file are declared static and not used elsewhere)

/**
 * Check if a file has a specific extension
 *
 * @param filename The filename to check
 * @param ext The extension to look for (with the dot, e.g. ".cpp")
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
 * Check if a file is a C++ source file
 *
 * @param filename The filename to check
 * @return true if the file has a C++ extension (.cpp, .cc, .cxx, etc.)
 */
static bool is_cpp_source_file(const char *filename) {
  return has_extension(filename, ".cpp") || has_extension(filename, ".cc") ||
         has_extension(filename, ".cxx") || has_extension(filename, ".hpp") ||
         has_extension(filename, ".h");
}

/**
 * Run a test for a specific C++ example file
 *
 * @param category The test category (subdirectory name)
 * @param filename The C++ source file name
 */
static void test_cpp_example(const char *category, const char *filename) {
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

  cr_log_info("Testing C++ example: %s/%s", SAFE_STR(category), SAFE_STR(base_filename));

  // 1. Read example C++ file
  char *source = read_test_file("cpp", category, filename);
  cr_assert(source != NULL, "Failed to read source file: %s/%s", category, filename);

  // 2. Parse the C++ code into an AST
  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Failed to initialize parser context");

  ASTNode *ast = parse_cpp_ast(ctx, source);
  cr_assert(ast != NULL, "Failed to parse C++ code into AST");

  // 3. Load the expected JSON file
  JsonValue *expected_json = load_expected_json("cpp", category, base_filename);
  if (!expected_json) {
    log_warning("No .expected.json file found for %s/%s, skipping validation", SAFE_STR(category),
                SAFE_STR(base_filename));
    free(base_filename);
    free(source);
    parser_context_free(ctx);
    return;
  }

  // 4. Validate AST against expected JSON
  bool valid = validate_ast_against_json(ast, expected_json, base_filename);

  // Free resources
  free_json_value(expected_json);
  free(base_filename);
  free(source);
  parser_context_free(ctx);

  // 5. Report results
  cr_assert(valid, "AST validation failed against expected JSON for %s/%s", category, filename);
}

/**
 * Process all examples in a C++ test category
 *
 * @param category The category (subdirectory) to process
 */
static void process_cpp_category(const char *category) {
  char path[512];
  snprintf(path, sizeof(path), "../examples/cpp/%s", category);

  DIR *dir = opendir(path);
  if (!dir) {
    log_warning("Could not open category directory: %s", path);
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip directories and non-C++ files
    if (entry->d_type != DT_REG || !is_cpp_source_file(entry->d_name)) {
      continue;
    }

    // Run test for this example file
    test_cpp_example(category, entry->d_name);
  }

  closedir(dir);
}

/**
 * Test basic C++ syntax examples
 */
Test(cpp_examples, basic_syntax) { process_cpp_category("basic_syntax"); }

/**
 * Test C++ template examples
 */
Test(cpp_examples, templates) { process_cpp_category("templates"); }

/**
 * Test C++ class examples
 */
Test(cpp_examples, classes) { process_cpp_category("classes"); }

/**
 * Test C++ namespace examples
 */
Test(cpp_examples, namespaces) { process_cpp_category("namespaces"); }

/**
 * Test C++ STL examples
 */
Test(cpp_examples, stl) { process_cpp_category("stl"); }

/**
 * Test modern C++ examples
 */
Test(cpp_examples, modern_cpp) { process_cpp_category("modern_cpp"); }
