/**
 * @file c_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for C language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/c directory, load C source files, extract their ASTs,
 * and validate them against corresponding .expected.json files.
 *
 * Subdirectory coverage:
 * - core/tests/examples/c/basic_syntax/
 * - core/tests/examples/c/core_constructs/
 * - core/tests/examples/c/control_flow/
 * - core/tests/examples/c/preprocessor/
 * - Any other directories added to examples/c/
 *
 * Each test:
 * 1. Reads a C source file from examples
 * 2. Parses it into an AST
 * 3. Loads the corresponding .expected.json file
 * 4. Compares the AST against the expected JSON output
 * 5. Reports any discrepancies
 *
 * This approach provides both regression testing and documentation of
 * the expected parser output for different C language constructs.
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
 * @param ext The extension to look for (with the dot, e.g. ".c")
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
 * Run a test for a specific C example file
 *
 * @param category The test category (subdirectory name)
 * @param filename The C source file name
 */
static void test_c_example(const char *category, const char *filename) {
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
  
  cr_log_info("Testing C example: %s/%s", category, base_filename);
  
  // 1. Read example C file
  char *source = read_test_file("c", category, filename);
  cr_assert(source != NULL, "Failed to read source file: %s/%s", category, filename);
  
  // 2. Parse the C code into an AST
  ParserContext *ctx = parser_init();
  cr_assert(ctx != NULL, "Failed to create parser context");

  // Parse the C code into an AST
  bool parse_success = parser_parse_string(ctx, source, strlen(source), "example.c", LANG_C);
  cr_assert(parse_success, "Failed to parse C code");
  
  // Get the root node of the AST directly from the parser context
  const ASTNode *ast = ctx->ast_root;
  cr_assert(ast != NULL, "Failed to get AST root node");
  
  // 3. Load the expected JSON file
  JsonValue *expected_json = load_expected_json("c", category, base_filename);
  if (!expected_json) {
    cr_log_warn("No .expected.json file found for %s/%s, skipping validation",
               category, base_filename);
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
  cr_assert(valid, "AST validation failed against expected JSON for %s/%s",
            category, filename);
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
    // Skip directories and non-C files
    if (entry->d_type != DT_REG || !has_extension(entry->d_name, ".c")) {
      continue;
    }
    
    // Run test for this example file
    test_c_example(category, entry->d_name);
  }
}

/**
 * Process all examples in a C test category
 *
 * @param category The category (subdirectory) to process
 */
static void process_c_category(const char *category) {
  char path[512];
  
  // First try using PROJECT_ROOT_DIR environment variable
  const char *project_root = getenv("PROJECT_ROOT_DIR");
  if (project_root) {
    snprintf(path, sizeof(path), "%s/core/tests/examples/c/%s", project_root, category);
    DIR *dir = opendir(path);
    if (dir) {
      process_directory(dir, category);
      closedir(dir);
      return;
    }
  }
  
  // Try different relative paths
  const char *possible_paths[] = {
    "../../../core/tests/examples/c/%s",    // From build/core/tests/
    "../../core/tests/examples/c/%s",       // One level up
    "../core/tests/examples/c/%s",          // Two levels up
    "../examples/c/%s",                     // Original path
    "./core/tests/examples/c/%s",           // From project root
    "/home/matrillo/apps/scopemux/core/tests/examples/c/%s" // Absolute path
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
 * Test basic C syntax examples
 */
Test(c_examples, basic_syntax) {
  process_c_category("basic_syntax");
}

/**
 * Test core C constructs examples
 */
Test(c_examples, core_constructs) {
  process_c_category("core_constructs");
}

/**
 * Test C control flow examples
 */
Test(c_examples, control_flow) {
  process_c_category("control_flow");
}

/**
 * Test C preprocessor examples
 */
Test(c_examples, preprocessor) {
  process_c_category("preprocessor");
}
