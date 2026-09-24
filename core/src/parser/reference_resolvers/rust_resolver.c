/**
 * @file rust_resolver.c
 * @brief Rust language reference resolver
 *
 * Resolves Rust identifier and path references against the project symbol
 * table using scope-aware lookup and `::` path segmentation. It handles the
 * shapes the parser currently emits (use paths, type names, function/method
 * calls) and falls back to a simple-name scan for cross-module paths such as
 * `crate::foo::bar`.
 *
 * Known gaps (tracked on WI-030): trait-object / dynamic-dispatch resolution,
 * macro expansion, generic type-parameter binding, and full `use` path
 * canonicalization (aliases, globs, nested groups).
 */

#include "../../../include/scopemux/ast.h"
#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Return the final `::` segment of a Rust path.
 */
static const char *rust_last_segment(const char *name) {
  const char *bare = name;

  if (!name) {
    return NULL;
  }

  for (const char *cursor = name; (cursor = strstr(cursor, "::")) != NULL; cursor += 2) {
    bare = cursor + 2;
  }

  return bare;
}

/**
 * @brief Copy a Rust identifier, stripping leading sigils and generic arguments.
 *
 * Examples: `Vec<Item>` -> `Vec`, `&mut Foo` -> `Foo`, `Foo<'a>` -> `Foo`.
 */
static void rust_clean_identifier(const char *input, char *out, size_t out_size) {
  size_t written = 0;

  if (!out || out_size == 0) {
    return;
  }
  out[0] = '\0';

  if (!input) {
    return;
  }

  while (*input == '&' || *input == '*' || *input == ' ' || *input == '\'') {
    input++;
  }

  while (*input && written + 1 < out_size) {
    if (*input == '<' || *input == '(' || *input == '[' || *input == ' ' || *input == '\t' ||
        *input == '\n' || *input == '\r') {
      break;
    }
    out[written++] = *input++;
  }

  out[written] = '\0';
}

/**
 * @brief Whether a resolved entry is compatible with the reference type.
 */
static bool rust_entry_matches_type(const SymbolEntry *entry, ReferenceType ref_type) {
  ASTNodeType type;

  if (!entry || !entry->node) {
    return false;
  }

  type = entry->node->type;
  switch (ref_type) {
  case REF_CALL:
  case REF_OVERRIDE:
    return type == NODE_FUNCTION || type == NODE_METHOD || type == NODE_MACRO;
  case REF_TYPE:
  case REF_INHERITANCE:
  case REF_IMPLEMENTATION:
  case REF_INTERFACE:
    return type == NODE_STRUCT || type == NODE_UNION || type == NODE_TYPEDEF || type == NODE_ENUM ||
           type == NODE_CLASS || type == NODE_INTERFACE;
  default:
    return true;
  }
}

/**
 * @brief Scan the symbol table for a symbol whose simple name matches.
 *
 * Used as a cross-module fallback when a qualified path cannot be resolved by
 * exact or scope lookup.
 */
static SymbolEntry *rust_find_by_simple_name(GlobalSymbolTable *symbol_table, const char *simple_name,
                                             ReferenceType ref_type) {
  if (!symbol_table || !simple_name || simple_name[0] == '\0') {
    return NULL;
  }

  for (size_t i = 0; i < symbol_table->num_buckets; i++) {
    SymbolEntry *entry = symbol_table->buckets[i];
    while (entry) {
      bool name_matches = false;

      if (entry->simple_name && strcmp(entry->simple_name, simple_name) == 0) {
        name_matches = true;
      } else if (entry->qualified_name) {
        const char *last = rust_last_segment(entry->qualified_name);
        if (last && strcmp(last, simple_name) == 0) {
          name_matches = true;
        }
      }

      if (name_matches && rust_entry_matches_type(entry, ref_type)) {
        return entry;
      }

      entry = entry->next;
    }
  }

  return NULL;
}

/**
 * @brief Resolve a Rust reference by exact name, path segment, scope, or simple name.
 */
ResolutionStatus reference_resolver_rust(ASTNode *node, ReferenceType ref_type, const char *name,
                                         GlobalSymbolTable *symbol_table, void *resolver_data) {
  const char *bare;
  char bare_buf[256];
  SymbolEntry *entry = NULL;

  (void)resolver_data;

  if (!node || !name || !symbol_table) {
    return RESOLUTION_ERROR;
  }

  if (name[0] == '\0') {
    return RESOLUTION_NOT_FOUND;
  }

  // For a path like `module::Type::method`, fall back to the final segment.
  bare = rust_last_segment(name);
  if (!bare || bare[0] == '\0') {
    bare = name;
  }

  // Strip generic arguments and sigils so `Vec<Item>` resolves to `Vec`.
  rust_clean_identifier(bare, bare_buf, sizeof(bare_buf));
  if (bare_buf[0] != '\0') {
    bare = bare_buf;
  }

  // 1. The name exactly as written (fully qualified in the symbol table).
  entry = symbol_table_lookup(symbol_table, name);

  // 2. The final path segment.
  if (!entry && strcmp(bare, name) != 0) {
    entry = symbol_table_lookup(symbol_table, bare);
  }

  // 3. Scope-aware lookup from the enclosing item.
  if (!entry) {
    const char *current_scope =
        (node->parent && node->parent->qualified_name) ? node->parent->qualified_name : NULL;
    entry = symbol_table_scope_lookup(symbol_table, bare, current_scope, LANG_RUST);
  }

  // 4. Cross-module fallback by simple name for paths/imports/types/calls.
  if (!entry) {
    entry = rust_find_by_simple_name(symbol_table, bare, ref_type);
  }

  if (entry && entry->node) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}
