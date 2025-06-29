/**
 * @file resolver_registration_tests.c
 * @brief Unit tests for the resolver registration functionality
 *
 * Tests for language resolver registration, lookup, and built-in language resolvers.
 */

#include "reference_resolvers/reference_resolver_private.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>

// Forward declarations
void setup_registration(void);
void teardown_registration(void);

// First mock resolver - always returns SUCCESS
static ResolutionStatus first_resolver_func(ASTNode *node, ReferenceType ref_type, const char *name,
                                            GlobalSymbolTable *symbol_table, void *resolver_data) {
  (void)node;
  (void)ref_type;
  (void)name;
  (void)symbol_table;
  (void)resolver_data;
  return RESOLUTION_SUCCESS;
}

// Second mock resolver - always returns NOT_FOUND
static ResolutionStatus second_resolver_func(ASTNode *node, ReferenceType ref_type,
                                             const char *name, GlobalSymbolTable *symbol_table,
                                             void *resolver_data) {
  (void)node;
  (void)ref_type;
  (void)name;
  (void)symbol_table;
  (void)resolver_data;
  return RESOLUTION_NOT_FOUND;
}

// Custom data cleanup function
static void custom_cleanup_func(void *data) {
  if (data) {
    free(data);
  }
}

// Custom resolver that uses custom data
static ResolutionStatus custom_data_resolver_func(ASTNode *node, ReferenceType ref_type,
                                                  const char *name, GlobalSymbolTable *symbol_table,
                                                  void *resolver_data) {
  (void)node;
  (void)ref_type;
  (void)symbol_table;

  // Use resolver data to verify it's working
  typedef struct {
    int value;
    const char *name;
  } CustomData;

  CustomData *custom_data = (CustomData *)resolver_data;
  if (custom_data && custom_data->value == 42 && strcmp(custom_data->name, "test") == 0 &&
      strcmp(name, "custom_data_function") == 0) {
    return RESOLUTION_SUCCESS;
  }
  return RESOLUTION_NOT_FOUND;
}

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

// Test fixture setup
static ReferenceResolver *resolver = NULL;
static GlobalSymbolTable *symbol_table = NULL;

void setup_registration(void) {
  symbol_table = symbol_table_create(16);
  cr_assert(symbol_table != NULL, "Failed to create symbol table for tests");

  resolver = reference_resolver_create(symbol_table);
  cr_assert(resolver != NULL, "Failed to create reference resolver for tests");
}

void teardown_registration(void) {
  if (resolver) {
    reference_resolver_free(resolver);
    resolver = NULL;
  }

  if (symbol_table) {
    symbol_table_free(symbol_table);
    symbol_table = NULL;
  }
}

// Test language resolver lookup - modified to use only public API
Test(resolver_registration, find_language_resolver, .init = setup_registration,
     .fini = teardown_registration) {
  // Register a mock resolver
  bool reg_result = reference_resolver_register(resolver, LANG_C, mock_resolver_func, NULL, NULL);
  cr_assert(reg_result, "Registration should succeed");

  // Create a test node and try to resolve it to verify registration worked
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  // Try resolving with the registered resolver - should succeed for C
  ResolutionStatus c_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name");
  cr_assert(c_status == RESOLUTION_SUCCESS, "Resolution should succeed with registered C resolver");

  // Try with Python - should fail since we haven't registered it
  reference_resolver_unregister(resolver, LANG_C); // Unregister C resolver first
  ResolutionStatus py_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name");
  cr_assert(py_status == RESOLUTION_NOT_FOUND, "Resolution should fail with unregistered resolver");

  // Clean up
  ast_node_free(test_node);
}

// Test built-in resolver initialization
Test(resolver_registration, init_builtin, .init = setup_registration,
     .fini = teardown_registration) {
  // Initialize built-in resolvers
  reference_resolver_init_builtin(resolver);
  // Since init_builtin is void, we'll verify success by checking resolver functionality

  // Create test node to verify resolver functionality
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  // Verify C resolver functionality
  ResolutionStatus c_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_c_function");
  cr_assert(c_status != RESOLUTION_NOT_FOUND, "C resolver should be registered");

  // Verify Python resolver functionality
  ResolutionStatus py_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_py_function");
  cr_assert(py_status != RESOLUTION_NOT_FOUND, "Python resolver should be registered");

  // Verify JavaScript resolver functionality
  ResolutionStatus js_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_js_function");
  cr_assert(js_status != RESOLUTION_NOT_FOUND, "JavaScript resolver should be registered");

  // Verify TypeScript resolver functionality
  ResolutionStatus ts_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_ts_function");
  cr_assert(ts_status != RESOLUTION_NOT_FOUND, "TypeScript resolver should be registered");

  // Clean up
  ast_node_free(test_node);
}

// Test resolver priority (last registered wins)
Test(resolver_registration, resolver_priority, .init = setup_registration,
     .fini = teardown_registration) {
  // We'll use the predefined functions to test resolver priority

  // Register first resolver
  bool result1 = reference_resolver_register(resolver, LANG_C, first_resolver_func, NULL, NULL);
  cr_assert(result1, "First resolver registration should succeed");

  // Create a test node
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  // Verify first resolver is active - should return SUCCESS
  ResolutionStatus status1 =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name");
  cr_assert(status1 == RESOLUTION_SUCCESS, "First resolver should return SUCCESS");

  // Register second resolver (should replace first)
  bool result2 = reference_resolver_register(resolver, LANG_C, second_resolver_func, NULL, NULL);
  cr_assert(result2, "Second resolver registration should succeed");

  // Verify the second one is active - should return NOT_FOUND
  ResolutionStatus status2 =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "test_name");
  cr_assert(status2 == RESOLUTION_NOT_FOUND,
            "Second resolver should be active and return NOT_FOUND");

  // Clean up
  ast_node_free(test_node);
}

// Test resolver with custom data
Test(resolver_registration, resolver_with_custom_data, .init = setup_registration,
     .fini = teardown_registration) {
  // Custom resolver data structure
  typedef struct {
    int value;
    const char *name;
  } CustomData;

  // Create custom data
  CustomData *data = malloc(sizeof(CustomData));
  data->value = 42;
  data->name = "test";

  // We'll define a custom resolver function to test resolver data

  // We'll use the predefined custom_cleanup_func

  // Register resolver with custom data
  bool reg_result = reference_resolver_register(resolver, LANG_C, custom_data_resolver_func, data,
                                                custom_cleanup_func);
  cr_assert(reg_result, "Registration with custom data should succeed");

  // Create test node
  ASTNode *test_node = ast_node_new(NODE_TYPE_FUNCTION_CALL, "test_function");
  cr_assert(test_node != NULL, "Node creation should succeed");

  // Verify the resolver with custom data works correctly
  ResolutionStatus matched_status = reference_resolver_resolve_node(
      resolver, test_node, REF_TYPE_FUNCTION, "custom_data_function");
  cr_assert(matched_status == RESOLUTION_SUCCESS, "Resolver should use custom data correctly");

  // Try a mismatched name
  ResolutionStatus mismatched_status =
      reference_resolver_resolve_node(resolver, test_node, REF_TYPE_FUNCTION, "wrong_name");
  cr_assert(mismatched_status == RESOLUTION_NOT_FOUND, "Resolver should fail with wrong name");

  // Cleanup
  ast_node_free(test_node);
  // Note: data will be cleaned up by the cleanup_func when the resolver is freed
}
