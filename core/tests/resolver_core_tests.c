/**
 * @file resolver_core_tests.c
 * @brief Main test runner for resolver core functionality tests
 */

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>

// Include our resolver core tests
#include "reference_resolvers/resolver_core_tests.c"

int main(int argc, char *argv[]) {
  // Initialize Criterion test framework
  struct criterion_test_set *tests = criterion_initialize();

  // Run tests
  int result = criterion_run_all_tests(tests);

  // Clean up
  criterion_finalize(tests);

  return result;
}
