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

#include <stdbool.h>
#include <stddef.h>

// Include necessary headers for full definitions
#include "ast.h"
#include "parser.h"
#include "project_context.h"
#include "source_range.h"
#include "symbol_table.h"

/**
 * @brief Types of cross-file relationships
 */
typedef enum {
  REF_UNKNOWN = 0,
  REF_CALL,           ///< Function/method call
  REF_TYPE,           ///< Type reference (variable declaration, parameter type, etc.)
  REF_INHERITANCE,    ///< Class/interface inheritance
  REF_IMPORT,         ///< Import/include relationship
  REF_IMPLEMENTATION, ///< Interface implementation
  REF_OVERRIDE,       ///< Method override
  REF_USE,            ///< Variable/symbol usage
  REF_EXTENSION,      ///< Extension/trait application
  REF_TEMPLATE,       ///< Template/generic instantiation
  REF_INTERFACE,      ///< Interface reference
  REF_GENERIC,        ///< Generic type reference
  REF_INCLUDE,        ///< Include directive (duplicate of REF_IMPORT but for clarity)
  REF_PROPERTY,       ///< Property/field access
  REF_NODE_TYPE       ///< Reference to a type node (may overlap with REF_TYPE)
} ReferenceType;

/**
 * @brief Status codes for reference resolution
 */
typedef enum {
  RESOLUTION_SUCCESS = 0,   ///< Reference successfully resolved
  RESOLUTION_NOT_FOUND,     ///< Target symbol not found
  RESOLUTION_AMBIGUOUS,     ///< Multiple potential targets found
  RESOLUTION_CIRCULAR,      ///< Circular dependency detected
  RESOLUTION_ERROR,         ///< Other resolution error
  RESOLUTION_NOT_SUPPORTED, ///< Resolution mechanism not supported for this type
  RESOLUTION_FAILED         ///< Resolution failed for another reason
} ResolutionStatus;

/**
 * @brief Metadata for a resolved reference
 */
typedef struct {
  ReferenceType type;      ///< Type of reference
  const char *source_file; ///< Source file of the reference
  const char *target_file; ///< Target file of the resolved symbol
  ResolutionStatus status; ///< Resolution status
  char *error_message;     ///< Error message if resolution failed (NULL otherwise)
} ReferenceMetadata;

/**
 * @brief Language-specific resolver function type
 *
 * @param node Node containing the reference
 * @param ref_type Type of reference to resolve
 * @param name Name of the symbol to resolve
 * @param symbol_table Global symbol table
 * @param resolver_data Language-specific resolver data
 * @return ResolutionStatus indicating the success or failure
 */
typedef ResolutionStatus (*ResolverFunction)(ASTNode *node, ReferenceType ref_type,
                                             const char *name, GlobalSymbolTable *symbol_table,
                                             void *resolver_data);

/**
 * @brief Cleanup function type for language-specific resolver data
 *
 * @param resolver_data The data to clean up
 */
typedef void (*ResolverCleanupFunction)(void *resolver_data);

/**
 * @brief Language-specific resolver
 */
typedef struct {
  Language language;                    ///< Language this resolver handles
  ResolverFunction resolver_func;       ///< The resolution function
  void *resolver_data;                  ///< Language-specific resolver data
  ResolverCleanupFunction cleanup_func; ///< Function to clean up resolver data
} LanguageResolver;

/**
 * @brief Main reference resolver
 *
 * This structure contains the core resolver infrastructure and statistics
 */
typedef struct ReferenceResolver {
  GlobalSymbolTable *symbol_table;      ///< Symbol table for lookups
  LanguageResolver *language_resolvers; ///< Array of language-specific resolvers
  size_t num_resolvers;                 ///< Number of registered resolvers

  // Statistics
  size_t total_references;    ///< Total references encountered
  size_t resolved_references; ///< Successfully resolved references
} ReferenceResolver;

/**
 * @brief Registry of language-specific reference resolvers
 */
typedef struct {
  LanguageResolver **resolvers; ///< Array of language-specific resolvers
  size_t num_resolvers;         ///< Number of registered resolvers
  size_t capacity;              ///< Capacity of resolvers array
} ResolverRegistry;

/**
 * @brief Create a new reference resolver registry
 *
 * @param initial_capacity Initial capacity for the resolver array
 * @return ResolverRegistry* New resolver registry or NULL on failure
 */
ResolverRegistry *resolver_registry_create(size_t initial_capacity);

void resolver_registry_free(ResolverRegistry *registry);

bool resolver_registry_add(ResolverRegistry *registry, const LanguageResolver *resolver);

const LanguageResolver *resolver_registry_get(const ResolverRegistry *registry, Language language);

bool resolver_registry_init_defaults(ResolverRegistry *registry);

bool resolve_cross_file_references(ParserContext *ctx, ProjectContext *project,
                                   ReferenceResolver *resolver);

bool ast_node_add_reference_with_metadata(ASTNode *from, ASTNode *to, ReferenceType ref_type);

const ReferenceMetadata *ast_node_get_reference_metadata(const ASTNode *from, const ASTNode *to);

ReferenceResolver *reference_resolver_create(GlobalSymbolTable *symbol_table);

void reference_resolver_free(ReferenceResolver *resolver);

bool reference_resolver_register(Language language, ResolverFunction resolver_func,
                                 void *resolver_data, ResolverCleanupFunction cleanup_func);

bool reference_resolver_init_builtin(ReferenceResolver *resolver);

/**
 * @brief Resolve a reference in a specific node
 *
 * @param resolver The reference resolver
 * @param node The AST node containing the reference
 * @param ref_type The type of reference
 * @param qualified_name The qualified name to resolve
 * @param language The language of the reference
 * @return Resolution status
 */
ResolutionStatus reference_resolver_resolve_node(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type, const char *qualified_name,
                                                 Language language);

/**
 * @brief Resolve all references in a file
 *
 * @param resolver The reference resolver
 * @param file_context The parser context for the file
 * @return Number of resolved references
 */
size_t reference_resolver_resolve_file(ReferenceResolver *resolver, ParserContext *file_context);

/**
 * @brief Resolve all references in a project
 *
 * @param resolver The reference resolver
 * @param project_context The project context containing all files
 * @return Number of resolved references across the entire project
 */
size_t reference_resolver_resolve_all(ReferenceResolver *resolver, ProjectContext *project_context);

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
 * @param symbol_table Global symbol table
 * @return ASTNode* Resolved target node or NULL if not found
 */
ASTNode *resolve_symbol_reference(const char *name, const ASTNode *current_node, ParserContext *ctx,
                                  ProjectContext *project, ReferenceType ref_type,
                                  GlobalSymbolTable *symbol_table);

/**
 * @brief Functions for creating language-specific reference resolvers
 */
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data);

ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data);

ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

/**
 * @brief Create a reference resolver for a given node
 *
 * @param node The node to create a resolver for
 * @param ref_type The type of reference to resolve
 * @param name The name of the reference
 * @param symbol_table The global symbol table
 * @return ResolutionStatus indicating the result
 */
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table);

#endif /* SCOPEMUX_REFERENCE_RESOLVER_H */
