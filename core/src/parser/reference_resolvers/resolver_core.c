/**
 * @file resolver_core.c
 * @brief Core lifecycle management for reference resolver
 *
 * This file handles the core operations of the reference resolver:
 * - Creation and initialization
 * - Cleanup and memory management
 * - Resolver statistics tracking
 */

#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of language-specific resolvers we expect to register
#define MAX_LANGUAGE_RESOLVERS 16

/**
 * Create a new reference resolver
 */
ReferenceResolver *reference_resolver_create_impl(GlobalSymbolTable *symbol_table) {
  if (!symbol_table) {
    return NULL;
  }

  ReferenceResolver *resolver = (ReferenceResolver *)malloc(sizeof(ReferenceResolver));
  if (!resolver) {
    return NULL;
  }

  // Initialize all fields
  memset(resolver, 0, sizeof(ReferenceResolver));
  resolver->symbol_table = symbol_table;

  // Allocate resolver array
  resolver->language_resolvers =
      (LanguageResolver *)calloc(MAX_LANGUAGE_RESOLVERS, sizeof(LanguageResolver));
  if (!resolver->language_resolvers) {
    free(resolver);
    return NULL;
  }

  return resolver;
}

/**
 * Free all resources associated with a reference resolver
 */
void reference_resolver_free_impl(ReferenceResolver *resolver) {
  if (!resolver) {
    return;
  }

  // Free language-specific resolver data
  for (size_t i = 0; i < resolver->num_resolvers; i++) {
    if (resolver->language_resolvers[i].cleanup_func) {
      resolver->language_resolvers[i].cleanup_func(resolver->language_resolvers[i].resolver_data);
    }
  }

  // Free arrays and resolver
  free(resolver->language_resolvers);
  free(resolver);
}

/**
 * Get resolver statistics
 */
void reference_resolver_get_stats_impl(const ReferenceResolver *resolver,
                                       size_t *out_total_references,
                                       size_t *out_resolved_references,
                                       size_t *out_unresolved_references) {
  if (!resolver) {
    if (out_total_references)
      *out_total_references = 0;
    if (out_resolved_references)
      *out_resolved_references = 0;
    if (out_unresolved_references)
      *out_unresolved_references = 0;
    return;
  }

  if (out_total_references)
    *out_total_references = resolver->total_references;
  if (out_resolved_references)
    *out_resolved_references = resolver->resolved_references;
  if (out_unresolved_references)
    *out_unresolved_references = resolver->total_references - resolver->resolved_references;
}

/**
 * Register a language-specific resolver
 */
bool reference_resolver_register_impl(ReferenceResolver *resolver, LanguageType language,
                                      ResolverFunction resolver_func, void *resolver_data,
                                      ResolverCleanupFunction cleanup_func) {
  if (!resolver || !resolver_func) {
    return false;
  }

  // Check if this language already has a resolver
  for (size_t i = 0; i < resolver->num_resolvers; i++) {
    if (resolver->language_resolvers[i].language == language) {
      // Clean up existing resolver if needed
      if (resolver->language_resolvers[i].cleanup_func) {
        resolver->language_resolvers[i].cleanup_func(resolver->language_resolvers[i].resolver_data);
      }

      // Replace with new resolver
      resolver->language_resolvers[i].resolver_func = resolver_func;
      resolver->language_resolvers[i].resolver_data = resolver_data;
      resolver->language_resolvers[i].cleanup_func = cleanup_func;
      return true;
    }
  }

  // Check if we have space for more resolvers
  if (resolver->num_resolvers >= MAX_LANGUAGE_RESOLVERS) {
    return false;
  }

  // Add new resolver
  resolver->language_resolvers[resolver->num_resolvers].language = language;
  resolver->language_resolvers[resolver->num_resolvers].resolver_func = resolver_func;
  resolver->language_resolvers[resolver->num_resolvers].resolver_data = resolver_data;
  resolver->language_resolvers[resolver->num_resolvers].cleanup_func = cleanup_func;
  resolver->num_resolvers++;

  return true;
}

/**
 * Unregister a language-specific resolver
 */
bool reference_resolver_unregister_impl(ReferenceResolver *resolver, LanguageType language) {
  if (!resolver) {
    return false;
  }

  // Find the resolver to remove
  for (size_t i = 0; i < resolver->num_resolvers; i++) {
    if (resolver->language_resolvers[i].language == language) {
      // Clean up if needed
      if (resolver->language_resolvers[i].cleanup_func) {
        resolver->language_resolvers[i].cleanup_func(resolver->language_resolvers[i].resolver_data);
      }

      // Shift remaining resolvers
      if (i < resolver->num_resolvers - 1) {
        memmove(&resolver->language_resolvers[i], &resolver->language_resolvers[i + 1],
                (resolver->num_resolvers - i - 1) * sizeof(LanguageResolver));
      }

      // Update count and return
      resolver->num_resolvers--;
      return true;
    }
  }

  return false;
}
