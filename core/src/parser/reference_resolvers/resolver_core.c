/**
 * @file resolver_core.c
 * @brief Core lifecycle management for reference resolver
 *
 * This file handles the core operations of the reference resolver:
 * - Creation and initialization
 * - Cleanup and memory management
 * - Resolver statistics tracking
 */

#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of language-specific resolvers we expect to register
#define MAX_LANGUAGE_RESOLVERS 16

/**
 * Create a new reference resolver
 */
ReferenceResolver *reference_resolver_create_impl(GlobalSymbolTable *symbol_table);

/**
 * Free all resources associated with a reference resolver
 */
void reference_resolver_free_impl(ReferenceResolver *resolver);

/**
 * Get resolver statistics
 */
void reference_resolver_get_stats_impl(const ReferenceResolver *resolver,
                                       size_t *out_total_references,
                                       size_t *out_resolved_references,
                                       size_t *out_unresolved_references);

/**
 * Register a language-specific resolver
 */
bool reference_resolver_register_impl(ReferenceResolver *resolver, Language language,
                                      ResolverFunction resolver_func, void *resolver_data,
                                      ResolverCleanupFunction cleanup_func);

/**
 * Unregister a language-specific resolver
 */
bool reference_resolver_unregister_impl(ReferenceResolver *resolver, Language language);
