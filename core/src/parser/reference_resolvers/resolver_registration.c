/**
 * @file resolver_registration.c
 * @brief Registration of language-specific resolvers
 *
 * This file handles:
 * - Built-in resolver registration
 * - Finding appropriate resolvers for specific languages
 */

#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/reference_resolver_internal.h"

#include <stdlib.h>
#include <string.h>

// Forward declarations of language-specific resolvers
extern ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type,
                                             const char *name, GlobalSymbolTable *symbol_table,
                                             void *resolver_data);

extern ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type,
                                                  const char *name, GlobalSymbolTable *symbol_table,
                                                  void *resolver_data);

extern ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                                      const char *name,
                                                      GlobalSymbolTable *symbol_table,
                                                      void *resolver_data);

extern ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                                      const char *name,
                                                      GlobalSymbolTable *symbol_table,
                                                      void *resolver_data);

/**
 * Find the appropriate resolver for a language
 */
LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver, Language language);
