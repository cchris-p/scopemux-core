/**
 * @file ts_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for TypeScript language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/ts directory, load TypeScript source files, extract their ASTs,
 * and validate them against corresponding .expected.json files.
 */

#include "../../include/ast_test_utils.h"
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
 */
static void test_ts_example(const char *category, const char *filename) {
  // Get test paths
  TestPaths paths = construct_test_paths("ts", category, filename);
  if (!paths.base_filename) {
    cr_log_error("Failed to construct test paths");
    cr_assert_fail("Memory allocation failed");
  }

  // Initialize test configuration
  ASTTestConfig config = ast_test_config_init();
  config.source_file = paths.source_path;
  config.json_file = paths.json_path;
  config.category = category;
  config.base_filename = paths.base_filename;
  config.language = LANG_TYPESCRIPT;
  config.debug_mode = true;

  // Run the test
  bool test_passed = run_ast_test(&config);

  // Cleanup
  free(paths.base_filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Test that processes all TypeScript example files
 */
Test(ts_examples, all_examples) {
  const char *categories[] = {"basic_syntax", "interfaces_and_types", "decorators", NULL};

  for (int i = 0; categories[i] != NULL; i++) {
    process_category_files("ts", categories[i], (bool (*)(const char *, const char *))has_extension,
                           test_ts_example);
  }
}
