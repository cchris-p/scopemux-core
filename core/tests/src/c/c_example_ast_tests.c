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
  const char *env_source_file = getenv("SCOPEMUX_TEST_FILE");
  const char *env_json_file = getenv("SCOPEMUX_EXPECTED_JSON");

  TestPaths paths = {0};
  const char *source_file_path;
  const char *json_file_path;

  if (env_source_file && env_json_file) {
    source_file_path = env_source_file;
    json_file_path = env_json_file;

    const char *base_start = strrchr(filename, '/');
    if (base_start) {
      base_start++;
    } else {
      base_start = filename;
    }

    paths.base_filename = strdup(base_start);
    if (paths.base_filename) {
      char *dot = strrchr(paths.base_filename, '.');
      if (dot) {
        *dot = '\0';
      }
    }
  } else {
    paths = construct_test_paths("c", category, filename);
    if (!paths.base_filename) {
      cr_log_error("Failed to construct test paths");
      cr_assert_fail("Memory allocation failed");
    }
    source_file_path = paths.source_path;
    json_file_path = paths.json_path;
  }

  ASTTestConfig config = ast_test_config_init();
  config.source_file = source_file_path;
  config.json_file = json_file_path;
  config.category = category;
  config.base_filename = paths.base_filename;
  config.language = LANG_C;
  config.debug_mode = true;

  bool test_passed = run_ast_test(&config);

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

  const char *pattern = "core/tests/examples/c/";
  const char *start = strstr(test_file_path, pattern);
  if (!start)
    return false;

  start += strlen(pattern);

  const char *slash = strchr(start, '/');
  if (!slash)
    return false;

  size_t category_len = slash - start;
  *category = malloc(category_len + 1);
  if (!*category)
    return false;
  strncpy(*category, start, category_len);
  (*category)[category_len] = '\0';

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
    char *category = NULL;
    char *filename = NULL;

    if (extract_test_info(test_file_env, &category, &filename)) {
      test_c_example(category, filename);
      free(category);
      free(filename);
    } else {
      cr_assert_fail("Failed to parse SCOPEMUX_TEST_FILE: %s", test_file_env);
    }
  } else {
    process_language_example_categories("c", test_c_example);
  }
}
