/**
 * @file symbol_table_tests.c
 * @brief Main test runner for symbol table functionality tests
 *
 * These tests verify that the symbol table module correctly handles symbols
 * across files and maintains proper delegation to implementation modules.
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
  // Create a test AST node
  ASTNode *node = ast_node_new(NODE_FUNCTION, "test_symbol");
  cr_assert(node != NULL, "Failed to create AST node for test");

  // Register symbol in symbol table
  SymbolEntry *entry =
      symbol_table_register(symbol_table, "test_symbol", node, "test_file.c", SCOPE_GLOBAL, LANG_C);
  cr_assert(entry != NULL, "Symbol should be successfully registered");
  cr_assert(symbol_table->count == 1, "Symbol table count should be incremented");

  // Look up the symbol
  SymbolEntry *found = symbol_table_lookup(symbol_table, "test_symbol");
  cr_assert(found != NULL, "Symbol should be found");
  cr_assert_str_eq(found->qualified_name, "test_symbol", "Symbol qualified name should match");
  cr_assert_str_eq(found->file_path, "test_file.c", "Symbol file path should match");
  cr_assert(found->node == node, "Symbol node should match");
  cr_assert(found->scope == SCOPE_GLOBAL, "Symbol scope should match");
  cr_assert(found->language == LANG_C, "Symbol language should match");

  // Clean up
  ast_node_free(node);
}

// Test interfile symbol lookup
Test(symbol_table_delegation, interfile_lookup, .init = setup_symbol_table,
     .fini = teardown_symbol_table) {
  // Create test AST nodes
  ASTNode *node1 = ast_node_new(NODE_FUNCTION, "file1_symbol");
  ASTNode *node2 = ast_node_new(NODE_FUNCTION, "file2_symbol");
  cr_assert(node1 != NULL, "Failed to create first AST node");
  cr_assert(node2 != NULL, "Failed to create second AST node");

  // Register symbols from different files
  SymbolEntry *entry1 =
      symbol_table_register(symbol_table, "file1_symbol", node1, "file1.c", SCOPE_GLOBAL, LANG_C);
  SymbolEntry *entry2 =
      symbol_table_register(symbol_table, "file2_symbol", node2, "file2.c", SCOPE_GLOBAL, LANG_C);

  cr_assert(entry1 != NULL, "First symbol should be registered");
  cr_assert(entry2 != NULL, "Second symbol should be registered");

  // Look up both symbols
  SymbolEntry *found1 = symbol_table_lookup(symbol_table, "file1_symbol");
  SymbolEntry *found2 = symbol_table_lookup(symbol_table, "file2_symbol");

  cr_assert(found1 != NULL, "Should find symbol from file1");
  cr_assert(found2 != NULL, "Should find symbol from file2");
  cr_assert_str_eq(found1->file_path, "file1.c", "File path for first symbol should match");
  cr_assert_str_eq(found2->file_path, "file2.c", "File path for second symbol should match");

  // Clean up
  ast_node_free(node1);
  ast_node_free(node2);
}
