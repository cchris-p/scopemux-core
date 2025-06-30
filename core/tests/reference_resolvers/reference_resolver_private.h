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
bool reference_resolver_init_builtin(ReferenceResolver *resolver);

// The real signatures for the resolve functions (based on the original API)
typedef ResolutionStatus (*LanguageResolverFunc)(ASTNode *node, ReferenceType ref_type,
                                                 const char *name, GlobalSymbolTable *symbol_table,
                                                 void *resolver_data);
typedef void (*ResolverCleanupFunc)(void *resolver_data);

// We're using the opaque LanguageResolver type from the headers
// No need to redefine it here

// Register and unregister language resolvers
bool reference_resolver_register(ReferenceResolver *resolver, Language language,
                                 LanguageResolverFunc resolver_func, void *resolver_data,
                                 ResolverCleanupFunc cleanup_func);

bool reference_resolver_unregister(ReferenceResolver *resolver, Language language);

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

// LanguageResolver is already defined in reference_resolver.h
// No need to redefine it here

// Language constants for testing
#define LANG_RUST LANG_UNKNOWN // Use LANG_UNKNOWN for tests

// Use the standard ResolutionStatus from reference_resolver.h
// No need to redefine it here

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
  Language language;    ///< Language of the symbol
  void *data;           ///< Optional additional data
} Symbol;

// Symbol functions are implemented in symbol_test_helpers.c
// Forward declarations only
Symbol *symbol_new(const char *name, SymbolType type);
void symbol_free(Symbol *symbol);

// Function to add a symbol to a symbol table (for tests)
#include <string.h>

// Simple symbol entry for chaining
static inline unsigned long hash_qualified_name(const char *qualified_name, size_t num_buckets) {
  unsigned long hash = 5381;
  int c;
  while ((c = *qualified_name++))
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  return hash % num_buckets;
}

static inline bool symbol_table_add(GlobalSymbolTable *table, SymbolEntry *entry) {
  if (!table || !entry || !entry->qualified_name)
    return false;

  unsigned long index = hash_qualified_name(entry->qualified_name, table->num_buckets);
  SymbolEntry *existing = table->buckets[index];

  // Check for duplicates (same qualified_name + language)
  while (existing) {
    if (strcmp(existing->qualified_name, entry->qualified_name) == 0 &&
        existing->language == entry->language) {
      // Duplicate found
      return false;
    }
    existing = existing->next;
  }

  // Insert at head of chain
  entry->next = table->buckets[index];
  table->buckets[index] = entry;

  table->num_symbols++;
  table->count++;

  if (entry->next) {
    table->collisions++;
  }

  return true;
}
// Function to resolve a reference (for tests)
static inline ResolutionStatus resolve_reference(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type) {
  // This is a mock implementation that calls the actual API
  if (!resolver || !node)
    return RESOLUTION_ERROR;

  // Use direct reference resolver function
  ResolutionStatus status =
      reference_resolver_resolve_node(resolver, node, ref_type, "test_symbol");

  return status;
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
  ReferenceType type;  // Add the missing type field
  struct NodeRefEntry *next;
} NodeRefEntry;

extern NodeRefEntry *g_node_refs;

// Functions to attach and get extension data for tests
static inline bool ast_node_set_reference(ASTNode *node, ReferenceType type, Symbol *reference) {
  if (!node)
    return false;

  // First check if this node is already in our list
  NodeRefEntry *current = g_node_refs;
  while (current) {
    if (current->node == node) {
      current->reference = reference;
      current->type = type;  // Set the type field
      return true;
    }
    current = current->next;
  }

  // Not found, add a new entry
  NodeRefEntry *entry = (NodeRefEntry *)malloc(sizeof(NodeRefEntry));
  if (!entry) {
    log_debug("NodeRefEntry not found for AST Node.");
    return false;
  }

  entry->node = node;
  entry->reference = reference;
  entry->type = type;  // Set the type field
  entry->next = g_node_refs;
  g_node_refs = entry;
  return true;
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

/* Extended private struct for test resolver implementation */
struct ReferenceResolver_Private {
  GlobalSymbolTable *symbol_table_ptr;
  int num_resolvers;
  size_t total_references;
  size_t resolved_references;
  size_t unresolved_references;
  struct {
    Language language;
    ResolverFunction resolver_func;
    void *resolver_data;
    ResolverCleanupFunc cleanup_func;
  } language_resolvers[10];
};

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H */
