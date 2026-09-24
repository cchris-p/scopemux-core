#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../core/include/scopemux/parser.h"

#include "../../include/test_helpers.h"

//=================================
// Rust AST Extraction Tests
//=================================

static const char *RUST_SAMPLE =
    "use std::collections::HashMap;\n"
    "\n"
    "pub const MAX_ITEMS: usize = 16;\n"
    "\n"
    "pub struct Worker {\n"
    "    pub name: String,\n"
    "    pub count: usize,\n"
    "}\n"
    "\n"
    "pub enum State {\n"
    "    Idle,\n"
    "    Busy,\n"
    "}\n"
    "\n"
    "pub trait Runnable {\n"
    "    fn run(&self);\n"
    "}\n"
    "\n"
    "impl Worker {\n"
    "    pub fn new(name: String) -> Worker {\n"
    "        Worker { name, count: 0 }\n"
    "    }\n"
    "}\n"
    "\n"
    "pub fn helper(value: usize) -> usize {\n"
    "    value + 1\n"
    "}\n";

static ParserContext *parse_rust_sample(void) {
  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Parser context should be created");

  parser_parse_string(ctx, RUST_SAMPLE, strlen(RUST_SAMPLE), "sample.rs", LANG_RUST);

  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");
  return ctx;
}

Test(rust_ast, parses_functions) {
  ParserContext *ctx = parse_rust_sample();
  const ASTNode *nodes[16];
  size_t count = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, nodes, 16);
  cr_assert_gt(count, 0, "Should extract at least one Rust function");

  bool saw_helper = false;
  for (size_t i = 0; i < count; i++) {
    if (nodes[i]->name && strcmp(nodes[i]->name, "helper") == 0) {
      saw_helper = true;
    }
  }
  cr_assert(saw_helper, "Should extract the `helper` function");

  parser_free(ctx);
}

Test(rust_ast, parses_structs_methods_and_variables) {
  ParserContext *ctx = parse_rust_sample();
  const ASTNode *nodes[16];

  size_t struct_count = parser_get_ast_nodes_by_type(ctx, NODE_STRUCT, nodes, 16);
  cr_assert_gt(struct_count, 0, "Should extract a Rust struct");

  size_t method_count = parser_get_ast_nodes_by_type(ctx, NODE_METHOD, nodes, 16);
  cr_assert_gt(method_count, 0, "Should extract a Rust method from an impl block");

  size_t variable_count = parser_get_ast_nodes_by_type(ctx, NODE_VARIABLE, nodes, 16);
  cr_assert_gt(variable_count, 0, "Should extract a Rust const/static as a variable");

  parser_free(ctx);
}

Test(rust_ast, detects_language_from_extension) {
  cr_assert_eq(language_detect_from_extension("lib.rs"), LANG_RUST,
               ".rs should be detected as Rust");
  cr_assert_str_eq(language_to_string(LANG_RUST), "rust");
  cr_assert_eq(language_from_string("rust"), LANG_RUST);
}
