/**
 * @file cpp_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for C++ language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/cpp directory, load C++ source files, extract their ASTs,
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
 */
static void test_cpp_example(const char *category, const char *filename) {
  // Get test paths
  TestPaths paths = construct_test_paths("cpp", category, filename);
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
  config.language = LANG_CPP;
  config.debug_mode = true;

  // Run the test
  bool test_passed = run_ast_test(&config);

  // Cleanup
  free(paths.base_filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Test that processes all C++ example files
 */
Test(cpp_examples, all_examples) {
  const char *categories[] = {"basic_syntax", "templates",  "classes", "namespaces",
                              "stl",          "modern_cpp", NULL};

  for (int i = 0; categories[i] != NULL; i++) {
    process_category_files("cpp", categories[i], is_cpp_source_file, test_cpp_example);
  }
}
