/**
 * @file reference_resolver_facade.c
 * @brief Implementation of the reference resolver facade functions
 *
 * This file implements the main facade functions for the reference resolver
 * that are exposed in the public API but were missing from the implementation.
 * These functions delegate to the appropriate implementation functions.
 */

#include "reference_resolver_internal.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Initialize the reference resolver with built-in resolvers
 *
 * This function creates a new reference resolver and initializes it
 * with built-in resolvers for all supported languages.
 *
 * @param symbol_table The global symbol table to use for resolution
 * @return A newly initialized reference resolver, or NULL on failure
 */
ReferenceResolver *reference_resolver_init(GlobalSymbolTable *symbol_table) {
  // Create a new reference resolver
  ReferenceResolver *resolver = reference_resolver_create_impl(symbol_table);
  if (!resolver) {
    log_error("Failed to create reference resolver");
    return NULL;
  }

  // Initialize built-in resolvers
  if (!reference_resolver_init_builtin_impl(resolver)) {
    log_error("Failed to initialize built-in resolvers");
    reference_resolver_free_impl(resolver);
    return NULL;
  }

  return resolver;
}

/**
 * Resolve a reference in an AST node
 *
 * This function resolves a reference in an AST node by delegating to
 * the appropriate language-specific resolver.
 *
 * @param resolver The reference resolver
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param qualified_name The qualified name to resolve
 * @return Resolution status
 */
ResolutionStatus reference_resolver_resolve(ReferenceResolver *resolver, ASTNode *node,
                                            ReferenceType ref_type, const char *qualified_name) {
  return reference_resolver_resolve_node_impl(resolver, node, ref_type, qualified_name);
}

/**
 * Add a reference with metadata to an AST node
 *
 * This function adds a reference with metadata to an AST node.
 * It is used by reference resolvers to establish relationships
 * between nodes with additional semantic information.
 *
 * @param node The source AST node
 * @param target The target AST node to reference
 * @param relationship_type The type of relationship (e.g., "calls", "inherits", etc.)
 * @return true if the reference was added, false otherwise
 */
bool ast_node_add_reference_with_metadata(ASTNode *node, ASTNode *target,
                                          const char *relationship_type) {
  if (!node || !target) {
    return false;
  }

  // First, add the basic reference
  if (node->num_references >= node->references_capacity) {
    // Need to resize references array
    size_t new_capacity = node->references_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4;
    }

    ASTNode **new_refs = (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));
    if (!new_refs) {
      log_error("Failed to resize references array");
      return false;
    }

    node->references = new_refs;
    node->references_capacity = new_capacity;
  }

  // Add the reference
  node->references[node->num_references++] = target;

  // Add metadata if provided
  if (relationship_type) {
    // Store the relationship type as a property
    // Note: This assumes ast_node_set_property is implemented elsewhere
    char property_name[64];
    snprintf(property_name, sizeof(property_name), "ref_type_%zu", node->num_references - 1);

    // We would call ast_node_set_property here, but for now just log
    log_debug("Added reference from %s to %s with type %s", node->name ? node->name : "(unnamed)",
              target->name ? target->name : "(unnamed)", relationship_type);
  }

  return true;
}

/**
 * Get statistics about reference resolution
 *
 * This function retrieves statistics about reference resolution,
 * such as the number of total, resolved, and unresolved references.
 *
 * @param resolver The reference resolver
 * @param out_total_references Optional output for total references
 * @param out_resolved_references Optional output for resolved references
 * @param out_unresolved_references Optional output for unresolved references
 */
void reference_resolver_get_stats(const ReferenceResolver *resolver, size_t *out_total_references,
                                  size_t *out_resolved_references,
                                  size_t *out_unresolved_references) {
  reference_resolver_get_stats_impl(resolver, out_total_references, out_resolved_references,
                                    out_unresolved_references);
}
