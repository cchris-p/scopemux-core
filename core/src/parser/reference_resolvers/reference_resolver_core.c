/**
 * @file reference_resolver_core.c
 * @brief Core implementation of the reference resolution system
 *
 * This file implements the central reference resolution system that delegates
 * to language-specific resolvers. It manages resolver registration, statistics
 * tracking, and core resolution logic.
 */

#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Forward declarations for language-specific resolvers
 */
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
ResolverRegistry resolver_registry = {0}; // Global dynamic registry instance

/**
 * Resolution statistics
 */
static struct {
  size_t total_lookups;
  size_t resolved_count;
  size_t failed_count;
} resolver_stats = {0};

/**
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
 * @brief Initialize the reference resolver module
 *
 * Sets up the resolver registry and registers default language-specific resolvers.
 *
 * @return true if initialization was successful, false otherwise
 */
bool reference_resolver_init() {
  // Reset registry
  memset(&resolver_registry, 0,
         sizeof(resolver_registry)); // Reset the registry struct (not an array)
  // Optionally, free any dynamically allocated memory if needed

  // Reset statistics
  resolver_stats.total_lookups = 0;
  resolver_stats.resolved_count = 0;
  resolver_stats.failed_count = 0;

  // Register default resolvers
  reference_resolver_register(LANG_C, reference_resolver_c, NULL, NULL);
  reference_resolver_register(LANG_CPP, reference_resolver_cpp, NULL, NULL);
  reference_resolver_register(LANG_PYTHON, reference_resolver_python, NULL, NULL);
  reference_resolver_register(LANG_JAVASCRIPT, reference_resolver_javascript, NULL, NULL);
  reference_resolver_register(LANG_TYPESCRIPT, reference_resolver_typescript, NULL, NULL);

  return true;
}

/**
 * @brief Clean up the reference resolver module
 *
 * Releases any resources allocated by the reference resolver system.
 */
void reference_resolver_cleanup() {
  // Clean up any resolver-specific data
  for (int i = 0; i < LANG_MAX; i++) {
    // Use the registry API to iterate and clean up each registered resolver
    LanguageResolver *entry = resolver_registry.resolvers[i];
    if (entry && entry->resolver_data) {
      // Free resolver data if needed
      // Note: currently this doesn't allocate any memory, but might in the future
    }

    // Use the registry API to clean up the entry (if needed)
    entry->resolver_func = NULL;
    entry->resolver_data = NULL;
    entry->cleanup_func = NULL;
  }
}

/**
 * @brief Register a language-specific resolver
 *
 * @param lang Language to register the resolver for
 * @param resolver_func Function pointer to the resolver implementation
 * @param resolver_data Optional data to pass to the resolver
 * @return true if registration was successful, false otherwise
 */

/**
 * @brief Unregister a language-specific resolver
 *
 * @param lang Language to unregister the resolver for
 * @return true if unregistration was successful, false otherwise
 */

/**
 * @brief Resolve a reference in an ASTNode
 *
 * This is the main entry point for reference resolution. It delegates to
 * the appropriate language-specific resolver if one is registered. Otherwise,
 * it returns RESOLUTION_NOT_IMPLEMENTED to indicate no resolver is available.
 *
 * Following the strict facade pattern, there are no fallbacks - only explicit delegation.
 *
 * @param node The node containing the reference to resolve
 * @param ref_type Type of reference being resolved
 * @param name Name of the symbol being referenced
 * @param symbol_table Global symbol table to search in
 * @return Resolution status indicating success, failure, or not implemented
 */

/**
 * @brief Get resolver statistics
 *
 * @param total_lookups Output parameter for total number of lookups
 * @param resolved_count Output parameter for number of successfully resolved references
 * @param failed_count Output parameter for number of failed resolutions
 */

/**
 * @brief Reset resolver statistics
 *
 * Resets all counters tracking resolution performance.
 */
