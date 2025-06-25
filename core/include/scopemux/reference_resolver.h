/**
 * @file reference_resolver.h
 * @brief Cross-file reference resolution for ScopeMux
 *
 * This module provides the infrastructure for resolving references between
 * ASTNodes across different source files, including function calls, type
 * references, imports/includes, and inheritance relationships.
 *
 * The ReferenceResolver framework includes a language-agnostic core with
 * pluggable language-specific resolvers for handling the unique reference
 * patterns of each supported language.
 */

#ifndef SCOPEMUX_REFERENCE_RESOLVER_H
#define SCOPEMUX_REFERENCE_RESOLVER_H

#include "parser.h"
#include "project_context.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Types of cross-file relationships
 */
typedef enum {
    REF_UNKNOWN = 0,
    REF_CALL,              ///< Function/method call
    REF_TYPE,              ///< Type reference (variable declaration, parameter type, etc.)
    REF_INHERITANCE,       ///< Class/interface inheritance
    REF_IMPORT,            ///< Import/include relationship
    REF_IMPLEMENTATION,    ///< Interface implementation
    REF_OVERRIDE,          ///< Method override
    REF_USE,               ///< Variable/symbol usage
    REF_EXTENSION,         ///< Extension/trait application
    REF_TEMPLATE           ///< Template/generic instantiation
} ReferenceType;

/**
 * @brief Status codes for reference resolution
 */
typedef enum {
    RESOLVE_SUCCESS = 0,    ///< Reference successfully resolved
    RESOLVE_NOT_FOUND,      ///< Target symbol not found
    RESOLVE_AMBIGUOUS,      ///< Multiple potential targets found
    RESOLVE_CIRCULAR,       ///< Circular dependency detected
    RESOLVE_ERROR           ///< Other resolution error
} ResolutionStatus;

/**
 * @brief Metadata for a resolved reference
 */
typedef struct {
    ReferenceType type;     ///< Type of reference
    const char *source_file; ///< Source file of the reference
    const char *target_file; ///< Target file of the resolved symbol
    ResolutionStatus status; ///< Resolution status
    char *error_message;    ///< Error message if resolution failed (NULL otherwise)
} ReferenceMetadata;

/**
 * @brief Language-specific reference resolver
 * 
 * This structure contains function pointers for language-specific
 * reference resolution logic.
 */
typedef struct ReferenceResolver {
    LanguageType language;  ///< Language this resolver handles

    /**
     * @brief Resolve import/include statements in a file
     * 
     * @param ctx Parser context for the file
     * @param project Project context
     * @param table Global symbol table
     * @return bool True if all imports were successfully resolved
     */
    bool (*resolve_imports)(ParserContext *ctx, ProjectContext *project, 
                           GlobalSymbolTable *table);

    /**
     * @brief Resolve function/method calls in a file
     * 
     * @param ctx Parser context for the file
     * @param project Project context
     * @param table Global symbol table
     * @return bool True if all calls were successfully resolved
     */
    bool (*resolve_calls)(ParserContext *ctx, ProjectContext *project, 
                        GlobalSymbolTable *table);

    /**
     * @brief Resolve type references in a file
     * 
     * @param ctx Parser context for the file
     * @param project Project context
     * @param table Global symbol table
     * @return bool True if all type references were successfully resolved
     */
    bool (*resolve_types)(ParserContext *ctx, ProjectContext *project, 
                        GlobalSymbolTable *table);

    /**
     * @brief Resolve inheritance relationships in a file
     * 
     * @param ctx Parser context for the file
     * @param project Project context
     * @param table Global symbol table
     * @return bool True if all inheritance relationships were successfully resolved
     */
    bool (*resolve_inheritance)(ParserContext *ctx, ProjectContext *project, 
                             GlobalSymbolTable *table);

} ReferenceResolver;

/**
 * @brief Registry of language-specific reference resolvers
 */
typedef struct {
    ReferenceResolver **resolvers;  ///< Array of language-specific resolvers
    size_t num_resolvers;           ///< Number of registered resolvers
    size_t capacity;                ///< Capacity of resolvers array
} ResolverRegistry;

/**
 * @brief Create a new reference resolver registry
 * 
 * @param initial_capacity Initial capacity for the resolver array
 * @return ResolverRegistry* New resolver registry or NULL on failure
 */
ResolverRegistry *resolver_registry_create(size_t initial_capacity);

/**
 * @brief Free all resources associated with a resolver registry
 * 
 * @param registry Registry to free
 */
void resolver_registry_free(ResolverRegistry *registry);

/**
 * @brief Register a language-specific resolver
 * 
 * @param registry Resolver registry
 * @param resolver Reference resolver to register (will be copied)
 * @return bool True on success, false on failure
 */
bool resolver_registry_add(ResolverRegistry *registry, const ReferenceResolver *resolver);

/**
 * @brief Get a reference resolver for a specific language
 * 
 * @param registry Resolver registry
 * @param language Language to get resolver for
 * @return const ReferenceResolver* Matching resolver or NULL if not found
 */
const ReferenceResolver *resolver_registry_get(const ResolverRegistry *registry, 
                                            LanguageType language);

/**
 * @brief Initialize the built-in reference resolvers
 * 
 * This function registers the default resolvers for all supported languages.
 * 
 * @param registry Resolver registry
 * @return bool True if all built-in resolvers were registered successfully
 */
bool resolver_registry_init_defaults(ResolverRegistry *registry);

/**
 * @brief Resolve all cross-file references in a parser context
 * 
 * This function uses the appropriate language-specific resolver to
 * establish relationships between ASTNodes across different files.
 * 
 * @param ctx Parser context to resolve references for
 * @param project Project context
 * @param registry Resolver registry
 * @return bool True if all references were resolved successfully
 */
bool resolve_cross_file_references(ParserContext *ctx, ProjectContext *project,
                                 const ResolverRegistry *registry);

/**
 * @brief Add a reference between two ASTNodes with metadata
 * 
 * @param from Source node
 * @param to Target node
 * @param ref_type Type of reference
 * @return bool True on success, false on failure
 */
bool ast_node_add_reference_with_metadata(ASTNode *from, ASTNode *to, ReferenceType ref_type);

/**
 * @brief Get reference metadata for a relationship between two nodes
 * 
 * @param from Source node
 * @param to Target node
 * @return ReferenceMetadata* Metadata or NULL if no relationship exists
 */
const ReferenceMetadata *ast_node_get_reference_metadata(const ASTNode *from, const ASTNode *to);

/**
 * @brief Resolve a symbol reference using scope-aware lookup
 * 
 * This function attempts to resolve a reference by:
 * 1. Exact match of qualified name
 * 2. Relative to the current namespace/scope
 * 3. Implicit imports/includes
 * 
 * @param name Symbol name (may be unqualified)
 * @param current_node Current node providing context
 * @param ctx Parser context
 * @param project Project context
 * @param ref_type Type of reference to resolve
 * @return ASTNode* Resolved target node or NULL if not found
 */
ASTNode *resolve_symbol_reference(const char *name, const ASTNode *current_node,
                               ParserContext *ctx, ProjectContext *project,
                               ReferenceType ref_type);

/**
 * @brief C/C++ specific reference resolver
 */
extern const ReferenceResolver C_CPP_REFERENCE_RESOLVER;

/**
 * @brief Python specific reference resolver
 */
extern const ReferenceResolver PYTHON_REFERENCE_RESOLVER;

/**
 * @brief JavaScript/TypeScript specific reference resolver
 */
extern const ReferenceResolver JS_TS_REFERENCE_RESOLVER;

#endif /* SCOPEMUX_REFERENCE_RESOLVER_H */
