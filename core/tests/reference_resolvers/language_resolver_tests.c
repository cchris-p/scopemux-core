/**
 * @file language_resolver_tests.c
 * @brief Unit tests for language-specific resolvers
 *
 * Tests each language resolver implementation (C, Python, JavaScript, TypeScript)
 * to ensure they correctly handle language-specific reference resolution patterns.
 */

#include "scopemux/ast.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>

// Forward declarations for language-specific resolvers
extern ResolutionResult c_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                        GlobalSymbolTable *symbol_table, void *resolver_data);

extern ResolutionResult python_resolver_impl(ASTNode *node, ReferenceType ref_type,
                                             const char *name, GlobalSymbolTable *symbol_table,
                                             void *resolver_data);

extern ResolutionResult javascript_resolver_impl(ASTNode *node, ReferenceType ref_type,
                                                 const char *name, GlobalSymbolTable *symbol_table,
                                                 void *resolver_data);

extern ResolutionResult typescript_resolver_impl(ASTNode *node, ReferenceType ref_type,
                                                 const char *name, GlobalSymbolTable *symbol_table,
                                                 void *resolver_data);

// Test fixtures
static GlobalSymbolTable *symbol_table = NULL;
static ASTNode *c_node = NULL;
static ASTNode *python_node = NULL;
static ASTNode *js_node = NULL;
static ASTNode *ts_node = NULL;

// Helper for creating test AST nodes with language-specific attributes
static ASTNode *create_test_node(LanguageType lang, const char *name) {
  ASTNode *node = ast_node_new(NODE_TYPE_FUNCTION_CALL);
  node->language = lang;
  ast_node_set_name(node, name);

  // Add language-specific attributes where needed
  switch (lang) {
  case LANG_C:
    // Simulate scoping via parent relationship for C
    ASTNode *parent = ast_node_new(NODE_TYPE_FUNCTION);
    parent->language = LANG_C;
    ast_node_set_name(parent, "parent_function");
    ast_node_add_child(parent, node);
    return parent;

  case LANG_PYTHON:
    // Add import attributes for Python
    ast_node_set_attribute(node, "module", "test_module");
    return node;

  case LANG_JAVASCRIPT:
    // Add module attributes for JavaScript
    ast_node_set_attribute(node, "module_path", "./test_module.js");
    return node;

  case LANG_TYPESCRIPT:
    // Add type information for TypeScript
    ast_node_set_attribute(node, "type_annotation", "TestType");
    return node;

  default:
    return node;
  }
}

void setup_language_resolvers() {
  // Create a symbol table and populate it with test symbols for each language
  symbol_table = symbol_table_create(32);
  cr_assert(symbol_table != NULL, "Failed to create symbol table");

  // Add C symbol
  Symbol *c_sym = symbol_new("c_function", SYMBOL_FUNCTION);
  c_sym->language = LANG_C;
  c_sym->file_path = strdup("test.c");
  c_sym->line = 10;
  symbol_table_add(symbol_table, c_sym);

  // Add Python symbol
  Symbol *py_sym = symbol_new("python_function", SYMBOL_FUNCTION);
  py_sym->language = LANG_PYTHON;
  py_sym->file_path = strdup("test.py");
  py_sym->line = 20;
  symbol_table_add(symbol_table, py_sym);

  // Add JavaScript symbol
  Symbol *js_sym = symbol_new("js_function", SYMBOL_FUNCTION);
  js_sym->language = LANG_JAVASCRIPT;
  js_sym->file_path = strdup("test.js");
  js_sym->line = 30;
  symbol_table_add(symbol_table, js_sym);

  // Add TypeScript symbol
  Symbol *ts_sym = symbol_new("ts_function", SYMBOL_FUNCTION);
  ts_sym->language = LANG_TYPESCRIPT;
  ts_sym->file_path = strdup("test.ts");
  ts_sym->line = 40;
  symbol_table_add(symbol_table, ts_sym);

  // Create test nodes for each language
  c_node = create_test_node(LANG_C, "c_function");
  python_node = create_test_node(LANG_PYTHON, "python_function");
  js_node = create_test_node(LANG_JAVASCRIPT, "js_function");
  ts_node = create_test_node(LANG_TYPESCRIPT, "ts_function");
}

void teardown_language_resolvers() {
  if (symbol_table) {
    symbol_table_free(symbol_table);
    symbol_table = NULL;
  }

  // Clean up test nodes
  if (c_node) {
    ast_node_free(c_node); // This will also free the child node
    c_node = NULL;
  }

  if (python_node) {
    ast_node_free(python_node);
    python_node = NULL;
  }

  if (js_node) {
    ast_node_free(js_node);
    js_node = NULL;
  }

  if (ts_node) {
    ast_node_free(ts_node);
    ts_node = NULL;
  }
}

// Test C language resolver
Test(language_resolvers, c_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Get the actual function call node (child of the parent node)
  ASTNode *call_node = c_node->children[0];

  // Call the C resolver directly
  ResolutionResult result =
      c_resolver_impl(call_node, REF_TYPE_FUNCTION, "c_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS, "C resolver should successfully resolve the reference");
  cr_assert(call_node->reference != NULL, "Reference should be populated");
  cr_assert_str_eq(call_node->reference->file_path, "test.c", "Reference file path should match");
  cr_assert(call_node->reference->line == 10, "Reference line should match");
}

// Test Python language resolver
Test(language_resolvers, python_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Call the Python resolver directly
  ResolutionResult result =
      python_resolver_impl(python_node, REF_TYPE_FUNCTION, "python_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS,
            "Python resolver should successfully resolve the reference");
  cr_assert(python_node->reference != NULL, "Reference should be populated");
  cr_assert_str_eq(python_node->reference->file_path, "test.py",
                   "Reference file path should match");
  cr_assert(python_node->reference->line == 20, "Reference line should match");
}

// Test JavaScript language resolver
Test(language_resolvers, javascript_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Call the JavaScript resolver directly
  ResolutionResult result =
      javascript_resolver_impl(js_node, REF_TYPE_FUNCTION, "js_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS,
            "JavaScript resolver should successfully resolve the reference");
  cr_assert(js_node->reference != NULL, "Reference should be populated");
  cr_assert_str_eq(js_node->reference->file_path, "test.js", "Reference file path should match");
  cr_assert(js_node->reference->line == 30, "Reference line should match");
}

// Test TypeScript language resolver
Test(language_resolvers, typescript_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  // Call the TypeScript resolver directly
  ResolutionResult result =
      typescript_resolver_impl(ts_node, REF_TYPE_FUNCTION, "ts_function", symbol_table, NULL);

  // Verify resolution
  cr_assert(result == RESOLUTION_SUCCESS,
            "TypeScript resolver should successfully resolve the reference");
  cr_assert(ts_node->reference != NULL, "Reference should be populated");
  cr_assert_str_eq(ts_node->reference->file_path, "test.ts", "Reference file path should match");
  cr_assert(ts_node->reference->line == 40, "Reference line should match");
}
