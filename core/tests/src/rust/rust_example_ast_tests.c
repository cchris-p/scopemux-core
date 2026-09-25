/**
 * @file rust_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for Rust
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/rust directory, load Rust source files, extract their
 * ASTs, and validate them against corresponding .expected.json files.
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
 * Run a test for a specific Rust example file
 *
 * @param category Category subdirectory under core/tests/examples/rust
 * @param filename Source filename within the category
 */
static void test_rust_example(const char *category, const char *filename) {
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
    paths = construct_test_paths("rust", category, filename);
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
  config.language = LANG_RUST;
  config.debug_mode = true;

  bool test_passed = run_ast_test(&config);

  free(paths.base_filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Extract category and filename from a full example path
 *
 * Example: "core/tests/examples/rust/basic_syntax/language_features.rs" ->
 * category="basic_syntax", filename="language_features.rs"
 *
 * @param test_file_path Full or relative path to the example source
 * @param category Output category (caller frees)
 * @param filename Output filename (caller frees)
 * @return true on success, false otherwise
 */
static bool extract_test_info(const char *test_file_path, char **category, char **filename) {
  if (!test_file_path) {
    return false;
  }

  const char *pattern = "core/tests/examples/rust/";
  const char *start = strstr(test_file_path, pattern);
  if (!start) {
    return false;
  }

  start += strlen(pattern);

  const char *slash = strchr(start, '/');
  if (!slash) {
    return false;
  }

  size_t category_len = (size_t)(slash - start);
  *category = malloc(category_len + 1);
  if (!*category) {
    return false;
  }
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
 * Test that processes all Rust example files or a specific file based on the
 * SCOPEMUX_TEST_FILE environment variable.
 */
Test(rust_examples, all_examples) {
  const char *test_file_env = getenv("SCOPEMUX_TEST_FILE");

  if (test_file_env) {
    char *category = NULL;
    char *filename = NULL;

    if (extract_test_info(test_file_env, &category, &filename)) {
      test_rust_example(category, filename);
      free(category);
      free(filename);
    } else {
      cr_assert_fail("Failed to parse SCOPEMUX_TEST_FILE: %s", test_file_env);
    }
  } else {
    process_language_example_categories("rust", test_rust_example);
  }
}
