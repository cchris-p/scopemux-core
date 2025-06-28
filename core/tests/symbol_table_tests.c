/**
 * @file symbol_table_tests.c
 * @brief Main test runner for symbol table functionality tests
 *
 * These tests verify that the symbol table module correctly handles symbols
 * across files and maintains proper delegation to implementation modules.
 */

#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>

// Test fixture setup
static GlobalSymbolTable *symbol_table = NULL;

void setup_symbol_table() {
  symbol_table = symbol_table_create(32);
  cr_assert(symbol_table != NULL, "Failed to create symbol table for tests");
}

void teardown_symbol_table() {
  if (symbol_table) {
    symbol_table_free(symbol_table);
    symbol_table = NULL;
  }
}

// Test creation and basic properties
Test(symbol_table_delegation, create_delegate, .init = setup_symbol_table,
     .fini = teardown_symbol_table) {
  cr_assert(symbol_table != NULL, "Symbol table should be non-NULL");
  cr_assert(symbol_table->capacity == 32, "Symbol table should have requested capacity");
  cr_assert(symbol_table->count == 0, "Symbol table should start with 0 symbols");
}

// Test symbol addition and lookup
Test(symbol_table_delegation, add_lookup_delegate, .init = setup_symbol_table,
     .fini = teardown_symbol_table) {
  // Create a test symbol
  Symbol *sym = symbol_new("test_symbol", SYMBOL_FUNCTION);
  sym->file_path = strdup("test_file.c");
  sym->line = 42;
  sym->column = 10;
  sym->language = LANG_C;

  // Add to symbol table
  bool added = symbol_table_add(symbol_table, sym);
  cr_assert(added, "Symbol should be successfully added");
  cr_assert(symbol_table->count == 1, "Symbol table count should be incremented");

  // Look up the symbol
  Symbol *found = symbol_table_lookup(symbol_table, "test_symbol");
  cr_assert(found != NULL, "Symbol should be found");
  cr_assert_str_eq(found->name, "test_symbol", "Symbol name should match");
  cr_assert_str_eq(found->file_path, "test_file.c", "Symbol file path should match");
  cr_assert(found->line == 42, "Symbol line should match");
}

// Test interfile symbol lookup
Test(symbol_table_delegation, interfile_lookup, .init = setup_symbol_table,
     .fini = teardown_symbol_table) {
  // Add symbols from different files
  Symbol *sym1 = symbol_new("file1_symbol", SYMBOL_FUNCTION);
  sym1->file_path = strdup("file1.c");
  sym1->line = 10;
  sym1->language = LANG_C;

  Symbol *sym2 = symbol_new("file2_symbol", SYMBOL_FUNCTION);
  sym2->file_path = strdup("file2.c");
  sym2->line = 20;
  sym2->language = LANG_C;

  symbol_table_add(symbol_table, sym1);
  symbol_table_add(symbol_table, sym2);

  // Look up both symbols
  Symbol *found1 = symbol_table_lookup(symbol_table, "file1_symbol");
  Symbol *found2 = symbol_table_lookup(symbol_table, "file2_symbol");

  cr_assert(found1 != NULL, "Should find symbol from file1");
  cr_assert(found2 != NULL, "Should find symbol from file2");
  cr_assert_str_eq(found1->file_path, "file1.c", "File path for first symbol should match");
  cr_assert_str_eq(found2->file_path, "file2.c", "File path for second symbol should match");
}

// Test main entry point
int main(int argc, char *argv[]) {
  // Initialize Criterion test framework
  struct criterion_test_set *tests = criterion_initialize();

  // Run tests
  int result = criterion_run_all_tests(tests);

  // Clean up
  criterion_finalize(tests);

  return result;
}
