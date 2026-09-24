/**
 * @file rust_resolver.c
 * @brief Rust language reference resolver (first pass)
 *
 * Resolves Rust identifier and path references against the project symbol
 * table using scope-aware lookup. Full Rust semantics (trait objects, macros,
 * dynamic dispatch, generic type parameters) are documented known gaps for a
 * follow-up pass; this resolver handles direct symbol and qualified-path hits.
 */

#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Resolve a Rust reference by direct name, last path segment, or scope.
 */
ResolutionStatus reference_resolver_rust(ASTNode *node, ReferenceType ref_type, const char *name,
                                         GlobalSymbolTable *symbol_table, void *resolver_data) {
  const char *bare = name;

  (void)ref_type;
  (void)resolver_data;

  if (!node || !name || !symbol_table) {
    return RESOLUTION_ERROR;
  }

  // For a path like `module::Type::method`, fall back to the final segment.
  for (const char *cursor = name; (cursor = strstr(cursor, "::")) != NULL; cursor += 2) {
    bare = cursor + 2;
  }

  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (!entry && bare != name) {
    entry = symbol_table_lookup(symbol_table, bare);
  }
  if (!entry) {
    const char *current_scope =
        (node->parent && node->parent->qualified_name) ? node->parent->qualified_name : NULL;
    entry = symbol_table_scope_lookup(symbol_table, bare, current_scope, LANG_RUST);
  }

  if (entry && entry->node) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}
