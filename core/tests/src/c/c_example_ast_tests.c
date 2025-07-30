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
static bool is_c_file(const char *filename) {
  // Check if file ends with .c exactly (not .c.something)
  size_t len = strlen(filename);
  if (len < 3)
    return false; // Must be at least "x.c"

  // Check if it ends with ".c" and that's the end of the filename
  return strcmp(filename + len - 2, ".c") == 0;
}

/**
 * Extract category and filename from a full test file path
 * Example: "core/tests/examples/c/basic_syntax/hello_world.c" -> category="basic_syntax",
 * filename="hello_world.c"
 */
static bool extract_test_info(const char *test_file_path, char **category, char **filename) {
  if (!test_file_path)
    return false;

  // Look for the pattern: core/tests/examples/c/{category}/{filename}
  const char *pattern = "core/tests/examples/c/";
  const char *start = strstr(test_file_path, pattern);
  if (!start)
    return false;

  start += strlen(pattern);

  // Find the next slash to separate category from filename
  const char *slash = strchr(start, '/');
  if (!slash)
    return false;

  // Extract category
  size_t category_len = slash - start;
  *category = malloc(category_len + 1);
  if (!*category)
    return false;
  strncpy(*category, start, category_len);
  (*category)[category_len] = '\0';

  // Extract filename (skip the slash)
  const char *filename_start = slash + 1;
  *filename = strdup(filename_start);
  if (!*filename) {
    free(*category);
    *category = NULL;
    return false;
  }

  return true;
}

/**
 * Test that processes all C example files or a specific file based on environment variable
 */
Test(c_examples, all_examples) {
  const char *test_file_env = getenv("SCOPEMUX_TEST_FILE");

  if (test_file_env) {
    // Run specific test file only
    char *category = NULL;
    char *filename = NULL;

    if (extract_test_info(test_file_env, &category, &filename)) {
      cr_log_info("Running single test: %s/%s (from SCOPEMUX_TEST_FILE=%s)", category, filename,
                  test_file_env);
      test_c_example(category, filename);
      free(category);
      free(filename);
    } else {
      cr_assert_fail("Failed to parse SCOPEMUX_TEST_FILE: %s", test_file_env);
    }
  } else {
    // Run all tests (original behavior)
    cr_log_info("Running all C example tests (no SCOPEMUX_TEST_FILE set)");
    const char *categories[] = {"basic_syntax",      "complex_structures", "file_io",
                                "memory_management", "struct_union_enum",  NULL};

    for (int i = 0; categories[i] != NULL; i++) {
      process_category_files("c", categories[i], is_c_file, test_c_example);
    }
  }
}
