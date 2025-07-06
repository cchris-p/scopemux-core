/**
 * @file resolver_core_tests.c
 * @brief Unit tests for the reference resolver core functionality
 *
 * Tests for creation, destruction, registration, and unregistration of reference resolvers.
 */

#include "reference_resolver_private.h" /* For resolution constants */
#include "scopemux/parser.h"            /* For AST node types and operations */
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>

// Function prototypes
void setup_resolver(void);
void teardown_resolver(void);

// Mock function for testing resolver registration
static ResolutionStatus mock_resolver_func(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data) {
  // Just return success for testing
  (void)node;          // Suppress unused parameter warnings
  (void)ref_type;      // Suppress unused parameter warnings
  (void)name;          // Suppress unused parameter warnings
  (void)symbol_table;  // Suppress unused parameter warnings
  (void)resolver_data; // Suppress unused parameter warnings
  return RESOLUTION_SUCCESS;
}

// Test fixture for reference resolver tests
static ReferenceResolver *resolver = NULL;
static GlobalSymbolTable *symbol_table = NULL;

// Setup function for tests
void setup_resolver(void) {
  symbol_table = symbol_table_create(16);
  cr_assert(symbol_table != NULL, "Failed to create symbol table for tests");

  resolver = reference_resolver_create(symbol_table);
  cr_assert(resolver != NULL, "Failed to create reference resolver for tests");
}

// Teardown function for tests
void teardown_resolver(void) {
  if (resolver) {
    reference_resolver_free(resolver);
    resolver = NULL;
  }

  if (symbol_table) {
    symbol_table_free(symbol_table);
    symbol_table = NULL;
  }
}

// Test creation and initialization of reference resolver
Test(resolver_core, create, .init = setup_resolver, .fini = teardown_resolver) {
  // We can only verify that the resolver was created successfully
  // Internal implementation details are hidden
  cr_assert(resolver != NULL, "reference_resolver_create should return non-NULL");
}

// Test registration of a language resolver
Test(resolver_core, register_resolver, .init = setup_resolver, .fini = teardown_resolver) {
  // Register a mock resolver for C language
  bool result = reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);
  cr_assert(result, "Registration of resolver should succeed");

  // Create test node and attempt to resolve it to verify registration worked
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  // Try resolving with the registered resolver
  ResolutionStatus status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name", LANG_C);
  cr_assert(status == RESOLUTION_SUCCESS, "Resolution should succeed with registered resolver");

  // Clean up
  ast_node_free(test_node);
}

// Test registration with replacement (registering same language twice)
Test(resolver_core, register_replacement, .init = setup_resolver, .fini = teardown_resolver) {
  // First registration
  bool result1 = reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);
  cr_assert(result1, "First registration should succeed");

  // Second registration for same language type
  bool result2 = reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);
  cr_assert(result2, "Replacement registration should succeed");

  // We can verify the replacement worked by ensuring resolution still works
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  ResolutionStatus status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name", LANG_C);
  cr_assert(status == RESOLUTION_SUCCESS, "Resolution should still work after replacement");

  // Clean up
  ast_node_free(test_node);
}

// Test unregistration of a language resolver
Test(resolver_core, unregister_resolver, .init = setup_resolver, .fini = teardown_resolver) {
  // Register then unregister
  reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);
  bool result = reference_resolver_unregister(resolver, LANG_C);

  cr_assert(result, "Unregistration should succeed");

  // Verify unregistration worked by checking that resolution now fails
  // First create a test node
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  // With no resolver registered, we should get RESOLUTION_UNKNOWN
  ResolutionStatus status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name", LANG_C);
  cr_assert(status == RESOLUTION_NOT_FOUND, "Resolution should fail after unregistering resolver");

  // Clean up
  ast_node_free(test_node);
}

// Test statistics reporting
Test(resolver_core, get_stats, .init = setup_resolver, .fini = teardown_resolver) {
  // First register a resolver
  reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);

  // Create a node and resolve it multiple times to generate statistics
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");

  // Call resolve multiple times
  for (int i = 0; i < 5; i++) {
    reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name", LANG_C);
  }

  // Get the statistics
  size_t total_refs = 0, resolved_refs = 0, unresolved_refs = 0;
  reference_resolver_get_stats(resolver, &total_refs, &resolved_refs, &unresolved_refs);

  // Verify that some statistics were collected
  cr_assert(total_refs > 0, "Total references should be greater than 0");
  cr_assert(resolved_refs > 0, "Resolved references should be greater than 0");
  cr_assert(total_refs == resolved_refs + unresolved_refs,
            "Total should equal resolved + unresolved");

  // Clean up
  ast_node_free(test_node);
}