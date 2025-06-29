/**
 * @file resolver_resolution.c
 * @brief Implementation of resolution operations
 *
 * This file handles:
 * - Generic resolution strategies
 * - Node-specific resolution
 * - File-level resolution
 * - Project-level resolution
 */

#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Find the appropriate resolver for a language
 */
extern LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver,
                                                     LanguageType language);

/**
 * Resolve a reference in a specific node
 */
ResolutionStatus reference_resolver_resolve_node_impl(ReferenceResolver *resolver, ASTNode *node,
                                                      ReferenceType ref_type,
                                                      const char *qualified_name) {
  if (!resolver || !node || !qualified_name) {
    return RESOLUTION_ERROR;
  }

  // Find the appropriate language resolver
  LanguageResolver *lang_resolver = find_language_resolver_impl(resolver, node->language);

  // Track statistics
  resolver->total_references++;

  // If we have a language-specific resolver, use it
  if (lang_resolver && lang_resolver->resolver_func) {
    ResolutionStatus result = lang_resolver->resolver_func(
        node, ref_type, qualified_name, resolver->symbol_table, lang_resolver->resolver_data);

    if (result == RESOLUTION_SUCCESS) {
      resolver->resolved_references++;
    }

    return result;
  }

  // Fallback to generic resolution
  ResolutionStatus result = reference_resolver_generic_resolve_impl(node, ref_type, qualified_name,
                                                                    resolver->symbol_table);

  if (result == RESOLUTION_SUCCESS) {
    resolver->resolved_references++;
  }

  return result;
}

/**
 * Resolve all references in a file
 */
size_t reference_resolver_resolve_file_impl(ReferenceResolver *resolver,
                                            ParserContext *file_context) {
  if (!resolver || !file_context) {
    return 0;
  }

  size_t resolved_count = 0;
  ASTNode *root = parser_context_get_ast(file_context);
  if (!root) {
    return 0;
  }

  // Create queue for BFS traversal
  ASTNode **queue = (ASTNode **)malloc(1000 * sizeof(ASTNode *));
  if (!queue) {
    return 0;
  }

  size_t front = 0;
  size_t rear = 0;
  queue[rear++] = root;

  // BFS to process all nodes
  while (front < rear) {
    ASTNode *current = queue[front++];

    // Process references in this node
    // Note: num_references_to_resolve and references_to_resolve fields don't exist in ASTNode
    // This will need to be implemented when we add reference tracking to ASTNode
    // For now, skip this step
    (void)current; // Suppress unused variable warning

    // Add children to queue
    for (size_t i = 0; i < current->num_children; i++) {
      if (rear < 1000) { // Prevent overflow
        queue[rear++] = current->children[i];
      }
    }
  }

  free(queue);
  LOG_DEBUG("Resolved %zu references in file %s", resolved_count, file_context->filename);
  return resolved_count;
}

/**
 * Resolve all references in a project
 */
size_t reference_resolver_resolve_all_impl(ReferenceResolver *resolver,
                                           ProjectContext *project_context) {
  if (!resolver || !project_context) {
    return 0;
  }

  size_t total_resolved = 0;
  size_t num_files = project_context_get_file_count(project_context);

  for (size_t i = 0; i < num_files; i++) {
    ParserContext *file_context = project_context_get_file_by_index(project_context, i);
    if (file_context) {
      total_resolved += reference_resolver_resolve_file_impl(resolver, file_context);
    }
  }

  LOG_DEBUG("Resolved %zu references total across %zu files", total_resolved, num_files);
  return total_resolved;
}

/**
 * Generic reference resolution algorithm
 */
ResolutionStatus reference_resolver_generic_resolve_impl(ASTNode *node, ReferenceType ref_type,
                                                         const char *name,
                                                         GlobalSymbolTable *symbol_table) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_ERROR;
  }

  // Try direct lookup first (for fully qualified names)
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry) {
    // Found an exact match
    // Add reference to node
    if (node->num_references < node->references_capacity) {
      node->references[node->num_references++] = entry->node;
      return RESOLUTION_SUCCESS;
    } else {
      // Resize references array
      size_t new_capacity = node->references_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 4;

      ASTNode **new_refs = (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

      if (new_refs) {
        node->references = new_refs;
        node->references_capacity = new_capacity;
        node->references[node->num_references++] = entry->node;
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // Try scope-aware resolution
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_UNKNOWN);
  if (entry) {
    // Found in scope
    // Add reference to node
    if (node->num_references < node->references_capacity) {
      node->references[node->num_references++] = entry->node;
      return RESOLUTION_SUCCESS;
    } else {
      // Resize references array
      size_t new_capacity = node->references_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 4;

      ASTNode **new_refs = (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

      if (new_refs) {
        node->references = new_refs;
        node->references_capacity = new_capacity;
        node->references[node->num_references++] = entry->node;
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // If we get here, resolution failed
  LOG_DEBUG("Failed to resolve reference '%s' in %s", name,
            node->qualified_name ? node->qualified_name : "unknown node");
  return RESOLUTION_NOT_FOUND;
}
