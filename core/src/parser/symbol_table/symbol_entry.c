/**
 * @file symbol_entry.c
 * @brief Symbol entry management for the global symbol table
 *
 * Implements functions for creating, manipulating, and freeing
 * individual symbol entries, along with utility functions for
 * extracting and processing symbol names and attributes.
 */

#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include "symbol_table_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Extract the simple name from a qualified name
 *
 * Parses a fully qualified name to extract its simple name component.
 * For example, from "namespace::class::method", extracts "method".
 *
 * @param qualified_name The fully qualified name to parse
 * @return The extracted simple name (caller must free) or NULL on failure
 */
char *extract_simple_name(const char *qualified_name) {
  if (!qualified_name) {
    return NULL;
  }

  // Find the last separator (either '.' or ':')
  const char *last_dot = strrchr(qualified_name, '.');
  const char *last_colon = strrchr(qualified_name, ':');

  // Use the rightmost separator
  const char *last_sep = (last_dot > last_colon) ? last_dot : last_colon;

  // If no separator was found, the simple name is the same as the qualified name
  if (!last_sep) {
    return strdup(qualified_name);
  }

  // Skip any additional colons (for C++ '::', last_colon points to the first ':')
  if (last_sep == last_colon && *(last_sep + 1) == ':') {
    last_sep++;
  }

  // Extract the part after the separator
  return strdup(last_sep + 1);
}

/**
 * Create a new symbol entry
 *
 * Allocates and initializes a SymbolEntry structure with the
 * provided information. The caller maintains ownership of the
 * ASTNode, but all strings are duplicated.
 *
 * @param qualified_name Fully qualified name of the symbol
 * @param node AST node representing the symbol
 * @param file_path Path to the source file containing this symbol
 * @param scope Symbol visibility scope
 * @param language Language of the source file
 * @return A new symbol entry or NULL on failure
 */
SymbolEntry *symbol_table_entry_create_impl(const char *qualified_name, ASTNode *node,
                                            const char *file_path, SymbolScope scope,
                                            Language language) {
  if (!qualified_name || !node || !file_path) {
    return NULL;
  }

  SymbolEntry *entry = (SymbolEntry *)malloc(sizeof(SymbolEntry));
  if (!entry) {
    return NULL;
  }

  // Initialize all fields to safe values
  memset(entry, 0, sizeof(SymbolEntry));

  // Copy strings
  entry->qualified_name = strdup(qualified_name);
  entry->file_path = strdup(file_path);
  entry->simple_name = extract_simple_name(qualified_name);

  // Check if any allocation failed
  if (!entry->qualified_name || !entry->file_path || !entry->simple_name) {
    symbol_table_entry_free_impl(entry);
    return NULL;
  }

  // Set other fields
  entry->node = node;
  entry->scope = scope;
  entry->language = language;
  entry->is_definition = true; // Assume it's a definition by default

  return entry;
}

/**
 * Free a symbol entry
 *
 * Releases all memory allocated for a symbol entry, including
 * all copied strings. The associated ASTNode is not freed, as
 * its ownership remains with the caller.
 *
 * @param entry Symbol entry to free
 */
void symbol_table_entry_free_impl(SymbolEntry *entry) {
  if (!entry) {
    return;
  }

  // Free strings
  free(entry->qualified_name);
  free(entry->simple_name);
  free(entry->file_path);
  free(entry->module_path);

  // Free the entry itself
  free(entry);
}

/**
 * Set the module path for a symbol
 *
 * Associates a module path (e.g., import path or include path)
 * with a symbol entry, which is useful for tracking dependencies.
 *
 * @param entry Symbol entry to update
 * @param module_path Module path to set (will be copied)
 * @return true on success, false on failure
 */
bool symbol_entry_set_module_path(SymbolEntry *entry, const char *module_path) {
  if (!entry || !module_path) {
    return false;
  }

  // Free any existing module path
  free(entry->module_path);

  // Set the new module path
  entry->module_path = strdup(module_path);
  return entry->module_path != NULL;
}

/**
 * Set whether this entry represents a definition or declaration
 *
 * Updates the is_definition flag for a symbol entry, which helps
 * distinguish between declarations and definitions of the same symbol.
 *
 * @param entry Symbol entry to update
 * @param is_definition true if this is a definition, false if it's only a declaration
 */
void symbol_entry_set_definition(SymbolEntry *entry, bool is_definition) {
  if (entry) {
    entry->is_definition = is_definition;
  }
}

/**
 * Set the parent symbol for a symbol
 *
 * Establishes a hierarchical relationship between symbols, such as
 * a method belonging to a class or a variable belonging to a namespace.
 *
 * @param entry Symbol entry to update
 * @param parent Parent symbol entry
 */
void symbol_entry_set_parent(SymbolEntry *entry, SymbolEntry *parent) {
  if (entry) {
    entry->parent = parent;
  }
}
