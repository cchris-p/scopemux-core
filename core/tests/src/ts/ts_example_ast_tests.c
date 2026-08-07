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
 * Run a test for a specific TypeScript example file
 */
static void test_ts_example(const char *category, const char *filename) {
  bool test_passed = run_language_example_test("ts", LANG_TYPESCRIPT, category, filename);

  cr_assert(test_passed, "AST test failed for %s/%s", category, filename);
}

/**
 * Test that processes all TypeScript example files
 */
Test(ts_examples, all_examples) {
  process_language_example_categories("ts", test_ts_example);
}
