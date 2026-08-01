#include "reference_resolver_private.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>

static ReferenceResolver *resolver;
static GlobalSymbolTable *symbol_table;
static ASTNode *root_node;
static ASTNode *symbol_node;

static ASTNode *make_resolution_tree(void) {
  ASTNode *root = make_test_node(NODE_ROOT, "root", LANG_C);
  ASTNode *func = make_test_node(NODE_FUNCTION, "test_function", LANG_C);
  ASTNode *call = make_test_node(NODE_FUNCTION, "referenced_function", LANG_C);

  ast_node_add_child(root, func);
  ast_node_add_child(func, call);
  return root;
}

static void setup_resolution(void) {
  symbol_table = symbol_table_create(16);
  cr_assert_not_null(symbol_table);

  resolver = reference_resolver_create(symbol_table);
  cr_assert_not_null(resolver);
  cr_assert(reference_resolver_init_builtin(resolver));

  root_node = make_resolution_tree();
  cr_assert_not_null(root_node);

  symbol_node = make_test_node(NODE_FUNCTION, "referenced_function", LANG_C);
  cr_assert_not_null(symbol_node);
  cr_assert(ast_node_set_qualified_name(symbol_node, "referenced_function", AST_SOURCE_STATIC));
  cr_assert(ast_node_set_file_path(symbol_node, "test_file.c", AST_SOURCE_STATIC));
  symbol_node->range.start.line = 42;
  cr_assert_not_null(symbol_table_register(symbol_table, "referenced_function", symbol_node,
                                           "test_file.c", SCOPE_GLOBAL, LANG_C));
}

static void teardown_resolution(void) {
  ast_node_free(root_node);
  ast_node_free(symbol_node);
  reference_resolver_free(resolver);
  symbol_table_free(symbol_table);
  root_node = NULL;
  symbol_node = NULL;
  resolver = NULL;
  symbol_table = NULL;
}

Test(resolver_resolution, node_level, .init = setup_resolution, .fini = teardown_resolution) {
  ASTNode *func = ast_node_child_at(root_node, 0);
  ASTNode *call = ast_node_child_at(func, 0);
  size_t total = 0;
  size_t resolved = 0;

  cr_assert_eq(reference_resolver_resolve_node(resolver, call, REF_CALL, "referenced_function",
                                               LANG_C),
               RESOLUTION_SUCCESS);
  cr_assert_eq(call->num_references, 1);
  cr_assert_eq(call->references[0], symbol_node);

  reference_resolver_get_stats(resolver, &total, &resolved);
  cr_assert_eq(total, 1);
  cr_assert_eq(resolved, 1);
}

Test(resolver_resolution, scope_lookup, .init = setup_resolution, .fini = teardown_resolution) {
  ASTNode *func = ast_node_child_at(root_node, 0);
  ASTNode *call = ast_node_child_at(func, 0);

  cr_assert(ast_node_set_qualified_name(func, "module", AST_SOURCE_STATIC));
  cr_assert(symbol_table_add_scope(symbol_table, "module"));
  cr_assert_not_null(symbol_table_register(symbol_table, "module.scoped_function", symbol_node,
                                           "test_file.c", SCOPE_GLOBAL, LANG_C));

  cr_assert_eq(reference_resolver_generic_resolve(call, REF_CALL, "scoped_function", symbol_table),
               RESOLUTION_SUCCESS);
  cr_assert_eq(call->references[0], symbol_node);
}

Test(resolver_resolution, missing_symbol, .init = setup_resolution, .fini = teardown_resolution) {
  ASTNode *func = ast_node_child_at(root_node, 0);
  ASTNode *call = ast_node_child_at(func, 0);
  size_t total = 0;
  size_t resolved = 0;

  cr_assert_eq(reference_resolver_resolve_node(resolver, call, REF_CALL, "does_not_exist", LANG_C),
               RESOLUTION_NOT_FOUND);

  reference_resolver_get_stats(resolver, &total, &resolved);
  cr_assert_eq(total, 1);
  cr_assert_eq(resolved, 0);
}

Test(resolver_resolution, fuzzy_symbol, .init = setup_resolution, .fini = teardown_resolution) {
  ASTNode *func = ast_node_child_at(root_node, 0);
  ASTNode *call = ast_node_child_at(func, 0);

  cr_assert_eq(reference_resolver_resolve_node(resolver, call, REF_CALL, "referenced_functoin",
                                               LANG_C),
               RESOLUTION_SUCCESS);
  cr_assert_eq(call->num_references, 1);
  cr_assert_eq(call->references[0], symbol_node);
}

Test(resolver_resolution, include_lookup, .init = setup_resolution, .fini = teardown_resolution) {
  ASTNode *func = ast_node_child_at(root_node, 0);
  ASTNode *call = ast_node_child_at(func, 0);
  ASTNode *include_node = make_test_node(NODE_INCLUDE, "api.h", LANG_C);
  ASTNode *header_symbol = make_test_node(NODE_FUNCTION, "header_function", LANG_C);

  cr_assert_not_null(include_node);
  cr_assert_not_null(header_symbol);
  cr_assert(ast_node_add_child(root_node, include_node));
  cr_assert(ast_node_set_qualified_name(header_symbol, "header_function", AST_SOURCE_STATIC));
  cr_assert(ast_node_set_file_path(header_symbol, "include/api.h", AST_SOURCE_STATIC));
  cr_assert_not_null(symbol_table_register(symbol_table, "header_function", header_symbol,
                                           "include/api.h", SCOPE_GLOBAL, LANG_C));

  cr_assert_eq(reference_resolver_c(call, REF_CALL, "header_function", symbol_table, NULL),
               RESOLUTION_SUCCESS);
  cr_assert_eq(call->references[0], header_symbol);

  ast_node_free(header_symbol);
}

Test(resolver_resolution, include_type_lookup, .init = setup_resolution, .fini = teardown_resolution) {
  ASTNode *func = ast_node_child_at(root_node, 0);
  ASTNode *call = ast_node_child_at(func, 0);
  ASTNode *include_node = make_test_node(NODE_INCLUDE, "types.h", LANG_C);
  ASTNode *type_symbol = make_test_node(NODE_STRUCT, "header_type", LANG_C);

  cr_assert_not_null(include_node);
  cr_assert_not_null(type_symbol);
  cr_assert(ast_node_add_child(root_node, include_node));
  cr_assert(ast_node_set_qualified_name(type_symbol, "header_type", AST_SOURCE_STATIC));
  cr_assert(ast_node_set_file_path(type_symbol, "include/types.h", AST_SOURCE_STATIC));
  cr_assert_not_null(symbol_table_register(symbol_table, "header_type", type_symbol,
                                           "include/types.h", SCOPE_GLOBAL, LANG_C));

  cr_assert_eq(reference_resolver_c(call, REF_TYPE, "header_type", symbol_table, NULL),
               RESOLUTION_SUCCESS);
  cr_assert_eq(call->references[0], type_symbol);

  ast_node_free(type_symbol);
}
