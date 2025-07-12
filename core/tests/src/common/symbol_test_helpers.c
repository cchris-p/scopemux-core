#include "../../include/symbol_test_helpers.h"
#include "scopemux/logging.h"
#include <string.h>

TestSymbol *test_symbol_new(const char *name, SymbolType type) {
  TestSymbol *symbol = malloc(sizeof(TestSymbol));
  if (!symbol) {
    return NULL;
  }

  symbol->name = strdup(name);
  if (!symbol->name) {
    free(symbol);
    return NULL;
  }

  symbol->qualified_name = NULL;
  symbol->type = type;
  symbol->file_path = NULL;
  symbol->line = 0;
  symbol->column = 0;
  symbol->language = LANG_UNKNOWN;
  symbol->data = NULL;

  return symbol;
}

void test_symbol_free(TestSymbol *symbol) {
  if (!symbol) {
    return;
  }

  free(symbol->name);
  free(symbol->qualified_name);
  free(symbol->file_path);
  free(symbol->data);
  free(symbol);
}

bool symbol_table_add(GlobalSymbolTable *table, SymbolEntry *entry) {
  if (!table || !entry) {
    return false;
  }

  // Hash the qualified name to get the bucket index
  unsigned long index = hash_qualified_name(entry->qualified_name, table->num_buckets);

  // Create a new entry
  SymbolEntry *new_entry = malloc(sizeof(SymbolEntry));
  if (!new_entry) {
    return false;
  }

  // Copy the entry data
  memcpy(new_entry, entry, sizeof(SymbolEntry));

  // Add to the bucket
  new_entry->next = table->buckets[index];
  table->buckets[index] = new_entry;
  table->num_symbols++;

  return true;
}