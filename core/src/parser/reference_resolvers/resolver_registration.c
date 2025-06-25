/**
 * @file resolver_registration.c
 * @brief Registration of language-specific resolvers
 * 
 * This file handles:
 * - Built-in resolver registration
 * - Finding appropriate resolvers for specific languages
 */

#include "scopemux/reference_resolver.h"
#include "scopemux/logging.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations of language-specific resolvers
extern ResolutionResult reference_resolver_c(ASTNode *node, ReferenceType ref_type,
                                          const char *name, GlobalSymbolTable *symbol_table,
                                          void *resolver_data);

extern ResolutionResult reference_resolver_python(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

extern ResolutionResult reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                                   const char *name, GlobalSymbolTable *symbol_table,
                                                   void *resolver_data);

extern ResolutionResult reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                                   const char *name, GlobalSymbolTable *symbol_table,
                                                   void *resolver_data);

/**
 * Find the appropriate resolver for a language
 */
LanguageResolver *find_language_resolver_impl(ReferenceResolver *resolver, LanguageType language) {
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
 * Initialize built-in resolvers for all supported languages
 */
bool reference_resolver_init_builtin_impl(ReferenceResolver *resolver) {
    if (!resolver) {
        return false;
    }
    
    // Register C resolver
    bool success = reference_resolver_register_impl(
        resolver, LANG_C, reference_resolver_c, NULL, NULL);
    if (!success) {
        LOG_ERROR("Failed to register C resolver");
        return false;
    }
    
    // Register Python resolver
    success = reference_resolver_register_impl(
        resolver, LANG_PYTHON, reference_resolver_python, NULL, NULL);
    if (!success) {
        LOG_ERROR("Failed to register Python resolver");
        return false;
    }
    
    // Register JavaScript resolver
    success = reference_resolver_register_impl(
        resolver, LANG_JAVASCRIPT, reference_resolver_javascript, NULL, NULL);
    if (!success) {
        LOG_ERROR("Failed to register JavaScript resolver");
        return false;
    }
    
    // Register TypeScript resolver
    success = reference_resolver_register_impl(
        resolver, LANG_TYPESCRIPT, reference_resolver_typescript, NULL, NULL);
    if (!success) {
        LOG_ERROR("Failed to register TypeScript resolver");
        return false;
    }
    
    LOG_DEBUG("Successfully registered all built-in language resolvers");
    return true;
}
