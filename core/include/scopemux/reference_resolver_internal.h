#ifndef SCOPEMUX_REFERENCE_RESOLVER_INTERNAL_H
#define SCOPEMUX_REFERENCE_RESOLVER_INTERNAL_H

#include "scopemux/reference_resolver.h"

// Expose the global resolver_registry for internal use
extern ResolverRegistry resolver_registry;

ResolutionStatus reference_resolver_resolve_node_impl(ReferenceResolver *resolver, ASTNode *node,
                                                      ReferenceType ref_type,
                                                      const char *qualified_name,
                                                      Language language);

ResolutionStatus reference_resolver_generic_resolve_impl(ASTNode *node, ReferenceType ref_type,
                                                         const char *qualified_name,
                                                         GlobalSymbolTable *symbol_table);

ReferenceResolver *reference_resolver_create_impl(GlobalSymbolTable *symbol_table);
bool reference_resolver_init_builtin_impl(ReferenceResolver *resolver);
void reference_resolver_free_impl(ReferenceResolver *resolver);
void reference_resolver_get_stats_impl(const ReferenceResolver *resolver,
                                       size_t *out_total_references,
                                       size_t *out_resolved_references);
bool reference_resolver_unregister_impl(ReferenceResolver *resolver, Language language);
LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver, Language language);
ResolutionStatus reference_resolver_resolve_file_impl(ReferenceResolver *resolver,
                                                      ParserContext *file_context);
ResolutionStatus reference_resolver_resolve_all_impl(ReferenceResolver *resolver,
                                                     ProjectContext *project_context);

#endif // SCOPEMUX_REFERENCE_RESOLVER_INTERNAL_H
