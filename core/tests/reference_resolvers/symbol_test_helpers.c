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
  printf("[DEBUG] Entering symbol_new with name=%s\n", name);
  if (!name) {
    printf("[DEBUG] symbol_new: name is NULL\n");
    return NULL;
  }

  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  printf("[DEBUG] symbol_new: malloc returned %p\n", (void *)symbol);
  if (!symbol) {
    printf("[DEBUG] symbol_new: malloc failed\n");
    return NULL;
  }

  // Initialize all fields to zero/NULL
  memset(symbol, 0, sizeof(Symbol));
  printf("[DEBUG] symbol_new: memset complete\n");

  // Set provided values
  symbol->name = strdup(name);
  printf("[DEBUG] symbol_new: strdup returned %p\n", (void *)symbol->name);
  if (!symbol->name) {
    printf("[DEBUG] symbol_new: strdup failed\n");
    free(symbol);
    return NULL;
  }

  symbol->type = type;
  printf("[DEBUG] symbol_new: returning symbol %p\n", (void *)symbol);
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
