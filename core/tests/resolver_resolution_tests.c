/**
 * @file resolver_resolution_tests.c
 * @brief Main test runner for resolver resolution functionality tests
 */

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>

// Include the resolver resolution tests
#include "reference_resolvers/resolver_resolution_tests.c"

int main(int argc, char *argv[]) {
  // Initialize Criterion test framework
  struct criterion_test_set *tests = criterion_initialize();

  // Run tests
  int result = criterion_run_all_tests(tests);

  // Clean up
  criterion_finalize(tests);

  return result;
}
