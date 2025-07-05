/**
 * @file reference_resolver_facade.c
 * @brief Implementation of the reference resolver facade functions
 *
 * This file implements the main facade functions for the reference resolver
 * that are exposed in the public API but were missing from the implementation.
 * These functions delegate to the appropriate implementation functions.
 */

#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/reference_resolver_internal.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Create and initialize the reference resolver with built-in resolvers
 *
 * This function creates a new reference resolver and initializes it
 * with built-in resolvers for all supported languages.
 *
 * @param symbol_table The global symbol table to use for resolution
 * @return A newly initialized reference resolver, or NULL on failure
 */
ReferenceResolver *reference_resolver_create_and_init(GlobalSymbolTable *symbol_table) {
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
 * @param language The language of the node
 * @return Resolution status
 */
ResolutionStatus reference_resolver_resolve(ReferenceResolver *resolver, ASTNode *node,
                                            ReferenceType ref_type, const char *qualified_name,
                                            Language language) {
  return reference_resolver_resolve_node_impl(resolver, node, ref_type, qualified_name, language);
}

/**
 * Add a reference with metadata to an AST node
 *
 * This function adds a reference with metadata to an AST node.
 * It is used by reference resolvers to establish relationships
 * between nodes with additional semantic information.
 *
 * @param from The source AST node
 * @param to The target AST node to reference
 * @param ref_type The type of reference
 * @param relationship_type The type of relationship (e.g., "calls", "inherits", etc.)
 * @return true if the reference was added, false otherwise
 */
bool ast_node_add_reference_with_metadata(ASTNode *from, ASTNode *to, ReferenceType ref_type) {
  if (!from || !to) {
    return false;
  }

  if (from->num_references >= from->references_capacity) {
    // Need to resize references array
    size_t new_capacity = from->references_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4;
    }
    ASTNode **new_refs = (ASTNode **)realloc(from->references, new_capacity * sizeof(ASTNode *));
    if (!new_refs) {
      return false;
    }
    from->references = new_refs;
    from->references_capacity = new_capacity;
  }

  from->references[from->num_references++] = to;

  // Optionally store ref_type metadata if needed
  // TODO: Implement metadata storage if required
  log_debug("Added reference from %s to %s with type %d", from->name ? from->name : "(unnamed)",
            to->name ? to->name : "(unnamed)", (int)ref_type);

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
 */
void reference_resolver_get_stats(const ReferenceResolver *resolver, size_t *out_total_references,
                                  size_t *out_resolved_references) {
  reference_resolver_get_stats_impl(resolver, out_total_references, out_resolved_references);
}

/**
 * Create a new reference resolver
 *
 * @param symbol_table The global symbol table to use for resolution
 * @return A newly allocated reference resolver, or NULL on failure
 */
ReferenceResolver *reference_resolver_create(GlobalSymbolTable *symbol_table) {
  return reference_resolver_create_impl(symbol_table);
}

/**
 * Free all resources associated with a reference resolver
 *
 * @param resolver The reference resolver to free
 */
void reference_resolver_free(ReferenceResolver *resolver) {
  reference_resolver_free_impl(resolver);
}

/**
 * Register a language-specific resolver
 *
 * @param resolver The reference resolver
 * @param language The language type to register for
 * @param resolver_func The function to call for resolving references
 * @param resolver_data Optional data for the resolver function
 * @param cleanup_func Optional function to clean up resolver data
 * @return true if registration succeeded, false otherwise
 */
bool reference_resolver_register(Language language, ResolverFunction resolver_func,
                                 void *resolver_data, ResolverCleanupFunction cleanup_func) {
  // Directly call the correct (public) registration function or implement as needed
  // If reference_resolver_register_impl is no longer valid, use the registry API
  return reference_resolver_register(language, resolver_func, resolver_data, cleanup_func);
}

/**
 * Unregister a language-specific resolver
 *
 * @param resolver The reference resolver
 * @param language The language type to unregister
 * @return true if unregistration succeeded, false otherwise
 */
bool reference_resolver_unregister(ReferenceResolver *resolver, Language language) {
  return reference_resolver_unregister_impl(resolver, language);
}

/**
 * Find a language-specific resolver
 *
 * @param resolver The reference resolver
 * @param language The language to find a resolver for
 * @return LanguageResolver* The language-specific resolver or NULL if not found
 */
LanguageResolver *find_language_resolver(ReferenceResolver *resolver, Language language) {
  return find_language_resolver_impl(resolver, language);
}

/**
 * Initialize built-in resolvers for all supported languages
 *
 * @param resolver The reference resolver to initialize
 * @return True if all built-in resolvers were successfully registered
 */
bool reference_resolver_init_builtin(ReferenceResolver *resolver) {
  return reference_resolver_init_builtin_impl(resolver);
}

/**
 * Resolve a reference in a specific node
 *
 * @param resolver The reference resolver
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param qualified_name The qualified name to resolve
 * @param language The language of the node
 * @return Resolution status
 */
ResolutionStatus reference_resolver_resolve_node(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type, const char *qualified_name,
                                                 Language language) {
  return reference_resolver_resolve_node_impl(resolver, node, ref_type, qualified_name, language);
}

/**
 * Resolve all references in a file
 *
 * @param resolver The reference resolver
 * @param file_context The parser context for the file
 * @return Number of references resolved
 */
size_t reference_resolver_resolve_file(ReferenceResolver *resolver, ParserContext *file_context) {
  return reference_resolver_resolve_file_impl(resolver, file_context);
}

/**
 * Resolve all references in a project
 *
 * @param resolver The reference resolver
 * @param project_context The project context
 * @return Number of references resolved
 */
size_t reference_resolver_resolve_all(ReferenceResolver *resolver,
                                      ProjectContext *project_context) {
  return reference_resolver_resolve_all_impl(resolver, project_context);
}

/**
 * Generic reference resolution function
 *
 * This function provides a language-agnostic way to resolve references
 * based on simple name matching. It's useful as a fallback when
 * language-specific resolution fails or isn't available.
 *
 * @param resolver The reference resolver
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param name The name to resolve (may be partially qualified)
 * @return Resolution status
 */
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table) {
  return reference_resolver_generic_resolve_impl(node, ref_type, name, symbol_table);
}
