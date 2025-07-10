/**
 * @file python_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for Python language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/python directory, load Python source files, extract their ASTs,
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
 * Run a test for a specific Python example file
 */
static void test_python_example(const char *category, const char *filename) {
  // Get test paths
  TestPaths paths = construct_test_paths("python", category, filename);
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
  config.language = LANG_PYTHON;
  config.debug_mode = true;

  // Run the test
  bool test_passed = run_ast_test(&config);

  // Cleanup
  free(paths.base_filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Test that processes all Python example files
 */
Test(python_examples, all_examples) {
  const char *categories[] = {"basic_syntax", "functions_and_classes", "decorators", "type_hints",
                              NULL};

  for (int i = 0; categories[i] != NULL; i++) {
    process_category_files("python", categories[i], has_extension, test_python_example);
  }
}
