#pragma once

#include "../ast_node.h"
#include "../../../include/scopemux/language.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/symbol_table.h"

/**
 * Generic resolver used as a fallback by language-specific resolvers.
 * This provides basic symbol resolution functionality common to all languages.
 */
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table);

// Language-specific resolver function signatures
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
