#include "reference_resolver_private.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/reference_resolver_internal.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>

static ReferenceResolver *resolver;
static GlobalSymbolTable *symbol_table;
static bool cleanup_called;

static ResolutionStatus first_resolver(ASTNode *node, ReferenceType ref_type, const char *name,
                                       GlobalSymbolTable *table, void *data) {
  (void)node;
  (void)ref_type;
  (void)name;
  (void)table;
  (void)data;
  return RESOLUTION_SUCCESS;
}

static ResolutionStatus second_resolver(ASTNode *node, ReferenceType ref_type, const char *name,
                                        GlobalSymbolTable *table, void *data) {
  (void)node;
  (void)ref_type;
  (void)name;
  (void)table;
  (void)data;
  return RESOLUTION_NOT_FOUND;
}

static ResolutionStatus custom_data_resolver(ASTNode *node, ReferenceType ref_type, const char *name,
                                             GlobalSymbolTable *table, void *data) {
  int *value = data;
  (void)node;
  (void)ref_type;
  (void)table;
  return (value && *value == 42 && strcmp(name, "custom_data_function") == 0)
             ? RESOLUTION_SUCCESS
             : RESOLUTION_NOT_FOUND;
}

static void custom_cleanup(void *data) {
  cleanup_called = true;
  free(data);
}

static void setup_registration(void) {
  cleanup_called = false;
  symbol_table = symbol_table_create(16);
  cr_assert_not_null(symbol_table);
  resolver = reference_resolver_create(symbol_table);
  cr_assert_not_null(resolver);
}

static void teardown_registration(void) {
  reference_resolver_free(resolver);
  symbol_table_free(symbol_table);
  resolver = NULL;
  symbol_table = NULL;
}

Test(resolver_registration, find_language_resolver, .init = setup_registration,
     .fini = teardown_registration) {
  LanguageResolver *lang_resolver;

  cr_assert(reference_resolver_register_impl(resolver, LANG_C, first_resolver, NULL, NULL));
  lang_resolver = find_language_resolver_impl(resolver, LANG_C);
  cr_assert_not_null(lang_resolver);
  cr_assert_eq(lang_resolver->resolver_func, first_resolver);
}

Test(resolver_registration, init_builtin, .init = setup_registration,
     .fini = teardown_registration) {
  cr_assert(reference_resolver_init_builtin(resolver));
  cr_assert_not_null(find_language_resolver_impl(resolver, LANG_C));
  cr_assert_not_null(find_language_resolver_impl(resolver, LANG_PYTHON));
  cr_assert_not_null(find_language_resolver_impl(resolver, LANG_JAVASCRIPT));
  cr_assert_not_null(find_language_resolver_impl(resolver, LANG_TYPESCRIPT));
}

Test(resolver_registration, resolver_priority, .init = setup_registration,
     .fini = teardown_registration) {
  ASTNode *node = make_test_node(NODE_FUNCTION, "test_function", LANG_C);

  cr_assert(reference_resolver_register_impl(resolver, LANG_C, first_resolver, NULL, NULL));
  cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "test_name", LANG_C),
               RESOLUTION_SUCCESS);

  cr_assert(reference_resolver_register_impl(resolver, LANG_C, second_resolver, NULL, NULL));
  cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "test_name", LANG_C),
               RESOLUTION_NOT_FOUND);
  ast_node_free(node);
}

Test(resolver_registration, resolver_with_custom_data, .init = setup_registration,
     .fini = teardown_registration) {
  ASTNode *node = make_test_node(NODE_FUNCTION, "test_function", LANG_C);
  int *value = malloc(sizeof(int));
  cr_assert_not_null(value);
  *value = 42;

  cr_assert(reference_resolver_register_impl(resolver, LANG_C, custom_data_resolver, value,
                                             custom_cleanup));
  cr_assert_eq(reference_resolver_resolve_node(resolver, node, REF_CALL, "custom_data_function",
                                               LANG_C),
               RESOLUTION_SUCCESS);

  reference_resolver_free(resolver);
  resolver = NULL;
  cr_assert(cleanup_called);
  ast_node_free(node);
}
