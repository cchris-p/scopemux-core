/**
 * @file reference_resolver_private.h
 * @brief Private interface for reference resolver testing
 *
 * This header defines the internal structures used by the reference resolver
 * implementation to facilitate testing. These structures should not be used
 * directly by client code outside of the test suite.
 */

#ifndef SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H
#define SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H

#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reference resolver API function signatures
// Forward declarations matching the public API
ReferenceResolver *reference_resolver_create(GlobalSymbolTable *symbol_table);
void reference_resolver_free(ReferenceResolver *resolver);
void reference_resolver_init_builtin(ReferenceResolver *resolver);

// The real signatures for the resolve functions (based on the original API)
typedef ResolutionStatus (*LanguageResolverFunc)(ASTNode *node, ReferenceType ref_type,
                                                 const char *name, GlobalSymbolTable *symbol_table,
                                                 void *resolver_data);
typedef void (*ResolverCleanupFunc)(void *resolver_data);

// We're using the opaque LanguageResolver type from the headers
// No need to redefine it here

// Register and unregister language resolvers
bool reference_resolver_register(ReferenceResolver *resolver, LanguageType language,
                                 LanguageResolverFunc resolver_func, void *resolver_data,
                                 ResolverCleanupFunc cleanup_func);

bool reference_resolver_unregister(ReferenceResolver *resolver, LanguageType language);

// Stats retrieval
void reference_resolver_get_stats(const ReferenceResolver *resolver, size_t *out_total_references,
                                  size_t *out_resolved_references,
                                  size_t *out_unresolved_references);

// Node resolution function
ResolutionStatus reference_resolver_resolve_node(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type, const char *name);

/**
 * @brief Language-specific resolution function signature
 */
typedef ResolutionStatus (*ResolverFunction)(ASTNode *node, ReferenceType ref_type,
                                             const char *name, GlobalSymbolTable *symbol_table,
                                             void *resolver_data);

/**
 * @brief Cleanup function for language-specific resolver data
 */
typedef void (*ResolverCleanupFunction)(void *resolver_data);

/**
 * @brief Language-specific resolver configuration
 */
typedef struct {
  LanguageType language;                ///< Language this resolver handles
  ResolverFunction resolver_func;       ///< Resolution function
  void *resolver_data;                  ///< Custom data for the resolver
  ResolverCleanupFunction cleanup_func; ///< Function to clean up resolver_data
} LanguageResolver;

// Language constants for testing
#define LANG_RUST LANG_UNKNOWN // Use LANG_UNKNOWN for tests

/**
 * @brief Detailed resolution result type
 */
typedef enum {
  RESOLUTION_UNKNOWN = 0,   ///< Resolution status is unknown
  RESOLUTION_SUCCESS,       ///< Reference successfully resolved
  RESOLUTION_NOT_FOUND,     ///< Symbol not found
  RESOLUTION_AMBIGUOUS,     ///< Multiple matching symbols found
  RESOLUTION_TYPE_MISMATCH, ///< Symbol found but type mismatch
  RESOLUTION_ERROR          ///< Resolution error (internal)
} ResolutionResult;

/**
 * @brief Symbol type enumeration
 */
typedef enum {
  SYMBOL_UNKNOWN = 0,
  SYMBOL_FUNCTION,
  SYMBOL_METHOD,
  SYMBOL_CLASS,
  SYMBOL_VARIABLE,
  SYMBOL_NAMESPACE,
  SYMBOL_MODULE,
  SYMBOL_TYPE
} SymbolType;

/**
 * @brief Symbol information for reference resolution
 */
typedef struct {
  char *name;           ///< Symbol name
  char *qualified_name; ///< Fully qualified name
  SymbolType type;      ///< Symbol type
  char *file_path;      ///< Source file path
  unsigned int line;    ///< Line number
  unsigned int column;  ///< Column number
  void *data;           ///< Optional additional data
} Symbol;

// Function to create a new symbol
Symbol *symbol_new(const char *name, SymbolType type);
// Function to free a symbol
void symbol_free(Symbol *symbol);
// Function to add a symbol to a symbol table (for tests)
static inline void symbol_table_add(GlobalSymbolTable *table, Symbol *symbol) {
  // This is a mock implementation for tests
  if (!table || !symbol)
    return;
  // In a real implementation, this would add the symbol to the table
}
// Function to resolve a reference (for tests)
static inline ResolutionResult resolve_reference(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type) {
  // This is a mock implementation that calls the actual API
  if (!resolver || !node)
    return RESOLUTION_ERROR;

  // Use direct reference resolver function
  ResolutionStatus status =
      reference_resolver_resolve_node(resolver, node, ref_type, "test_symbol");

  // Convert status to result for tests
  if (status == RESOLVE_SUCCESS) {
    return RESOLUTION_SUCCESS;
  } else {
    return RESOLUTION_ERROR;
  }
}

// Add extension to ASTNode for reference tests
typedef struct {
  Symbol *reference; ///< Resolved reference target
} ASTNodeExtension;

// We need to store node extensions in a separate map since ASTNode doesn't have a data field
#include <stdlib.h>

// Simple linked list of node references for testing
typedef struct NodeRefEntry {
  ASTNode *node;
  Symbol *reference;
  struct NodeRefEntry *next;
} NodeRefEntry;

static NodeRefEntry *g_node_refs = NULL;

// Functions to attach and get extension data for tests
static inline void ast_node_set_reference(ASTNode *node, Symbol *reference) {
  if (!node)
    return;

  // First check if this node is already in our list
  NodeRefEntry *current = g_node_refs;
  while (current) {
    if (current->node == node) {
      current->reference = reference;
      return;
    }
    current = current->next;
  }

  // Not found, add a new entry
  NodeRefEntry *entry = (NodeRefEntry *)malloc(sizeof(NodeRefEntry));
  if (!entry)
    return;

  entry->node = node;
  entry->reference = reference;
  entry->next = g_node_refs;
  g_node_refs = entry;
}

static inline Symbol *ast_node_get_reference(ASTNode *node) {
  if (!node)
    return NULL;

  NodeRefEntry *current = g_node_refs;
  while (current) {
    if (current->node == node) {
      return current->reference;
    }
    current = current->next;
  }

  return NULL;
}

// Clean up node references when done with tests
static inline void cleanup_node_references(void) {
  NodeRefEntry *current = g_node_refs;
  while (current) {
    NodeRefEntry *next = current->next;
    free(current);
    current = next;
  }
  g_node_refs = NULL;
}

// Define node types used in tests
#define NODE_TYPE_FUNCTION_CALL 100
#define REF_TYPE_FUNCTION REF_CALL

// Define resolution status mappings between public API and test implementation
#define RESOLVE_SUCCESS RESOLUTION_SUCCESS
#define RESOLVE_NOT_FOUND RESOLUTION_NOT_FOUND
#define RESOLVE_ERROR RESOLUTION_ERROR
#define RESOLVE_ERROR_INVALID_ARGS RESOLUTION_ERROR

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H */
