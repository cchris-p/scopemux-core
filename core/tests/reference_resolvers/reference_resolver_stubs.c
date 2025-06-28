/**
 * Reference resolver function stubs for unit tests
 * This file provides mock implementations of the reference resolver API
 * to allow tests to run without requiring the full implementation.
 */

#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/parser/parser_internal.h" // For ASTNODE_MAGIC
#include "reference_resolvers/reference_resolver_private.h"
#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

// Define max resolvers for our internal implementation
#define MAX_LANGUAGE_RESOLVERS 10

// Define private ReferenceResolver structure for our stubs
typedef struct {
  GlobalSymbolTable *symbol_table_ptr;                         // Global symbol table reference
  LanguageResolver language_resolvers[MAX_LANGUAGE_RESOLVERS]; // Language-specific resolvers
  int num_resolvers;                                           // Number of registered resolvers
  size_t total_references;                                     // Total references encountered
  size_t resolved_references;                                  // Successfully resolved references
  size_t unresolved_references;                                // Unresolved references
  SymbolType symbol_type;                                      // Symbol type
  ResolutionStatus resolution_status;                          // Resolution status
} ReferenceResolverImpl;

// General purpose mock resolver function used by multiple tests
static ResolutionStatus mock_resolver_func(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data) {
  // Prevent unused parameter warnings
  (void)ref_type;
  (void)name;
  (void)symbol_table;
  (void)resolver_data;

  // Safety check - validate the node and fix the magic number if needed
  if (!node) {
    return RESOLVE_ERROR;
  }

  // Check and fix magic number if corrupted
  if (node->magic != ASTNODE_MAGIC) {
    // Log that we're fixing a corrupted node
    fprintf(stderr,
            "[RESOLVER] Fixing corrupted magic number in mock_resolver_func: 0x%X -> 0x%X\n",
            node->magic, ASTNODE_MAGIC);
    node->magic = ASTNODE_MAGIC; // Restore the proper magic number
  }

  // Just return success for testing
  return RESOLVE_SUCCESS;
}

// Create a global symbol table
GlobalSymbolTable *symbol_table_create(size_t initial_capacity) {
  GlobalSymbolTable *table = calloc(1, sizeof(GlobalSymbolTable));
  cr_assert(table != NULL, "Failed to allocate symbol table");
  // We don't actually use initial_capacity in our stub
  (void)initial_capacity; // Suppress unused parameter warning
  return table;
}

// Free a symbol table
void symbol_table_free(GlobalSymbolTable *table) {
  if (table) {
    free(table);
  }
}

// Create a reference resolver
ReferenceResolver *reference_resolver_create(GlobalSymbolTable *symbol_table) {
  ReferenceResolverImpl *resolver = calloc(1, sizeof(ReferenceResolverImpl));
  cr_assert(resolver != NULL, "Failed to allocate reference resolver");
  // Store the symbol table in our test resolver
  resolver->symbol_table_ptr = symbol_table;
  resolver->num_resolvers = 0;
  resolver->total_references = 0;
  resolver->resolved_references = 0;
  resolver->unresolved_references = 0;
  return (ReferenceResolver *)resolver;
}

// Free a reference resolver
void reference_resolver_free(ReferenceResolver *resolver) {
  if (resolver) {
    ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

    // Clean up any registered resolvers with cleanup functions
    for (int i = 0; i < impl->num_resolvers; i++) {
      if (impl->language_resolvers[i].cleanup_func) {
        impl->language_resolvers[i].cleanup_func(impl->language_resolvers[i].resolver_data);
      }
    }

    free(impl);
  }
}

// Initialize built-in resolvers
void reference_resolver_init_builtin(ReferenceResolver *resolver) {
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  // Check if resolver is valid
  if (!impl) {
    return;
  }

  // Register mock resolvers for different languages
  // In a real implementation, this would load language-specific resolvers
  reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);
  reference_resolver_register(resolver, LANG_PYTHON, mock_resolver_func, NULL, NULL);
  reference_resolver_register(resolver, LANG_JAVASCRIPT, mock_resolver_func, NULL, NULL);
  reference_resolver_register(resolver, LANG_TYPESCRIPT, mock_resolver_func, NULL, NULL);
}

// Register a language resolver
bool reference_resolver_register(ReferenceResolver *resolver, LanguageType language,
                                 LanguageResolverFunc resolver_func, void *resolver_data,
                                 ResolverCleanupFunc cleanup_func) {
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  // Basic validation
  if (!impl || !resolver_func) {
    return false;
  }

  // Find an empty slot or one with matching language ID
  int slot = -1;
  for (int i = 0; i < impl->num_resolvers; i++) {
    if (impl->language_resolvers[i].language == language) {
      // Clean up existing resolver data if needed
      if (impl->language_resolvers[i].cleanup_func && impl->language_resolvers[i].resolver_data) {
        impl->language_resolvers[i].cleanup_func(impl->language_resolvers[i].resolver_data);
      }
      slot = i;
      break;
    }
  }

  // If no existing slot found, use the next available one
  if (slot == -1) {
    if (impl->num_resolvers >= MAX_LANGUAGE_RESOLVERS) {
      return false; // No more space for new resolvers
    }
    slot = impl->num_resolvers;
    impl->num_resolvers++;
  }

  // Store the resolver info
  impl->language_resolvers[slot].language = language;
  impl->language_resolvers[slot].resolver_func = resolver_func;
  impl->language_resolvers[slot].resolver_data = resolver_data;
  impl->language_resolvers[slot].cleanup_func = cleanup_func;

  return true;
}

// Unregister a language resolver
bool reference_resolver_unregister(ReferenceResolver *resolver, LanguageType language) {
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  if (!impl) {
    return false;
  }

  // Find the resolver for the specified language
  int index = -1;
  for (int i = 0; i < impl->num_resolvers; i++) {
    if (impl->language_resolvers[i].language == language) {
      index = i;
      break;
    }
  }

  // If no resolver found for this language, return failure
  if (index == -1) {
    return false;
  }

  // Clean up resolver data if needed
  if (impl->language_resolvers[index].cleanup_func &&
      impl->language_resolvers[index].resolver_data) {
    impl->language_resolvers[index].cleanup_func(impl->language_resolvers[index].resolver_data);
  }

  // Shift remaining resolvers down to fill the gap
  for (int i = index; i < impl->num_resolvers - 1; i++) {
    impl->language_resolvers[i] = impl->language_resolvers[i + 1];
  }

  // Clear the last resolver slot and decrement count
  memset(&impl->language_resolvers[impl->num_resolvers - 1], 0, sizeof(LanguageResolver));
  impl->num_resolvers--;

  return true;
}

// Resolve a node's references
ResolutionStatus reference_resolver_resolve_node(ReferenceResolver *resolver, ASTNode *node,
                                                 ReferenceType ref_type, const char *name) {
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  // Basic validation
  if (!impl || !node) {
    return RESOLVE_ERROR;
  }

  // Get the node language ID (stub implementation just uses the first language)
  LanguageType language = LANG_C; // Default to C language

  // Increment total references counter
  impl->total_references++;

  // Find a resolver for this language
  for (int i = 0; i < impl->num_resolvers; i++) {
    if (impl->language_resolvers[i].language == language) {
      // Call the resolver function
      LanguageResolverFunc resolver_func = impl->language_resolvers[i].resolver_func;
      if (resolver_func) {
        ResolutionStatus status = resolver_func(node, ref_type, name, impl->symbol_table_ptr,
                                                impl->language_resolvers[i].resolver_data);

        // Update stats
        if (status == RESOLVE_SUCCESS) {
          impl->resolved_references++;
        } else {
          impl->unresolved_references++;
        }

        return status;
      }
    }
  }

  // No resolver found or resolution failed
  impl->unresolved_references++;
  return RESOLVE_NOT_FOUND;
}

// Structure for resolver statistics
typedef struct ResolverStats {
  size_t total_references;
  size_t resolved_references;
  size_t unresolved_references;
} ResolverStats;

// Get detailed statistics for the reference resolver
void reference_resolver_get_statistics(const ReferenceResolver *resolver, ResolverStats *stats) {
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  if (impl && stats) {
    stats->total_references = impl->total_references;
    stats->resolved_references = impl->resolved_references;
    stats->unresolved_references = impl->unresolved_references;
  }
}

// Get statistics for the reference resolver (legacy compatibility function)
void reference_resolver_get_stats(const ReferenceResolver *resolver, size_t *out_total_references,
                                  size_t *out_resolved_references,
                                  size_t *out_unresolved_references) {
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  if (impl && out_total_references) {
    *out_total_references = impl->total_references;
  }

  if (impl && out_resolved_references) {
    *out_resolved_references = impl->resolved_references;
  }

  if (out_unresolved_references) {
    *out_unresolved_references = impl->unresolved_references;
  }
}

// Symbol functions are implemented in symbol_test_helpers.c
// We don't need to duplicate them here

// NOTE: Project context functions are now imported from the core library
// to avoid duplication and link conflicts

/**
 * Get the resolved symbol for a node (stub implementation).
 * Returns a predefined symbol for testing purposes.
 */
Symbol *reference_resolver_get_resolved_symbol(ReferenceResolver *resolver, ASTNode *node) {
  // Stub implementation returning a fixed symbol for testing
  static Symbol test_symbol = {0};

  // Prevent unused parameter warnings
  (void)resolver;

  // Initialize test symbol once
  if (test_symbol.name == NULL) {
    // For test_function, return referenced_function with file info
    test_symbol.name = strdup("referenced_function");
    test_symbol.file_path = strdup("test_file.c");
    test_symbol.line = 42;
    test_symbol.column = 10;
    test_symbol.type = SYMBOL_FUNCTION;
  }

  return &test_symbol;
}

/**
 * Resolve references in a file (stub implementation).
 * Returns the number of references resolved for testing.
 */
size_t reference_resolver_resolve_file(ReferenceResolver *resolver, ParserContext *ctx,
                                       const char *file_path) {
  // Stub implementation for file-level resolution
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  // Prevent unused parameter warnings
  (void)ctx;
  (void)file_path;

  if (!impl) {
    return 0;
  }

  // For testing, pretend one reference was resolved
  impl->total_references++;
  impl->resolved_references++;

  return 1; // Return 1 reference resolved for testing
}

/**
 * Resolve references in a project (stub implementation).
 * Returns the number of references resolved for testing.
 */
size_t reference_resolver_resolve_project(ReferenceResolver *resolver, ProjectContext *project,
                                          ParserContext *parser_ctx) {
  // Stub implementation for project-level resolution
  ReferenceResolverImpl *impl = (ReferenceResolverImpl *)resolver;

  // Prevent unused parameter warnings
  (void)project;
  (void)parser_ctx;

  if (!impl) {
    return 0;
  }

  // For testing, pretend two references were resolved (one for each test file)
  impl->total_references += 2;
  impl->resolved_references += 2;

  return 2; // Return 2 references resolved for testing
}
