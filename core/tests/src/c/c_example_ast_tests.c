/**
 * @file c_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for C language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/c directory, load C source files, extract their ASTs,
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
 * Run a test for a specific C example file
 */
static void test_c_example(const char *category, const char *filename) {
  // Get test paths
  TestPaths paths = construct_test_paths("c", category, filename);
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
  config.language = LANG_C;
  config.debug_mode = true;

  // Run the test
  bool test_passed = run_ast_test(&config);

  // Cleanup
  free(paths.base_filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Check if a file is a C source file
 *
 * @param filename The filename to check if the file is a C source file. This should be a more
 * robust check.
 *
 * TODO: Consider orientating this function to check if the file is a test file for a given
 * language.
 */
static bool is_c_file(const char *filename) { return has_extension(filename, ".c"); }

/**
 * Test that processes all C example files
 */
Test(c_examples, all_examples) {
  const char *categories[] = {"basic_syntax",      "complex_structures", "file_io",
                              "memory_management", "struct_union_enum",  NULL};

  for (int i = 0; categories[i] != NULL; i++) {
    process_category_files("c", categories[i], is_c_file, test_c_example);
  }
}
