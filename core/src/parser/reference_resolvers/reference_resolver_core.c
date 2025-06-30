#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for language-specific resolvers
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data);

ResolutionStatus reference_resolver_cpp(ASTNode *node, ReferenceType ref_type, const char *name,
                                        GlobalSymbolTable *symbol_table, void *resolver_data);

ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data);

ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

/**
 * Global registry of language-specific resolvers
 */
static struct {
  ResolverFunction resolver;
  void *resolver_data;
  bool is_registered;
} resolver_registry[LANG_MAX] = {0};

/**
 * Resolution statistics
 */
static struct {
  size_t total_lookups;
  size_t resolved_count;
  size_t failed_count;
} resolver_stats = {0};

/**
 * Generic reference resolution implementation that can be used
 * as a fallback for language-specific resolvers
 */
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table) {
  if (!node || !name || !symbol_table) {
    resolver_stats.failed_count++;
    return RESOLUTION_FAILED;
  }

  resolver_stats.total_lookups++;

  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);

  if (entry) {
    // Add reference to node
    if (node->num_references < node->references_capacity) {
      node->references[node->num_references++] = entry->node;
      resolver_stats.resolved_count++;
      return RESOLUTION_SUCCESS;
    } else {
      // Need to resize references array
      size_t new_capacity = node->references_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 4;

      ASTNode **new_refs = (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

      if (new_refs) {
        node->references = new_refs;
        node->references_capacity = new_capacity;
        node->references[node->num_references++] = entry->node;
        resolver_stats.resolved_count++;
        return RESOLUTION_SUCCESS;
      }
    }

    resolver_stats.failed_count++;
    return RESOLUTION_FAILED;
  }

  // Try scope-based lookup if direct lookup failed
  // Lang is needed here because lookup rules are language-sensitive
  if (node->parent && node->parent->qualified_name) {
    entry = symbol_table_scope_lookup(symbol_table, name, node->parent->qualified_name, node->lang);

    if (entry) {
      // Add reference to node
      if (node->num_references < node->references_capacity) {
        node->references[node->num_references++] = entry->node;
        resolver_stats.resolved_count++;
        return RESOLUTION_SUCCESS;
      } else {
        // Need to resize references array
        size_t new_capacity = node->references_capacity * 2;
        if (new_capacity == 0)
          new_capacity = 4;

        ASTNode **new_refs =
            (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

        if (new_refs) {
          node->references = new_refs;
          node->references_capacity = new_capacity;
          node->references[node->num_references++] = entry->node;
          resolver_stats.resolved_count++;
          return RESOLUTION_SUCCESS;
        }
      }

      resolver_stats.failed_count++;
      return RESOLUTION_FAILED;
    }
  }

  // Not found
  resolver_stats.failed_count++;
  return RESOLUTION_NOT_FOUND;
}

/**
 * Initialize the reference resolver module
 */
bool reference_resolver_init() {
  // Reset registry
  memset(resolver_registry, 0, sizeof(resolver_registry));

  // Reset statistics
  resolver_stats.total_lookups = 0;
  resolver_stats.resolved_count = 0;
  resolver_stats.failed_count = 0;

  // Register default resolvers
  reference_resolver_register(LANG_C, reference_resolver_c, NULL);
  reference_resolver_register(LANG_CPP, reference_resolver_cpp, NULL);
  reference_resolver_register(LANG_PYTHON, reference_resolver_python, NULL);
  reference_resolver_register(LANG_JAVASCRIPT, reference_resolver_javascript, NULL);
  reference_resolver_register(LANG_TYPESCRIPT, reference_resolver_typescript, NULL);

  return true;
}

/**
 * Clean up the reference resolver module
 */
void reference_resolver_cleanup() {
  // Clean up any resolver-specific data
  for (int i = 0; i < LANG_MAX; i++) {
    if (resolver_registry[i].is_registered && resolver_registry[i].resolver_data) {
      // Free resolver data if needed
      // Note: currently this doesn't allocate any memory, but might in the future
    }

    resolver_registry[i].is_registered = false;
    resolver_registry[i].resolver = NULL;
    resolver_registry[i].resolver_data = NULL;
  }
}

/**
 * Register a language-specific resolver
 */
bool reference_resolver_register(Language lang, ResolverFunction resolver_func,
                                 void *resolver_data) {
  if (lang < 0 || lang >= LANG_MAX || !resolver_func) {
    return false;
  }

  resolver_registry[lang].resolver = resolver_func;
  resolver_registry[lang].resolver_data = resolver_data;
  resolver_registry[lang].is_registered = true;

  return true;
}

/**
 * Unregister a language-specific resolver
 */
bool reference_resolver_unregister(Language lang) {
  if (lang < 0 || lang >= LANG_MAX) {
    return false;
  }

  resolver_registry[lang].resolver = NULL;
  resolver_registry[lang].resolver_data = NULL;
  resolver_registry[lang].is_registered = false;

  return true;
}

/**
 * Resolve a reference in an ASTNode
 *
 * This is the main entry point for reference resolution. It will delegate to
 * the appropriate language-specific resolver if one is registered. Otherwise,
 * it will fall back to the generic resolver.
 */
ResolutionStatus reference_resolver_resolve(ASTNode *node, ReferenceType ref_type, const char *name,
                                            GlobalSymbolTable *symbol_table) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_FAILED;
  }

  Language lang = node->lang;

  // Use language-specific resolver if available
  if (lang >= 0 && lang < LANG_MAX && resolver_registry[lang].is_registered &&
      resolver_registry[lang].resolver) {

    return resolver_registry[lang].resolver(node, ref_type, name, symbol_table,
                                            resolver_registry[lang].resolver_data);
  }

  // Fall back to generic resolver
  return reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
}

/**
 * Get resolver statistics
 */
void reference_resolver_get_stats(size_t *total_lookups, size_t *resolved_count,
                                  size_t *failed_count) {
  if (total_lookups)
    *total_lookups = resolver_stats.total_lookups;
  if (resolved_count)
    *resolved_count = resolver_stats.resolved_count;
  if (failed_count)
    *failed_count = resolver_stats.failed_count;
}

/**
 * Reset resolver statistics
 */
void reference_resolver_reset_stats() {
  resolver_stats.total_lookups = 0;
  resolver_stats.resolved_count = 0;
  resolver_stats.failed_count = 0;
}
