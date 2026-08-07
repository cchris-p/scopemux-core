/**
 * @file symbol_lookup.c
 * @brief Symbol lookup and resolution functionality
 *
 * Implements various lookup strategies for finding symbols in the global
 * symbol table, including direct lookup by name, scope-aware resolution,
 * and filtered queries.
 */

#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External functions from other files
extern uint32_t hash_string(const char *str, size_t table_size);

/**
 * Look up a symbol by its fully qualified name
 *
 * Performs a direct lookup in the symbol table using the hash
 * of the qualified name.
 *
 * @param table Symbol table
 * @param qualified_name Fully qualified name to look up
 * @return Matching symbol entry or NULL if not found
 */
SymbolEntry *symbol_table_lookup_impl(const GlobalSymbolTable *table, const char *qualified_name) {
  if (!table || !qualified_name) {
    return NULL;
  }

  // Calculate hash and find bucket
  uint32_t hash = hash_string(qualified_name, table->num_buckets);

  // Search the bucket for a matching entry
  SymbolEntry *entry = table->buckets[hash];
  while (entry) {
    if (strcmp(entry->qualified_name, qualified_name) == 0) {
      return entry;
    }
    entry = entry->next;
  }

  return NULL; // Not found
}

/**
 * Look up a symbol using scope-aware resolution
 *
 * This function attempts to resolve a possibly unqualified or partially qualified
 * name by searching through the current scope chain.
 *
 * The resolution algorithm depends on the language:
 * - For C/C++, it tries: current_scope::name, then global::name
 * - For Python, it tries: current_scope.name, then builtins.name
 * - For JavaScript/TypeScript, it tries: current_scope.name, then global.name
 *
 * @param table Symbol table
 * @param name Symbol name (may be partially qualified)
 * @param current_scope Current scope for resolution (e.g., namespace::class)
 * @param language Language context for resolution rules
 * @return Matching symbol entry or NULL if not found
 */
SymbolEntry *symbol_table_scope_lookup_impl(const GlobalSymbolTable *table, const char *name,
                                            const char *current_scope, Language language) {
  if (!table || !name) {
    return NULL;
  }

  SymbolEntry *result = NULL;
  char qualified_name[256]; // Buffer for building qualified names

  // First, try direct lookup in case it's already fully qualified
  result = symbol_table_lookup_impl(table, name);
  if (result) {
    return result;
  }

  // Try resolving in the current scope, if provided
  if (current_scope && current_scope[0] != '\0') {
    switch (language) {
    case LANG_C:
    case LANG_CPP:
      // Use :: as separator for C/C++ scope
      snprintf(qualified_name, sizeof(qualified_name), "%s::%s", current_scope, name);
      break;

    case LANG_PYTHON:
    case LANG_JAVASCRIPT:
    case LANG_TYPESCRIPT:
      // Use . as separator for Python/JS/TS
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", current_scope, name);
      break;

    default:
      // Default to . as separator
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", current_scope, name);
      break;
    }

    result = symbol_table_lookup_impl(table, qualified_name);
    if (result) {
      return result;
    }
  }

  // Try registered scope prefixes
  for (size_t i = 0; i < table->num_scopes; i++) {
    const char *scope = table->scope_prefixes[i];

    switch (language) {
    case LANG_C:
    case LANG_CPP:
      // Use :: as separator for C/C++ scope
      snprintf(qualified_name, sizeof(qualified_name), "%s::%s", scope, name);
      break;

    case LANG_PYTHON:
    case LANG_JAVASCRIPT:
    case LANG_TYPESCRIPT:
      // Use . as separator for Python/JS/TS
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", scope, name);
      break;

    default:
      // Default to . as separator
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", scope, name);
      break;
    }

    result = symbol_table_lookup_impl(table, qualified_name);
    if (result) {
      return result;
    }
  }

  // Try language-specific global/built-in scopes as a last resort
  switch (language) {
  case LANG_PYTHON:
    // Check Python builtins
    snprintf(qualified_name, sizeof(qualified_name), "builtins.%s", name);
    result = symbol_table_lookup_impl(table, qualified_name);
    break;

  case LANG_JAVASCRIPT:
  case LANG_TYPESCRIPT:
    // Check global object
    snprintf(qualified_name, sizeof(qualified_name), "global.%s", name);
    result = symbol_table_lookup_impl(table, qualified_name);
    break;

  default:
    // No special handling for other languages
    break;
  }

  return result;
}

/**
 * Get all symbols of a specific type
 *
 * Filters the symbol table to find all entries with a matching node type.
 * This is useful for collecting all functions, classes, or other specific
 * symbol types from across the codebase.
 *
 * @param table Symbol table
 * @param type Node type to filter by
 * @param out_entries Output array of entries (can be NULL to just get the count)
 * @param max_entries Maximum number of entries to return
 * @return Number of matching entries found (may exceed max_entries)
 */
size_t symbol_table_get_by_type_impl(const GlobalSymbolTable *table, ASTNodeType type,
                                     SymbolEntry **out_entries, size_t max_entries) {
  if (!table) {
    return 0;
  }

  size_t count = 0;

  // Iterate through all buckets
  for (size_t i = 0; i < table->num_buckets; i++) {
    SymbolEntry *entry = table->buckets[i];
    while (entry) {
      // Check if this entry has the requested node type
      if (entry->node && entry->node->type == type) {
        if (out_entries && count < max_entries) {
          out_entries[count] = entry;
        }
        count++;
      }
      entry = entry->next;
    }
  }

  return count;
}

/**
 * Find all symbols in a specific file
 *
 * Filters the symbol table to find all entries defined in a particular file.
 * This is useful for file-level analysis and scoping.
 *
 * @param table Symbol table
 * @param file_path File path to filter by
 * @param out_entries Output array of entries (can be NULL to just get the count)
 * @param max_entries Maximum number of entries to return
 * @return Number of matching entries found (may exceed max_entries)
 */
size_t symbol_table_get_by_file_impl(const GlobalSymbolTable *table, const char *file_path,
                                     SymbolEntry **out_entries, size_t max_entries) {
  if (!table || !file_path) {
    return 0;
  }

  size_t count = 0;

  // Iterate through all buckets
  for (size_t i = 0; i < table->num_buckets; i++) {
    SymbolEntry *entry = table->buckets[i];
    while (entry) {
      // Check if this entry is from the requested file
      if (entry->file_path && strcmp(entry->file_path, file_path) == 0) {
        if (out_entries && count < max_entries) {
          out_entries[count] = entry;
        }
        count++;
      }
      entry = entry->next;
    }
  }

  return count;
}

/**
 * Find all symbols by language
 *
 * Filters the symbol table to find all entries of a specific language.
 * This is useful when working with multi-language codebases.
 *
 * @param table Symbol table
 * @param language Language to filter by
 * @param out_entries Output array of entries (can be NULL to just get the count)
 * @param max_entries Maximum number of entries to return
 * @return Number of matching entries found (may exceed max_entries)
 */
size_t symbol_table_get_by_language_impl(const GlobalSymbolTable *table, Language language,
                                         SymbolEntry **out_entries, size_t max_entries) {
  if (!table) {
    return 0;
  }

  size_t count = 0;

  // Iterate through all buckets
  for (size_t i = 0; i < table->num_buckets; i++) {
    SymbolEntry *entry = table->buckets[i];
    while (entry) {
      // Check if this entry is in the requested language
      if (entry->language == language) {
        if (out_entries && count < max_entries) {
          out_entries[count] = entry;
        }
        count++;
      }
      entry = entry->next;
    }
  }

  return count;
}

/**
 * Find all symbols within a scope
 *
 * Filters the symbol table to find all entries defined within a particular scope.
 * This is useful for namespace and class analysis.
 *
 * @param table Symbol table
 * @param scope_prefix Scope prefix to match against (e.g., "namespace1::class1")
 * @param out_entries Output array of entries (can be NULL to just get the count)
 * @param max_entries Maximum number of entries to return
 * @return Number of matching entries found (may exceed max_entries)
 */
size_t symbol_table_get_by_scope_impl(const GlobalSymbolTable *table, const char *scope_prefix,
                                      SymbolEntry **out_entries, size_t max_entries) {
  if (!table || !scope_prefix) {
    return 0;
  }

  size_t count = 0;
  size_t prefix_len = strlen(scope_prefix);

  // Iterate through all buckets
  for (size_t i = 0; i < table->num_buckets; i++) {
    SymbolEntry *entry = table->buckets[i];
    while (entry) {
      // Check if this entry's qualified name starts with the scope prefix
      if (entry->qualified_name && strncmp(entry->qualified_name, scope_prefix, prefix_len) == 0) {
        // Make sure it's an exact scope match
        if (entry->qualified_name[prefix_len] == '.' || entry->qualified_name[prefix_len] == ':') {
          if (out_entries && count < max_entries) {
            out_entries[count] = entry;
          }
          count++;
        }
      }
      entry = entry->next;
    }
  }

  return count;
}

void symbol_table_remove_by_file_impl(GlobalSymbolTable *table, const char *file_path) {
  log_debug("[SYMTAB] Called remove_by_file for file_path='%s'", SAFE_STR(file_path));
  if (!table || !file_path)
    return;

  size_t count = symbol_table_get_by_file_impl(table, file_path, NULL, 0);
  if (count == 0) {
    log_debug("[SYMTAB] No symbols found for file_path='%s'", SAFE_STR(file_path));
    return;
  }

  SymbolEntry **to_remove = malloc(sizeof(SymbolEntry *) * count);
  if (!to_remove)
    return;

  size_t found = symbol_table_get_by_file_impl(table, file_path, to_remove, count);

  log_debug("[SYMTAB] Removing %zu symbols for file_path='%s'", found, SAFE_STR(file_path));
  for (size_t i = 0; i < found; ++i) {
    SymbolEntry *entry = to_remove[i];
    log_debug("[SYMTAB] Removing symbol: qualified_name='%s', file_path='%s', entry=%p", SAFE_STR(entry->qualified_name), SAFE_STR(entry->file_path), (void*)entry);
    uint32_t hash = hash_string(entry->qualified_name, table->num_buckets);
    SymbolEntry **prev = &table->buckets[hash];
    while (*prev) {
      if (*prev == entry) {
        *prev = entry->next;
        symbol_entry_free(entry);
        table->num_symbols--;
        break;
      }
      prev = &((*prev)->next);
    }
  }
  free(to_remove);
}
