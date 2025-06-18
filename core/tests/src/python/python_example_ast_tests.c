/**
 * @file python_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for Python language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/python directory, load Python source files, extract their ASTs,
 * and validate them against corresponding .expected.json files.
 *
 * Subdirectory coverage:
 * - core/tests/examples/python/basic_syntax/
 * - core/tests/examples/python/advanced_features/
 * - core/tests/examples/python/classes/
 * - core/tests/examples/python/decorators/
 * - core/tests/examples/python/type_hints/
 * - Any other directories added to examples/python/
 *
 * Each test:
 * 1. Reads a Python source file from examples
 * 2. Parses it into an AST
 * 3. Loads the corresponding .expected.json file
 * 4. Compares the AST against the expected JSON output
 * 5. Reports any discrepancies
 *
 * This approach provides both regression testing and documentation of
 * the expected parser output for different Python language constructs.
 */

#include "../../include/json_validation.h"
#include "../../include/test_helpers.h"
#include "../../../include/scopemux/parser.h"
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
 * @param ext The extension to look for (with the dot, e.g. ".py")
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
 * Run a test for a specific Python example file
 *
 * @param category The test category (subdirectory name)
 * @param filename The Python source file name
 */
static void test_python_example(const char *category, const char *filename) {
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
  
  cr_log_info("Testing Python example: %s/%s", category, base_filename);
  
  // 1. Read example Python file
  char *source = read_test_file("python", category, filename);
  cr_assert(source != NULL, "Failed to read source file: %s/%s", category, filename);
  
  // 2. Parse the Python code into an AST
  ParserContext *ctx = parser_context_new();
  cr_assert(ctx != NULL, "Failed to create parser context");
  
  ASTNode *ast = parse_python_ast(ctx, source);
  cr_assert(ast != NULL, "Failed to parse Python code into AST");
  
  // 3. Load the expected JSON file
  JsonValue *expected_json = load_expected_json("python", category, base_filename);
  if (!expected_json) {
    cr_log_warn("No .expected.json file found for %s/%s, skipping validation",
               category, base_filename);
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
  cr_assert(valid, "AST validation failed against expected JSON for %s/%s",
            category, filename);
}

/**
 * Process all examples in a Python test category
 *
 * @param category The category (subdirectory) to process
 */
static void process_python_category(const char *category) {
  char path[512];
  snprintf(path, sizeof(path), "../examples/python/%s", category);
  
  DIR *dir = opendir(path);
  if (!dir) {
    cr_log_warn("Could not open category directory: %s", path);
    return;
  }
  
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip directories and non-Python files
    if (entry->d_type != DT_REG || !has_extension(entry->d_name, ".py")) {
      continue;
    }
    
    // Run test for this example file
    test_python_example(category, entry->d_name);
  }
  
  closedir(dir);
}

/**
 * Test basic Python syntax examples
 */
Test(python_examples, basic_syntax) {
  process_python_category("basic_syntax");
}

/**
 * Test advanced Python features examples
 */
Test(python_examples, advanced_features) {
  process_python_category("advanced_features");
}

/**
 * Test Python classes examples
 */
Test(python_examples, classes) {
  process_python_category("classes");
}

/**
 * Test Python decorators examples
 */
Test(python_examples, decorators) {
  process_python_category("decorators");
}

/**
 * Test Python type hints examples
 */
Test(python_examples, type_hints) {
  process_python_category("type_hints");
}
