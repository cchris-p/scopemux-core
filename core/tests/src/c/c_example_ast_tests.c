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
  // CRITICAL FIX: Use environment variables when available (for test runner)
  // Otherwise fall back to constructed paths (for manual testing)
  const char *env_source_file = getenv("SCOPEMUX_TEST_FILE");
  const char *env_json_file = getenv("SCOPEMUX_EXPECTED_JSON");
  
  TestPaths paths = {0};
  const char *source_file_path;
  const char *json_file_path;
  
  if (env_source_file && env_json_file) {
    // Use environment variables (test runner mode)
    cr_log_info("DETECTED ENVIRONMENT VARIABLES!");
    cr_log_info("Using environment paths: source=%s, json=%s", env_source_file, env_json_file);
    source_file_path = env_source_file;
    json_file_path = env_json_file;
    
    // Extract base filename for config
    const char *base_start = strrchr(filename, '/');
    if (base_start) {
      base_start++; // Skip the '/'
    } else {
      base_start = filename;
    }
    
    paths.base_filename = strdup(base_start);
    if (paths.base_filename) {
      char *dot = strrchr(paths.base_filename, '.');
      if (dot) {
        *dot = '\0'; // Remove extension
      }
    }
  } else {
    // Use constructed paths (manual testing mode)
    cr_log_info("Using constructed paths for manual testing");
    paths = construct_test_paths("c", category, filename);
    if (!paths.base_filename) {
      cr_log_error("Failed to construct test paths");
      cr_assert_fail("Memory allocation failed");
    }
    source_file_path = paths.source_path;
    json_file_path = paths.json_path;
  }

  // Initialize test configuration
  ASTTestConfig config = ast_test_config_init();
  config.source_file = source_file_path;
  config.json_file = json_file_path;
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
 * Extract category and filename from a full test file path
 * Example: "core/tests/examples/c/basic_syntax/hello_world.c" -> category="basic_syntax",
 * filename="hello_world.c"
 */
static bool extract_test_info(const char *test_file_path, char **category, char **filename) {
  if (!test_file_path)
    return false;

  // Look for the pattern: core/tests/examples/c/{category}/{filename}
  // Also handle build directory pattern: core/tests/core/tests/examples/c/{category}/{filename}
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
    process_language_example_categories("c", test_c_example);
  }
}
