/**
 * @file js_example_ast_tests.c
 * @brief Tests for validating AST extraction against expected JSON output for JavaScript language
 *
 * This file contains tests that iterate through each subdirectory of the
 * core/tests/examples/js directory, load JavaScript source files, extract their ASTs,
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
 * Run a test for a specific JavaScript example file
 */
static void test_js_example(const char *category, const char *filename) {
  bool test_passed = run_language_example_test("js", LANG_JAVASCRIPT, category, filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Test that processes all JavaScript example files
 */
Test(js_examples, all_examples) {
  process_language_example_categories("js", test_js_example);
}
