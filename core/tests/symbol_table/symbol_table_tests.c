/**
 * @file symbol_table_tests.c
 * @brief Unit tests for the symbol table functionality (template)
 */

#include "scopemux/ast.h"
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

// Example test case
Test(symbol_table, create_and_destroy, .init = setup_symbol_table, .fini = teardown_symbol_table) {
  cr_assert(symbol_table != NULL, "Symbol table should be non-NULL after creation");
  cr_assert(symbol_table->capacity == 32, "Symbol table should have requested capacity");
  cr_assert(symbol_table->count == 0, "Symbol table should start with 0 symbols");
}