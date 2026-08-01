#include "reference_resolver_private.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/reference_resolver_internal.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>

static ReferenceResolver *resolver;
static GlobalSymbolTable *symbol_table;

static ResolutionStatus success_resolver(ASTNode *node, ReferenceType ref_type, const char *name,
                                         GlobalSymbolTable *table, void *data) {
  (void)node;
  (void)ref_type;
  (void)name;
  (void)table;
  (void)data;
  return RESOLUTION_SUCCESS;
}

static ResolutionStatus not_found_resolver(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *table, void *data) {
  (void)node;
  (void)ref_type;
  (void)name;
  (void)table;
  (void)data;
  return RESOLUTION_NOT_FOUND;
}

static void setup_resolver(void) {
  symbol_table = symbol_table_create(16);
  cr_assert_not_null(symbol_table);
  resolver = reference_resolver_create(symbol_table);
  cr_assert_not_null(resolver);
}

static void teardown_resolver(void) {
  reference_resolver_free(resolver);
  symbol_table_free(symbol_table);
  resolver = NULL;
  symbol_table = NULL;
}

Test(resolver_core, create, .init = setup_resolver, .fini = teardown_resolver) {
  cr_assert_not_null(resolver);
  cr_assert_eq(resolver->num_resolvers, 0);
}

Test(resolver_core, register_resolver, .init = setup_resolver, .fini = teardown_resolver) {
  ASTNode *node = make_test_node(NODE_FUNCTION, "test_function", LANG_C);
  cr_assert(reference_resolver_register_impl(resolver, LANG_C, success_resolver, NULL, NULL));
  cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "test_name", LANG_C),
               RESOLUTION_SUCCESS);
  ast_node_free(node);
}

Test(resolver_core, register_replacement, .init = setup_resolver, .fini = teardown_resolver) {
  ASTNode *node = make_test_node(NODE_FUNCTION, "test_function", LANG_C);
  cr_assert(reference_resolver_register_impl(resolver, LANG_C, success_resolver, NULL, NULL));
  cr_assert(reference_resolver_register_impl(resolver, LANG_C, not_found_resolver, NULL, NULL));
  cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "test_name", LANG_C),
               RESOLUTION_NOT_FOUND);
  ast_node_free(node);
}

Test(resolver_core, unregister_resolver, .init = setup_resolver, .fini = teardown_resolver) {
  ASTNode *node = make_test_node(NODE_FUNCTION, "test_function", LANG_C);
  cr_assert(reference_resolver_register_impl(resolver, LANG_C, success_resolver, NULL, NULL));
  cr_assert(reference_resolver_unregister(resolver, LANG_C));
  cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "missing", LANG_C),
               RESOLUTION_NOT_FOUND);
  ast_node_free(node);
}

Test(resolver_core, get_stats, .init = setup_resolver, .fini = teardown_resolver) {
  size_t total = 0;
  size_t resolved = 0;
  ASTNode *node = make_test_node(NODE_FUNCTION, "test_function", LANG_C);

  cr_assert(reference_resolver_register_impl(resolver, LANG_C, success_resolver, NULL, NULL));
  for (int i = 0; i < 5; i++) {
    cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "test_name", LANG_C),
                 RESOLUTION_SUCCESS);
  }

  reference_resolver_get_stats(resolver, &total, &resolved);
  cr_assert_eq(total, 5);
  cr_assert_eq(resolved, 5);
  ast_node_free(node);
}
