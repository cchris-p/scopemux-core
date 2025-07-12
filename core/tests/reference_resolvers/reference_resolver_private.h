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
#include <symbol.h>

#ifdef __cplusplus
extern "C" {
#endif

// Language constants for testing
#define LANG_RUST LANG_UNKNOWN // Use LANG_UNKNOWN for tests

// Symbol functions are implemented in symbol_test_helpers.c
// Forward declarations only
Symbol *symbol_new(const char *name, SymbolType type);
void symbol_free(Symbol *symbol);

// Simple symbol entry for chaining
static inline unsigned long hash_qualified_name(const char *qualified_name, size_t num_buckets) {
  unsigned long hash = 5381;
  int c;
  while ((c = *qualified_name++))
    hash = ((hash << 5) + hash) + c; // hash * 33 + c
  return hash % num_buckets;
}

// Function to resolve a reference (for tests)
static inline ResolutionStatus resolve_reference(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type) {
  // This is a mock implementation that calls the actual API
  if (!resolver || !node)
    return RESOLUTION_ERROR;

  // Use direct reference resolver function
  ResolutionStatus status =
      reference_resolver_resolve_node(resolver, node, ref_type, "test_symbol", LANG_C);

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
  ReferenceType type; // Add the missing type field
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
      current->type = type; // Set the type field
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
  entry->type = type; // Set the type field
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

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H */
