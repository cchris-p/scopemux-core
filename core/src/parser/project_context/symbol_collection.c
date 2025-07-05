/**
 * @file symbol_collection.c
 * @brief Symbol collection functionality for ProjectContext
 *
 * Implements functions for adding symbols to a collection during project analysis.
 */

#include "scopemux/symbol_collection.h"
#include "../parser_internal.h" // For ASTNODE_MAGIC
#include "project_context_internal.h"
#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol.h"
#include "scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAFE_STR(x) ((x) ? (x) : "(null)")

/**
 * Add a symbol to a symbol collection
 *
 * This function adds a symbol with the given name, type, and associated AST node
 * to the provided symbol collection.
 *
 * @param symbols The symbol collection to add to
 * @param name The name of the symbol
 * @param symbol_type The type of the symbol (SYMBOL_FUNCTION, SYMBOL_TYPE, etc.)
 * @param node The AST node associated with this symbol
 */
void symbol_collection_add(void *symbols, const char *name, int symbol_type, ASTNode *node) {
  log_debug("symbol_collection_add: Entry point (symbols=%p, name=%s, symbol_type=%d, node=%p)",
            symbols, SAFE_STR(name), symbol_type, (void *)node);

  if (!symbols || !name || !node) {
    log_error("symbol_collection_add: Invalid parameters (symbols=%p, name=%s, node=%p)", symbols,
              SAFE_STR(name), node);
    return;
  }

  // Validate the node magic number to ensure it's a valid ASTNode
  log_debug("symbol_collection_add: Checking node magic number: %x", node->magic);
  if (node->magic != ASTNODE_MAGIC) {
    log_error("symbol_collection_add: Invalid ASTNode magic number: %x (expected %x)", node->magic,
              ASTNODE_MAGIC);
    return;
  }
  log_debug("symbol_collection_add: Valid ASTNode magic number confirmed");

  // Log the symbol being added
  log_debug("Adding symbol to collection: %s (type: %d)", SAFE_STR(name), symbol_type);

  // The actual implementation would depend on what the 'symbols' void pointer
  // is expected to be. Based on the project context, it's likely a collection
  // or table of some kind.

  // For now, we'll implement a basic version that assumes 'symbols' is a
  // Cast the void pointer to the correct type
  GlobalSymbolTable *table = (GlobalSymbolTable *)symbols;

  // Validate the symbol table
  if (!table) {
    log_error("symbol_collection_add: Invalid symbol table pointer");
    return;
  }

  // Get the file path from the node
  const char *file_path = "unknown";
  log_debug("symbol_collection_add: Checking node->file_path: %p", node->file_path);
  if (node->file_path) {
    file_path = node->file_path;
    log_debug("symbol_collection_add: Using file_path from node: %s", SAFE_STR(file_path));
  } else {
    log_debug("symbol_collection_add: No file_path in node, using default: %s",
              SAFE_STR(file_path));
  }

  // Log the file path for debugging
  log_debug("Symbol %s from file: %s", SAFE_STR(name), SAFE_STR(file_path));

  // Map the symbol_type to a SymbolScope
  SymbolScope scope = SCOPE_UNKNOWN;
  switch (symbol_type) {
  case SYMBOL_FUNCTION:
    scope = SCOPE_GLOBAL; // Default to global scope for functions
    break;
  case SYMBOL_METHOD:
    scope = SCOPE_CLASS; // Methods belong to classes
    break;
  case SYMBOL_CLASS:
    scope = SCOPE_GLOBAL; // Default to global scope for classes
    break;
  case SYMBOL_VARIABLE:
    scope = SCOPE_FILE; // Default to file scope for variables
    break;
  case SYMBOL_NAMESPACE:
    scope = SCOPE_GLOBAL; // Default to global scope for namespaces
    break;
  case SYMBOL_MODULE:
    scope = SCOPE_MODULE; // Module scope for modules
    break;
  case SYMBOL_TYPE:
    scope = SCOPE_GLOBAL; // Default to global scope for types
    break;
  case SYMBOL_ENUM:
    scope = SCOPE_GLOBAL; // Default to global scope for enums
    break;
  default:
    scope = SCOPE_UNKNOWN;
    break;
  }

  // Determine the language from the node
  Language language = node->lang;
  log_debug("symbol_collection_add: Node language value: %d", language);

  // Validate language
  if (language <= LANG_UNKNOWN || language > LANG_MAX) {
    log_warning("symbol_collection_add: Invalid language value %d for symbol %s, defaulting to "
                "LANG_UNKNOWN",
                language, SAFE_STR(name));
    language = LANG_UNKNOWN;
  }
  log_debug("symbol_collection_add: Using language: %d", language);

  // Register the symbol in the table
  log_debug("symbol_collection_add: Registering symbol %s with table=%p, file_path=%s, scope=%d, "
            "language=%d",
            SAFE_STR(name), (void *)table, SAFE_STR(file_path), scope, language);
  SymbolEntry *entry = symbol_table_register(table, name, node, file_path, scope, language);
  if (!entry) {
    log_error("Failed to register symbol %s", SAFE_STR(name));
    return;
  }
  log_debug("symbol_collection_add: Successfully registered symbol %s, entry=%p", SAFE_STR(name),
            (void *)entry);

  // Symbol successfully added
  log_debug("Successfully added symbol %s to collection", SAFE_STR(name));
}
