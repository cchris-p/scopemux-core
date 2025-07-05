/**
 * @file resolver_implementation.c
 * @brief Implementation of reference resolver functions
 */

#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/reference_resolver_internal.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LANGUAGE_RESOLVERS 16

// Forward declarations for functions used in this file
const ASTNode *parser_get_ast_root(const ParserContext *parser_context);
size_t project_context_get_file_count(const ProjectContext *project_context);
ParserContext *project_context_get_file_by_index(const ProjectContext *project_context, size_t i);

// Function declaration for the implementation

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

  // Free language resolvers
  if (resolver->language_resolvers) {
    free(resolver->language_resolvers);
  }

  // Free the resolver itself
  free(resolver);
}

/**
 * Register a language-specific resolver
 */
bool reference_resolver_register_impl(ReferenceResolver *resolver, Language language,
                                      ResolverFunction func, void *data,
                                      ResolverCleanupFunction cleanup) {
  if (!resolver || !func) {
    return false;
  }

  // Check if we already have a resolver for this language
  for (size_t i = 0; i < resolver->num_resolvers; i++) {
    if (resolver->language_resolvers[i].language == language) {
      // Update existing resolver
      if (resolver->language_resolvers[i].cleanup_func) {
        resolver->language_resolvers[i].cleanup_func(resolver->language_resolvers[i].resolver_data);
      }
      resolver->language_resolvers[i].resolver_func = func;
      resolver->language_resolvers[i].resolver_data = data;
      resolver->language_resolvers[i].cleanup_func = cleanup;
      return true;
    }
  }

  // Add new resolver if we have space
  if (resolver->num_resolvers >= MAX_LANGUAGE_RESOLVERS) {
    return false;
  }

  // Add the new resolver
  resolver->language_resolvers[resolver->num_resolvers].language = language;
  resolver->language_resolvers[resolver->num_resolvers].resolver_func = func;
  resolver->language_resolvers[resolver->num_resolvers].resolver_data = data;
  resolver->language_resolvers[resolver->num_resolvers].cleanup_func = cleanup;
  resolver->num_resolvers++;

  return true;
}

/**
 * Unregister a language-specific resolver
 */
bool reference_resolver_unregister_impl(ReferenceResolver *resolver, Language language) {
  if (!resolver) {
    return false;
  }

  // Find and remove the resolver
  for (size_t i = 0; i < resolver->num_resolvers; i++) {
    if (resolver->language_resolvers[i].language == language) {
      // Call cleanup function if provided
      if (resolver->language_resolvers[i].cleanup_func) {
        resolver->language_resolvers[i].cleanup_func(resolver->language_resolvers[i].resolver_data);
      }

      // Shift remaining resolvers down
      if (i < resolver->num_resolvers - 1) {
        memmove(&resolver->language_resolvers[i], &resolver->language_resolvers[i + 1],
                (resolver->num_resolvers - i - 1) * sizeof(LanguageResolver));
      }

      resolver->num_resolvers--;
      return true;
    }
  }

  return false;
}

/**
 * Find the appropriate resolver for a language
 */
LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver, Language language) {
  if (!resolver) {
    return NULL;
  }

  for (size_t i = 0; i < resolver->num_resolvers; i++) {
    if (resolver->language_resolvers[i].language == language) {
      return &resolver->language_resolvers[i];
    }
  }

  return NULL;
}

/**
 * Resolve a reference in a specific node
 */
ResolutionStatus reference_resolver_resolve_node_impl(ReferenceResolver *resolver, ASTNode *node,
                                                      ReferenceType ref_type,
                                                      const char *qualified_name,
                                                      Language language) {
  if (!resolver || !node || !qualified_name) {
    return RESOLUTION_ERROR;
  }

  // Track statistics
  resolver->total_references++;

  // Try language-specific resolver first
  LanguageResolver *lang_resolver = find_language_resolver_impl(resolver, node->lang);
  if (lang_resolver && lang_resolver->resolver_func) {
    // Ensure proper type conversion for ref_type parameter
    ResolutionStatus result =
        lang_resolver->resolver_func(node, (ReferenceType)ref_type, qualified_name,
                                     resolver->symbol_table, lang_resolver->resolver_data);

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
ResolutionStatus reference_resolver_resolve_file_impl(ReferenceResolver *resolver,
                                                      ParserContext *file_context) {
  if (!resolver || !file_context) {
    return RESOLUTION_ERROR;
  }

  ResolutionStatus overall_status = RESOLUTION_SUCCESS;
  // Use the correct function to get the AST root
  // Cast away const as we need to modify the AST during reference resolution
  ASTNode *root = (ASTNode *)parser_get_ast_root(file_context);
  if (!root) {
    return RESOLUTION_ERROR;
  }

  // Create queue for BFS traversal
  ASTNode **queue = (ASTNode **)malloc(1000 * sizeof(ASTNode *));
  if (!queue) {
    return RESOLUTION_ERROR;
  }

  size_t front = 0;
  size_t rear = 0;
  queue[rear++] = root;

  // BFS to process all nodes
  while (front < rear) {
    ASTNode *current = queue[front++];

    // Process references in this node if it has any
    if (current->num_references > 0) {
      for (size_t i = 0; i < current->num_references; i++) {
        ResolutionStatus status = reference_resolver_resolve_node_impl(
            resolver, current, REF_UNKNOWN, current->references[i]->qualified_name, current->lang);
        if (status != RESOLUTION_SUCCESS) {
          overall_status = status;
        }
      }
    }

    // Add children to queue
    for (size_t i = 0; i < current->num_children; i++) {
      if (rear < 1000) { // Prevent overflow
        queue[rear++] = current->children[i];
      }
    }
  }

  free(queue);
  return overall_status;
}

/**
 * Resolve all references in a project
 */
ResolutionStatus reference_resolver_resolve_all_impl(ReferenceResolver *resolver,
                                                     ProjectContext *project_context) {
  if (!resolver || !project_context) {
    return RESOLUTION_ERROR;
  }

  ResolutionStatus overall_status = RESOLUTION_SUCCESS;
  size_t num_files = project_context_get_file_count(project_context);

  for (size_t i = 0; i < num_files; i++) {
    ParserContext *file_context = project_context_get_file_by_index(project_context, i);
    if (file_context) {
      ResolutionStatus status = reference_resolver_resolve_file_impl(resolver, file_context);
      if (status != RESOLUTION_SUCCESS) {
        overall_status = status;
      }
    }
  }

  return overall_status;
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

  return RESOLUTION_NOT_FOUND;
}

/**
 * Initialize built-in resolvers
 */
bool reference_resolver_init_builtin_impl(ReferenceResolver *resolver) {
  if (!resolver) {
    return false;
  }

  // Register built-in resolvers for each language
  bool success = true;

  // C language resolver
  success &= reference_resolver_register_impl(resolver, LANG_C, NULL, NULL, NULL);

  // Python language resolver
  success &= reference_resolver_register_impl(resolver, LANG_PYTHON, NULL, NULL, NULL);

  // JavaScript language resolver
  success &= reference_resolver_register_impl(resolver, LANG_JAVASCRIPT, NULL, NULL, NULL);

  // TypeScript language resolver
  success &= reference_resolver_register_impl(resolver, LANG_TYPESCRIPT, NULL, NULL, NULL);

  return success;
}

/**
 * Get resolver statistics
 */
void reference_resolver_get_stats_impl(const ReferenceResolver *resolver,
                                       size_t *out_total_references,
                                       size_t *out_resolved_references) {
  if (!resolver) {
    if (out_total_references)
      *out_total_references = 0;
    if (out_resolved_references)
      *out_resolved_references = 0;
    return;
  }

  if (out_total_references)
    *out_total_references = resolver->total_references;
  if (out_resolved_references)
    *out_resolved_references = resolver->resolved_references;
}
