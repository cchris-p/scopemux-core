/**
 * @file symbol_test_helpers.c
 * @brief Helper functions for symbol-related testing
 *
 * This file contains implementations of helper functions used in tests
 * to create and manipulate Symbol structures.
 */

#include "reference_resolvers/reference_resolver_private.h"
#include <stdlib.h>
#include <string.h>

/**
 * Create a new Symbol with the given name and type
 *
 * @param name The name of the symbol
 * @param type The symbol type
 * @return A newly allocated Symbol structure or NULL on failure
 */
Symbol *symbol_new(const char *name, SymbolType type) {
  if (!name) {
    return NULL;
  }

  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  if (!symbol) {
    return NULL;
  }

  // Initialize all fields to zero/NULL
  memset(symbol, 0, sizeof(Symbol));

  // Set provided values
  symbol->name = strdup(name);
  if (!symbol->name) {
    free(symbol);
    return NULL;
  }

  symbol->type = type;

  return symbol;
}

/**
 * Free all resources associated with a Symbol
 *
 * @param symbol The symbol to free
 */
void symbol_free(Symbol *symbol) {
  if (!symbol) {
    return;
  }

  free(symbol->name);
  free(symbol->qualified_name);
  free(symbol->file_path);
  free(symbol);
}
