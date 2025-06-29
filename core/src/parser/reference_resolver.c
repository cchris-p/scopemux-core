/**
 * @file reference_resolver.c
 * @brief Implementation of cross-file reference resolution
 *
 * This file serves as the main entry point for the reference resolution infrastructure,
 * delegating to specialized components in the reference_resolvers/ directory:
 *
 * - resolver_core.c: Core resolver lifecycle management
 * - resolver_registration.c: Language resolver registration
 * - resolver_resolution.c: Resolution algorithms
 * - language_resolvers.c: Language-specific implementations
 */

#include "scopemux/reference_resolver.h"
#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of implementation functions in modular components

// From resolver_core.c
extern ReferenceResolver *reference_resolver_create_impl(GlobalSymbolTable *symbol_table);
extern void reference_resolver_free_impl(ReferenceResolver *resolver);
extern bool reference_resolver_register_impl(ReferenceResolver *resolver, LanguageType language,
                                             ResolverFunction resolver_func, void *resolver_data,
                                             ResolverCleanupFunction cleanup_func);
extern bool reference_resolver_unregister_impl(ReferenceResolver *resolver, LanguageType language);
extern void reference_resolver_get_stats_impl(const ReferenceResolver *resolver,
                                              size_t *out_total_references,
                                              size_t *out_resolved_references,
                                              size_t *out_unresolved_references);

// From resolver_registration.c
extern LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver,
                                                     LanguageType language);
extern bool reference_resolver_init_builtin_impl(ReferenceResolver *resolver);

// From resolver_resolution.c
extern ResolutionStatus reference_resolver_resolve_node_impl(ReferenceResolver *resolver,
                                                             ASTNode *node, ReferenceType ref_type,
                                                             const char *qualified_name);
extern size_t reference_resolver_resolve_file_impl(ReferenceResolver *resolver,
                                                   ParserContext *file_context);
extern size_t reference_resolver_resolve_all_impl(ReferenceResolver *resolver,
                                                  ProjectContext *project_context);
extern ResolutionStatus reference_resolver_generic_resolve_impl(ASTNode *node,
                                                                ReferenceType ref_type,
                                                                const char *name,
                                                                GlobalSymbolTable *symbol_table);

/**
 * Create a new reference resolver
 *
 * @param symbol_table The global symbol table to use for resolution
 * @return A newly allocated reference resolver, or NULL on failure
 *
 * @see resolver_core.c for implementation
 */
ReferenceResolver *reference_resolver_create(GlobalSymbolTable *symbol_table) {
  return reference_resolver_create_impl(symbol_table);
}

/**
 * Free all resources associated with a reference resolver
 *
 * @param resolver The reference resolver to free
 *
 * @see resolver_core.c for implementation
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
 *
 * @see resolver_core.c for implementation
 */
bool reference_resolver_register(ReferenceResolver *resolver, LanguageType language,
                                 ResolverFunction resolver_func, void *resolver_data,
                                 ResolverCleanupFunction cleanup_func) {
  return reference_resolver_register_impl(resolver, language, resolver_func, resolver_data,
                                          cleanup_func);
}

/**
 * Unregister a language-specific resolver
 *
 * @param resolver The reference resolver
 * @param language The language type to unregister
 * @return true if unregistration succeeded, false otherwise
 *
 * @see resolver_core.c for implementation
 */
bool reference_resolver_unregister(ReferenceResolver *resolver, LanguageType language) {
  return reference_resolver_unregister_impl(resolver, language);
}

/**
 * Find the appropriate resolver for a language
 *
 * @param resolver The reference resolver
 * @param language The language type to find a resolver for
 * @return Pointer to the language resolver, or NULL if not found
 *
 * @see resolver_registration.c for implementation
 */
LanguageResolver *find_language_resolver(ReferenceResolver *resolver, LanguageType language) {
  return find_language_resolver_impl(resolver, language);
}

/**
 * Resolve a reference in a specific node
 *
 * @param resolver The reference resolver
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param qualified_name The qualified name to resolve
 * @return Resolution status
 *
 * @see resolver_resolution.c for implementation
 */
ResolutionStatus reference_resolver_resolve_node(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type,
                                                 const char *qualified_name) {
  return reference_resolver_resolve_node_impl(resolver, node, ref_type, qualified_name);
}

/**
 * Resolve all references in a file
 *
 * @param resolver The reference resolver
 * @param file_context The parser context for the file
 * @return Number of resolved references
 *
 * @see resolver_resolution.c for implementation
 */
size_t reference_resolver_resolve_file(ReferenceResolver *resolver, ParserContext *file_context) {
  return reference_resolver_resolve_file_impl(resolver, file_context);
}

/**
 * Resolve all references in a project
 *
 * @param resolver The reference resolver
 * @param project_context The project context containing all files
 * @return Number of resolved references across the entire project
 *
 * @see resolver_resolution.c for implementation
 */
size_t reference_resolver_resolve_all(ReferenceResolver *resolver,
                                      ProjectContext *project_context) {
  return reference_resolver_resolve_all_impl(resolver, project_context);
}

/**
 * Generic reference resolution algorithm
 *
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param name The name to resolve
 * @param symbol_table The symbol table to look up in
 * @return Resolution status
 *
 * @see resolver_resolution.c for implementation
 */
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table) {
  return reference_resolver_generic_resolve_impl(node, ref_type, name, symbol_table);
}

/**
 * Initialize built-in resolvers for all supported languages
 *
 * @param resolver The reference resolver to initialize
 * @return True if all built-in resolvers were successfully registered
 *
 * @see resolver_registration.c for implementation
 */
bool reference_resolver_init_builtin(ReferenceResolver *resolver) {
  return reference_resolver_init_builtin_impl(resolver);
}

/**
 * C language resolver implementation
 *
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param name The name of the reference to resolve
 * @param symbol_table The symbol table to look up in
 * @param resolver_data Optional C-specific resolver data
 * @return Resolution status
 *
 * @see language_resolvers.c for implementation
 */
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data);

/**
 * Python language resolver implementation
 *
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param name The name of the reference to resolve
 * @param symbol_table The symbol table to look up in
 * @param resolver_data Optional Python-specific resolver data
 * @return Resolution status
 *
 * @see language_resolvers.c for implementation
 */
ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data);

/**
 * JavaScript language resolver implementation
 *
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param name The name of the reference to resolve
 * @param symbol_table The symbol table to look up in
 * @param resolver_data Optional JavaScript-specific resolver data
 * @return Resolution status
 *
 * @see language_resolvers.c for implementation
 */
ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

/**
 * TypeScript language resolver implementation
 *
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param name The name of the reference to resolve
 * @param symbol_table The symbol table to look up in
 * @param resolver_data Optional TypeScript-specific resolver data
 * @return Resolution status
 *
 * @see language_resolvers.c for implementation
 */
ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

/**
 * Get resolver statistics
 *
 * @param resolver The reference resolver
 * @param out_total_references Optional output for total references
 * @param out_resolved_references Optional output for resolved references
 * @param out_unresolved_references Optional output for unresolved references
 *
 * @see resolver_core.c for implementation
 */
void reference_resolver_get_stats(const ReferenceResolver *resolver, size_t *out_total_references,
                                  size_t *out_resolved_references,
                                  size_t *out_unresolved_references) {
  reference_resolver_get_stats_impl(resolver, out_total_references, out_resolved_references,
                                    out_unresolved_references);
}
