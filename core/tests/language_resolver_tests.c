/**
 * @file language_resolver_tests.c
 * @brief Main test runner for language-specific resolver tests
 */

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/new/assert.h>
#include <criterion/options.h>

#include "reference_resolvers/reference_resolver_private.h"
#include "scopemux/ast.h"
#include "scopemux/memory_debug.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

// Forward declarations for language resolver functions
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

// Test fixtures
static GlobalSymbolTable *symbol_table = NULL;
static ASTNode *c_node = NULL;
static ASTNode *python_node = NULL;
static ASTNode *js_node = NULL;
static ASTNode *ts_node = NULL;

// Helper for creating test AST nodes with language-specific attributes
static ASTNode *create_test_node(Language lang, const char *name) {
  if (!name) {
    return NULL;
  }

  ASTNode *node = ast_node_new(NODE_FUNCTION, name);
  if (!node) {
    return NULL;
  }

  // Add language-specific attributes where needed
  switch (lang) {
  case LANG_C:
    // Simulate scoping via parent relationship for C
    ASTNode *parent = ast_node_new(NODE_FUNCTION, "parent_function");
    if (!parent) {
      ast_node_free(node);
      return NULL;
    }
    if (!ast_node_add_child(parent, node)) {
      ast_node_free(parent); // This will also free the child node
      return NULL;
    }
    return parent;

  case LANG_PYTHON:
  case LANG_JAVASCRIPT:
  case LANG_TYPESCRIPT:
  default:
    return node;
  }
}

void setup_language_resolvers() {
  printf("[DEBUG] Entering setup_language_resolvers\n");
  // Create a symbol table and populate it with test symbols for each language
  symbol_table = symbol_table_create(32);
  cr_assert(symbol_table != NULL, "Failed to create symbol table");

  // Add C symbol
  printf("[DEBUG] Allocating C symbol\n");
  printf("[DEBUG] About to call symbol_new for C symbol\n");
  Symbol *c_sym = symbol_new("c_function", SYMBOL_FUNCTION);
  printf("[DEBUG] Returned from symbol_new for C symbol\n");
  printf("[DEBUG] c_sym pointer: %p\n", (void *)c_sym);
  cr_assert(c_sym != NULL, "Failed to create C symbol");
  c_sym->language = LANG_C;
  printf("[DEBUG] Set language for C symbol\n");
  // Create a node for the symbol
  ASTNode *c_node = ast_node_new(NODE_FUNCTION, "c_function");
  c_node->file_path = STRDUP("test.c", "test_c_file_path");
  SourceRange range = {.start = {.line = 10, .column = 0, .offset = 0},
                       .end = {.line = 15, .column = 0, .offset = 0}};
  c_node->range = range;
  c_sym->node = c_node;
  c_sym->is_definition = true;
  printf("[DEBUG] Set node for C symbol\n");
  printf("[DEBUG] Set line for C symbol\n");
  int add_result = symbol_table_add(symbol_table, c_sym);
  printf("[DEBUG] symbol_table_add result for C symbol: %d\n", add_result);
  cr_assert(add_result, "Failed to add C symbol to table");

  // Add Python symbol
  printf("[DEBUG] Allocating Python symbol\n");
  Symbol *py_sym = symbol_new("python_function", SYMBOL_FUNCTION);
  cr_assert(py_sym != NULL, "Failed to create Python symbol");
  py_sym->language = LANG_PYTHON;
  // Create a node for the symbol
  ASTNode *py_node = ast_node_new(NODE_FUNCTION, "python_function");
  py_node->file_path = STRDUP("test.py", "test_py_file_path");
  SourceRange py_range = {.start = {.line = 20, .column = 0, .offset = 0},
                          .end = {.line = 25, .column = 0, .offset = 0}};
  py_node->range = py_range;
  py_sym->node = py_node;
  py_sym->is_definition = true;
  cr_assert(symbol_table_add(symbol_table, py_sym), "Failed to add Python symbol to table");

  // Add JavaScript symbol
  printf("[DEBUG] Allocating JavaScript symbol\n");
  Symbol *js_sym = symbol_new("js_function", SYMBOL_FUNCTION);
  cr_assert(js_sym != NULL, "Failed to create JavaScript symbol");
  js_sym->language = LANG_JAVASCRIPT;
  // Create a node for the symbol
  ASTNode *js_node = ast_node_new(NODE_FUNCTION, "js_function");
  js_node->file_path = STRDUP("test.js", "test_js_file_path");
  SourceRange js_range = {.start = {.line = 30, .column = 0, .offset = 0},
                          .end = {.line = 35, .column = 0, .offset = 0}};
  js_node->range = js_range;
  js_sym->node = js_node;
  js_sym->is_definition = true;
  cr_assert(symbol_table_add(symbol_table, js_sym), "Failed to add JavaScript symbol to table");

  // Add TypeScript symbol
  printf("[DEBUG] Allocating TypeScript symbol\n");
  Symbol *ts_sym = symbol_new("ts_function", SYMBOL_FUNCTION);
  cr_assert(ts_sym != NULL, "Failed to create TypeScript symbol");
  ts_sym->language = LANG_TYPESCRIPT;
  // Create a node for the symbol
  ASTNode *ts_node = ast_node_new(NODE_FUNCTION, "ts_function");
  ts_node->file_path = STRDUP("test.ts", "test_ts_file_path");
  SourceRange ts_range = {.start = {.line = 40, .column = 0, .offset = 0},
                          .end = {.line = 45, .column = 0, .offset = 0}};
  ts_node->range = ts_range;
  ts_sym->node = ts_node;
  ts_sym->is_definition = true;
  cr_assert(symbol_table_add(symbol_table, ts_sym), "Failed to add TypeScript symbol to table");

  // Create test nodes for each language
  printf("[DEBUG] Creating test AST nodes\n");
  c_node = create_test_node(LANG_C, "c_function");
  cr_assert(c_node != NULL, "Failed to create C test node");

  python_node = create_test_node(LANG_PYTHON, "python_function");
  cr_assert(python_node != NULL, "Failed to create Python test node");

  js_node = create_test_node(LANG_JAVASCRIPT, "js_function");
  cr_assert(js_node != NULL, "Failed to create JavaScript test node");

  ts_node = create_test_node(LANG_TYPESCRIPT, "ts_function");
  cr_assert(ts_node != NULL, "Failed to create TypeScript test node");
  printf("[DEBUG] Exiting setup_language_resolvers\n");
}

void teardown_language_resolvers() {
  printf("[DEBUG] Entering teardown_language_resolvers\n");
  if (symbol_table) {
    printf("[DEBUG] Freeing symbol_table at %p\n", (void *)symbol_table);
    symbol_table_free(symbol_table);
    symbol_table = NULL;
  }

  // Clean up test nodes
  if (c_node) {
    printf("[DEBUG] Freeing c_node at %p\n", (void *)c_node);
    ast_node_free(c_node); // This will also free the child node
    c_node = NULL;
  }

  if (python_node) {
    printf("[DEBUG] Freeing python_node at %p\n", (void *)python_node);
    ast_node_free(python_node);
    python_node = NULL;
  }

  if (js_node) {
    printf("[DEBUG] Freeing js_node at %p\n", (void *)js_node);
    ast_node_free(js_node);
    js_node = NULL;
  }

  if (ts_node) {
    printf("[DEBUG] Freeing ts_node at %p\n", (void *)ts_node);
    ast_node_free(ts_node);
    ts_node = NULL;
  }
  printf("[DEBUG] Exiting teardown_language_resolvers\n");
}

// Test C language resolver
Test(language_resolvers, c_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Get the actual function call node (child of the parent node)
  ASTNode *call_node = c_node->children[0];

  // Call the C resolver directly
  ResolutionStatus result =
      reference_resolver_c(call_node, REF_CALL, "c_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS, "C resolver should successfully resolve the reference");
  Symbol *ref = ast_node_get_reference(call_node);
  cr_assert(ref != NULL, "Reference should be populated");
  cr_assert_str_eq(ref->node->file_path, "test.c", "Reference file path should match");
  cr_assert(ref->node->range.start.line == 10, "Reference line should match");
}

// Test Python language resolver
Test(language_resolvers, python_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Call the Python resolver directly
  ResolutionStatus result =
      reference_resolver_python(python_node, REF_CALL, "python_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS,
            "Python resolver should successfully resolve the reference");
  Symbol *ref = ast_node_get_reference(python_node);
  cr_assert(ref != NULL, "Reference should be populated");
  cr_assert_str_eq(ref->node->file_path, "test.py", "Reference file path should match");
  cr_assert(ref->node->range.start.line == 20, "Reference line should match");
}

// Test JavaScript language resolver
Test(language_resolvers, javascript_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Call the JavaScript resolver directly
  ResolutionStatus result =
      reference_resolver_javascript(js_node, REF_CALL, "js_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS,
            "JavaScript resolver should successfully resolve the reference");
  Symbol *ref = ast_node_get_reference(js_node);
  cr_assert(ref != NULL, "Reference should be populated");
  cr_assert_str_eq(ref->node->file_path, "test.js", "Reference file path should match");
  cr_assert(ref->node->range.start.line == 30, "Reference line should match");
}

// Test TypeScript language resolver
Test(language_resolvers, typescript_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Call the TypeScript resolver directly
  ResolutionStatus result =
      reference_resolver_typescript(ts_node, REF_CALL, "ts_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS,
            "TypeScript resolver should successfully resolve the reference");
  Symbol *ref = ast_node_get_reference(ts_node);
  cr_assert(ref != NULL, "Reference should be populated");
  cr_assert_str_eq(ref->node->file_path, "test.ts", "Reference file path should match");
  cr_assert(ref->node->range.start.line == 40, "Reference line should match");
}

// No custom main function needed - Criterion provides its own when run through the test runner
