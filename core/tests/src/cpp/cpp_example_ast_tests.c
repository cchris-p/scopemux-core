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
 * Run a test for a specific C++ example file
 */
static void test_cpp_example(const char *category, const char *filename) {
  bool test_passed = run_language_example_test("cpp", LANG_CPP, category, filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Test that processes all C++ example files
 */
Test(cpp_examples, all_examples) {
  process_language_example_categories("cpp", test_cpp_example);
}
