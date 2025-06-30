/**
 * @file symbol_core.c
 * @brief Core infrastructure for the global symbol table
 *
 * Implements the foundational operations for symbol table management:
 * - Initialization and cleanup
 * - Statistics tracking
 * - Configuration and operational parameters
 * - Memory management for symbol table structures
 */

#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of functions defined in other component files
extern uint32_t hash_string(const char *str, size_t table_size);
extern SymbolEntry *symbol_entry_create(const char *qualified_name, ASTNode *node,
                                        const char *file_path, SymbolScope scope,
                                        Language language);
extern void symbol_entry_free(SymbolEntry *entry);

// Hash table load factor threshold for rehashing
#define LOAD_FACTOR_THRESHOLD 0.75

/**
 * Create a new global symbol table
 *
 * Initializes a new GlobalSymbolTable with the specified initial capacity.
 * The function ensures a minimum reasonable capacity and allocates all
 * necessary internal data structures.
 *
 * @param initial_capacity Initial number of hash buckets
 * @return A newly created symbol table, or NULL if allocation failed
 */
GlobalSymbolTable *symbol_table_create(size_t initial_capacity) {
  // Ensure initial capacity is reasonable
  if (initial_capacity < 8) {
    initial_capacity = 8;
  }

  GlobalSymbolTable *table = (GlobalSymbolTable *)malloc(sizeof(GlobalSymbolTable));
  if (!table) {
    return NULL;
  }

  // Initialize fields
  memset(table, 0, sizeof(GlobalSymbolTable));
  table->num_buckets = initial_capacity;

  // Allocate hash buckets
  table->buckets = (SymbolEntry **)calloc(initial_capacity, sizeof(SymbolEntry *));
  if (!table->buckets) {
    free(table);
    return NULL;
  }

  // Allocate initial scope prefixes
  table->scope_prefixes = (char **)malloc(8 * sizeof(char *));
  if (!table->scope_prefixes) {
    free(table->buckets);
    free(table);
    return NULL;
  }

  return table;
}

/**
 * Free all resources associated with a symbol table
 *
 * Releases all memory allocated for the symbol table, including
 * all entries, scope prefixes, and other internal data structures.
 * Note that this does not free the ASTNodes referenced by the entries.
 *
 * @param table Symbol table to free
 */
void symbol_table_free(GlobalSymbolTable *table) {
  if (!table) {
    return;
  }

  // Free all entries in the buckets
  for (size_t i = 0; i < table->num_buckets; i++) {
    SymbolEntry *entry = table->buckets[i];
    while (entry) {
      SymbolEntry *next = entry->next;
      symbol_entry_free(entry);
      entry = next;
    }
  }

  // Free scope prefixes
  for (size_t i = 0; i < table->num_scopes; i++) {
    free(table->scope_prefixes[i]);
  }

  // Free arrays and table
  free(table->scope_prefixes);
  free(table->buckets);
  free(table);
}

/**
 * Get statistics about the symbol table
 *
 * Retrieves various metrics about the symbol table's current state,
 * including its capacity, number of symbols, and collision statistics.
 *
 * @param table Symbol table
 * @param out_capacity Output parameter for table capacity (can be NULL)
 * @param out_size Output parameter for number of symbols (can be NULL)
 * @param out_collisions Output parameter for number of collisions (can be NULL)
 */
void symbol_table_get_stats(const GlobalSymbolTable *table, size_t *out_capacity, size_t *out_size,
                            size_t *out_collisions) {
  if (!table) {
    if (out_capacity)
      *out_capacity = 0;
    if (out_size)
      *out_size = 0;
    if (out_collisions)
      *out_collisions = 0;
    return;
  }

  if (out_capacity)
    *out_capacity = table->num_buckets;
  if (out_size)
    *out_size = table->num_symbols;
  if (out_collisions)
    *out_collisions = table->collisions;
}

/**
 * Analyze the symbol table for optimization opportunities
 *
 * Determines whether the symbol table should be rehashed based on
 * its current load factor. Rehashing is recommended when the load factor
 * exceeds LOAD_FACTOR_THRESHOLD.
 *
 * @param table Symbol table
 * @return true if rehashing is recommended, false otherwise
 */
bool symbol_table_should_rehash(const GlobalSymbolTable *table) {
  if (!table || table->num_buckets == 0) {
    return false;
  }

  // Calculate the load factor (ratio of items to buckets)
  double load_factor = (double)table->num_symbols / table->num_buckets;

  return load_factor > LOAD_FACTOR_THRESHOLD;
}

/**
 * Rehash the symbol table with a new capacity
 *
 * Rebuilds the symbol table with a new number of buckets to improve
 * lookup performance. This operation reallocates the buckets array
 * and reinserts all entries into their new hash positions.
 *
 * @param table Symbol table
 * @param new_capacity New capacity (number of buckets)
 * @return true on success, false on failure
 */
bool symbol_table_rehash(GlobalSymbolTable *table, size_t new_capacity) {
  if (!table || new_capacity <= table->num_symbols) {
    return false;
  }

  // Create a new buckets array
  SymbolEntry **new_buckets = (SymbolEntry **)calloc(new_capacity, sizeof(SymbolEntry *));
  if (!new_buckets) {
    return false;
  }

  // Rehash all entries
  for (size_t i = 0; i < table->num_buckets; i++) {
    SymbolEntry *entry = table->buckets[i];
    while (entry) {
      // Get the next entry before changing the current one
      SymbolEntry *next = entry->next;

      // Calculate new hash
      uint32_t new_hash = hash_string(entry->qualified_name, new_capacity);

      // Insert at the beginning of the new bucket
      entry->next = new_buckets[new_hash];
      new_buckets[new_hash] = entry;

      // Move to the next entry
      entry = next;
    }
  }

  // Free the old buckets array and update table
  free(table->buckets);
  table->buckets = new_buckets;
  table->num_buckets = new_capacity;
  table->collisions = 0; // Reset collision counter

  return true;
}

/**
 * Add a scope prefix for resolution
 *
 * Registers a scope prefix (e.g., namespace name) to be used during
 * scope-aware symbol resolution. This allows resolving unqualified names
 * in the context of common namespaces or modules.
 *
 * @param table Symbol table
 * @param scope_prefix Scope prefix to add (e.g., "std", "namespace1")
 * @return true on success, false on failure
 */
bool symbol_table_add_scope(GlobalSymbolTable *table, const char *scope_prefix) {
  if (!table || !scope_prefix) {
    return false;
  }

  // Check if the scope is already registered
  for (size_t i = 0; i < table->num_scopes; i++) {
    if (strcmp(table->scope_prefixes[i], scope_prefix) == 0) {
      return true; // Already registered
    }
  }

  // Check if we need to resize the scope prefixes array
  if (table->num_scopes % 8 == 0) {
    size_t new_capacity = table->num_scopes + 8;
    char **new_scopes = (char **)realloc(table->scope_prefixes, new_capacity * sizeof(char *));
    if (!new_scopes) {
      return false;
    }
    table->scope_prefixes = new_scopes;
  }

  // Add the new scope prefix
  table->scope_prefixes[table->num_scopes] = strdup(scope_prefix);
  if (!table->scope_prefixes[table->num_scopes]) {
    return false;
  }

  table->num_scopes++;
  return true;
}
