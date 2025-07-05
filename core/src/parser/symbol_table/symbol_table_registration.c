/**
 * @file symbol_table_registration.c
 * @brief Symbol registration and hash utility functions
 *
 * Implements functionality for registering symbols in the global table
 * and utility functions for hash calculations and symbol processing.
 */

#include "scopemux/ast.h" // For NODE_FILE, NODE_MODULE, ASTNode, etc.
#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include "symbol_table_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Prime number for hash calculation
#define HASH_PRIME 31

/**
 * Hash function for strings
 *
 * Computes a hash value for a string to be used in the hash table.
 * Uses a simple multiplicative hash algorithm with a prime factor.
 *
 * @param str String to hash
 * @param table_size Size of the hash table for modulo operation
 * @return Hash value in the range [0, table_size-1]
 */
uint32_t hash_string(const char *str, size_t table_size) {
  uint32_t hash = 0;

  while (*str) {
    hash = (hash * HASH_PRIME) + (*str++);
  }

  return hash % table_size;
}

/**
 * Register a symbol in the global table
 *
 * Creates a new symbol entry and adds it to the global symbol table.
 * If a symbol with the same name already exists, it will be tracked as a collision.
 * In a real use case, we might want to handle declarations vs. definitions differently.
 *
 * @param table Symbol table
 * @param qualified_name Fully qualified name (will be copied)
 * @param node AST node (not copied, ownership remains with caller)
 * @param file_path File path (will be copied)
 * @param scope Symbol visibility scope
 * @param language Language of the source file
 * @return The created entry or NULL on failure
 */
SymbolEntry *symbol_table_register_impl(GlobalSymbolTable *table, const char *qualified_name,
                                        ASTNode *node, const char *file_path, SymbolScope scope,
                                        Language language) {
  if (!table || !qualified_name || !node || !file_path) {
    return NULL;
  }

  // Check if the symbol already exists
  SymbolEntry *existing = symbol_table_lookup_impl(table, qualified_name);
  if (existing) {
    // Symbol already exists - this could be a declaration/definition pair
    // TODO: Implement logic to handle multiple entries for the same symbol
    // For now, track the collision and continue with registration
    table->collisions++;

    log_debug("Symbol collision detected: %s (existing in %s, new in %s)", qualified_name,
              existing->file_path, file_path);
  }

  // Create a new symbol entry
  SymbolEntry *entry =
      symbol_table_entry_create_impl(qualified_name, node, file_path, scope, language);
  if (!entry) {
    log_error("Failed to create symbol entry for %s", qualified_name);
    return NULL;
  }

  // Add to hash table
  uint32_t hash = hash_string(qualified_name, table->num_buckets);
  entry->next = table->buckets[hash];
  table->buckets[hash] = entry;
  table->num_symbols++;
  table->count++; // Increment count for test compatibility

  log_debug("Registered symbol: %s (%s)", qualified_name, file_path);

  // Check if rehashing is needed
  if (symbol_table_should_rehash(table)) {
    log_debug("Rehashing symbol table due to high load factor");
    symbol_table_rehash(table, table->num_buckets * 2);
  }

  return entry;
}

/**
 * Add a pre-created symbol entry to the symbol table (implementation)
 *
 * This function adds a pre-created SymbolEntry to the global symbol table.
 * The entry must have been created with symbol_entry_create or equivalent.
 * The function handles hash calculation and collision tracking.
 *
 * @param table Symbol table to add the entry to
 * @param entry Pre-created symbol entry (ownership transferred to table)
 * @return true on success, false on failure
 */
bool symbol_table_add_impl(GlobalSymbolTable *table, SymbolEntry *entry) {
  if (!table || !entry || !entry->qualified_name) {
    return false;
  }

  // Check if the symbol already exists
  SymbolEntry *existing = symbol_table_lookup_impl(table, entry->qualified_name);
  if (existing) {
    // Symbol already exists - this could be a declaration/definition pair
    // For now, track the collision and continue with registration
    table->collisions++;

    log_debug("Symbol collision detected during add: %s (existing in %s, new in %s)",
              entry->qualified_name, existing->file_path, entry->file_path);
  }

  // Add to hash table
  uint32_t hash = hash_string(entry->qualified_name, table->num_buckets);
  entry->next = table->buckets[hash];
  table->buckets[hash] = entry;
  table->num_symbols++;

  log_debug("Added symbol: %s (%s)", entry->qualified_name, entry->file_path);

  // Check if rehashing is needed
  if (symbol_table_should_rehash(table)) {
    log_debug("Rehashing symbol table due to high load factor");
    symbol_table_rehash(table, table->num_buckets * 2);
  }

  return true;
}

/**
 * Register symbols from an AST node recursively
 *
 * Walks through an AST and registers all relevant symbols in the global table.
 * This function intelligently extracts qualified names based on node type and
 * language context.
 *
 * @param table Symbol table
 * @param node Root AST node to process
 * @param current_scope Current namespace/scope for qualification
 * @param file_path Source file path
 * @param language Language of the source file
 * @return Number of symbols registered
 */
size_t symbol_table_register_from_ast_impl(GlobalSymbolTable *table, ASTNode *node,
                                           const char *current_scope, const char *file_path,
                                           Language language) {
  if (!table || !node || !file_path) {
    return 0;
  }

  size_t count = 0;
  char qualified_name[256];
  bool register_this_node = false;
  SymbolScope scope = SCOPE_UNKNOWN;

  // Determine if this node should be registered based on type
  switch (node->type) {
  case NODE_FUNCTION:
    register_this_node = true;
    scope = SCOPE_FILE; // Default scope, may be overridden below
    break;

  case NODE_CLASS:
  case NODE_INTERFACE:
  case NODE_ENUM:
  case NODE_STRUCT:
  case NODE_TYPEDEF:
    register_this_node = true;
    scope = SCOPE_FILE;
    break;

  case NODE_METHOD:
    register_this_node = true;
    scope = SCOPE_FILE;
    break;

  case NODE_VARIABLE:
    // Only register global/file-level variables, not locals
    if (node->parent && (node->parent->type == NODE_ROOT || node->parent->type == NODE_MODULE)) {
      register_this_node = true;
      scope = SCOPE_FILE;
    }
    break;

  case NODE_MODULE:
  case NODE_NAMESPACE:
    register_this_node = true;
    scope = SCOPE_GLOBAL;
    break;

  default:
    // Don't register other node types
    break;
  }

  // Check visibility modifiers to determine scope
  // NOTE: ASTNode no longer has an 'info' member. To support semantic scope detection (e.g.,
  // static/private/public/export), update this logic to use the correct member when/if available.
  // For now, skip this check.
  // TODO: Extract semantic modifiers from ASTNode attributes or a future semantic field if/when
  // added.

  // Register the node if applicable
  if (register_this_node && node->name) {
    // Build qualified name
    if (current_scope && strlen(current_scope) > 0) {
      // Use the appropriate separator based on language
      char separator[3] = ".";
      if (language == LANG_C || language == LANG_CPP) {
        strcpy(separator, "::");
      }

      snprintf(qualified_name, sizeof(qualified_name), "%s%s%s", current_scope, separator,
               node->name);
    } else {
      strncpy(qualified_name, node->name, sizeof(qualified_name) - 1);
      qualified_name[sizeof(qualified_name) - 1] = '\0';
    }

    // Register the symbol
    SymbolEntry *entry =
        symbol_table_register_impl(table, qualified_name, node, file_path, scope, language);
    if (entry) {
      count++;

      log_debug("Registered %s symbol: %s", entry->scope == SCOPE_GLOBAL ? "global" : "file-level",
                qualified_name);
    }

    // For scoped nodes, update the current scope for children
    if (node->type == NODE_CLASS || node->type == NODE_NAMESPACE || node->type == NODE_STRUCT ||
        node->type == NODE_INTERFACE) {
      current_scope = qualified_name;
    }
  }

  // Recursively process children
  for (size_t i = 0; i < node->num_children; i++) {
    count += symbol_table_register_from_ast_impl(table, node->children[i], current_scope, file_path,
                                                 language);
  }

  return count;
}

// NOTE: The public API function symbol_table_add is defined in symbol_table.c
// and delegates to symbol_table_add_impl defined in this file.
