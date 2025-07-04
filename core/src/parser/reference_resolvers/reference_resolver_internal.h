/**
 * @file reference_resolver_internal.h
 * @brief Internal coordination header for reference resolver implementation modules
 *
 * This header provides internal function declarations for the reference resolver
 * implementation modules to coordinate between specialized components.
 * It is not part of the public API and should only be included by
 * reference resolver implementation files.
 */

#ifndef REFERENCE_RESOLVER_INTERNAL_H
#define REFERENCE_RESOLVER_INTERNAL_H

#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <stdbool.h>
#include <stddef.h>

// Forward declarations from resolver_core.c
ReferenceResolver *reference_resolver_create_impl(GlobalSymbolTable *symbol_table);
void reference_resolver_free_impl(ReferenceResolver *resolver);
bool reference_resolver_register_impl(ReferenceResolver *resolver, Language language,
                                      ResolverFunction resolver_func, void *resolver_data,
                                      ResolverCleanupFunction cleanup_func);
bool reference_resolver_unregister_impl(ReferenceResolver *resolver, Language language);
void reference_resolver_get_stats_impl(const ReferenceResolver *resolver,
                                       size_t *out_total_references,
                                       size_t *out_resolved_references,
                                       size_t *out_unresolved_references);

// Forward declarations from resolver_registration.c
LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver, Language language);
bool reference_resolver_init_builtin_impl(ReferenceResolver *resolver);

// Forward declarations from resolver_resolution.c
ResolutionStatus reference_resolver_resolve_node_impl(ReferenceResolver *resolver, ASTNode *node,
                                                      ReferenceType ref_type,
                                                      const char *qualified_name);
size_t reference_resolver_resolve_file_impl(ReferenceResolver *resolver,
                                            ParserContext *file_context);
size_t reference_resolver_resolve_all_impl(ReferenceResolver *resolver,
                                           ProjectContext *project_context);
ResolutionStatus reference_resolver_generic_resolve_impl(ASTNode *node, ReferenceType ref_type,
                                                         const char *name,
                                                         GlobalSymbolTable *symbol_table);

// Language-specific resolver declarations
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

// Helper function for adding references with metadata
bool ast_node_add_reference_with_metadata(ASTNode *node, ASTNode *target,
                                          const char *relationship_type);

#endif /* REFERENCE_RESOLVER_INTERNAL_H */
