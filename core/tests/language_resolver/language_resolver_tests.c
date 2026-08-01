#include <criterion/criterion.h>

#include "../reference_resolvers/reference_resolver_private.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

static GlobalSymbolTable *symbol_table;
static ASTNode *c_symbol_node;
static ASTNode *python_symbol_node;
static ASTNode *js_symbol_node;
static ASTNode *ts_symbol_node;
static ASTNode *c_node;
static ASTNode *python_node;
static ASTNode *js_node;
static ASTNode *ts_node;

static void setup_language_resolvers(void) {
  symbol_table = symbol_table_create(32);
  cr_assert_not_null(symbol_table);

  c_symbol_node = make_test_node(NODE_FUNCTION, "c_function", LANG_C);
  python_symbol_node = make_test_node(NODE_FUNCTION, "python_function", LANG_PYTHON);
  js_symbol_node = make_test_node(NODE_FUNCTION, "js_function", LANG_JAVASCRIPT);
  ts_symbol_node = make_test_node(NODE_FUNCTION, "ts_function", LANG_TYPESCRIPT);

  cr_assert(ast_node_set_file_path(c_symbol_node, "test.c", AST_SOURCE_STATIC));
  cr_assert(ast_node_set_file_path(python_symbol_node, "test.py", AST_SOURCE_STATIC));
  cr_assert(ast_node_set_file_path(js_symbol_node, "test.js", AST_SOURCE_STATIC));
  cr_assert(ast_node_set_file_path(ts_symbol_node, "test.ts", AST_SOURCE_STATIC));

  c_symbol_node->range.start.line = 10;
  python_symbol_node->range.start.line = 20;
  js_symbol_node->range.start.line = 30;
  ts_symbol_node->range.start.line = 40;

  cr_assert_not_null(symbol_table_register(symbol_table, "c_function", c_symbol_node, "test.c",
                                           SCOPE_GLOBAL, LANG_C));
  cr_assert_not_null(symbol_table_register(symbol_table, "python_function", python_symbol_node,
                                           "test.py", SCOPE_GLOBAL, LANG_PYTHON));
  cr_assert_not_null(symbol_table_register(symbol_table, "js_function", js_symbol_node,
                                           "test.js", SCOPE_GLOBAL, LANG_JAVASCRIPT));
  cr_assert_not_null(symbol_table_register(symbol_table, "ts_function", ts_symbol_node,
                                           "test.ts", SCOPE_GLOBAL, LANG_TYPESCRIPT));

  c_node = make_test_node(NODE_FUNCTION, "c_function", LANG_C);
  python_node = make_test_node(NODE_FUNCTION, "python_function", LANG_PYTHON);
  js_node = make_test_node(NODE_FUNCTION, "js_function", LANG_JAVASCRIPT);
  ts_node = make_test_node(NODE_FUNCTION, "ts_function", LANG_TYPESCRIPT);
}

static void teardown_language_resolvers(void) {
  ast_node_free(c_node);
  ast_node_free(python_node);
  ast_node_free(js_node);
  ast_node_free(ts_node);
  ast_node_free(c_symbol_node);
  ast_node_free(python_symbol_node);
  ast_node_free(js_symbol_node);
  ast_node_free(ts_symbol_node);
  symbol_table_free(symbol_table);

  symbol_table = NULL;
  c_symbol_node = NULL;
  python_symbol_node = NULL;
  js_symbol_node = NULL;
  ts_symbol_node = NULL;
  c_node = NULL;
  python_node = NULL;
  js_node = NULL;
  ts_node = NULL;
}

Test(language_resolvers, c_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  cr_assert_eq(reference_resolver_c(c_node, REF_CALL, "c_function", symbol_table, NULL),
               RESOLUTION_SUCCESS);
  cr_assert_eq(c_node->num_references, 1);
  cr_assert_eq(c_node->references[0], c_symbol_node);
}

Test(language_resolvers, python_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  cr_assert_eq(reference_resolver_python(python_node, REF_CALL, "python_function", symbol_table,
                                         NULL),
               RESOLUTION_SUCCESS);
  cr_assert_eq(python_node->num_references, 1);
  cr_assert_eq(python_node->references[0], python_symbol_node);
}

Test(language_resolvers, javascript_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  cr_assert_eq(reference_resolver_javascript(js_node, REF_CALL, "js_function", symbol_table, NULL),
               RESOLUTION_SUCCESS);
  cr_assert_eq(js_node->num_references, 1);
  cr_assert_eq(js_node->references[0], js_symbol_node);
}

Test(language_resolvers, typescript_resolver, .init = setup_language_resolvers,
     .fini = teardown_language_resolvers) {
  cr_assert_eq(reference_resolver_typescript(ts_node, REF_CALL, "ts_function", symbol_table,
                                             NULL),
               RESOLUTION_SUCCESS);
  cr_assert_eq(ts_node->num_references, 1);
  cr_assert_eq(ts_node->references[0], ts_symbol_node);
}
