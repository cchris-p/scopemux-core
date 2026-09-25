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
    "pub union Raw {\n"
    "    pub as_int: u32,\n"
    "    pub as_float: f32,\n"
    "}\n"
    "\n"
    "pub type Handle = usize;\n"
    "\n"
    "pub mod inner;\n"
    "\n"
    "macro_rules! make_worker {\n"
    "    () => { Worker { name: String::new(), count: 0 } };\n"
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

Test(rust_ast, scopes_methods_by_impl_type) {
  static const char *RUST_SCOPING_SAMPLE =
      "struct Point { x: i32 }\n"
      "struct Vector { x: i32 }\n"
      "\n"
      "impl Point {\n"
      "    fn origin() -> Point { Point { x: 0 } }\n"
      "}\n"
      "\n"
      "impl Vector {\n"
      "    fn origin() -> Vector { Vector { x: 0 } }\n"
      "}\n";

  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Parser context should be created");
  parser_parse_string(ctx, RUST_SCOPING_SAMPLE, strlen(RUST_SCOPING_SAMPLE), "scoping.rs",
                      LANG_RUST);
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");

  const ASTNode *classes[8];
  size_t class_count = parser_get_ast_nodes_by_type(ctx, NODE_CLASS, classes, 8);
  cr_assert_geq(class_count, 2, "Both impl blocks should be extracted as class containers");

  const ASTNode *methods[8];
  size_t method_count = parser_get_ast_nodes_by_type(ctx, NODE_METHOD, methods, 8);
  cr_assert_eq(method_count, 2, "Both impl methods should be extracted");

  bool saw_point_origin = false;
  bool saw_vector_origin = false;
  for (size_t i = 0; i < method_count; i++) {
    cr_assert_not_null(methods[i]->qualified_name, "Method should have a qualified name");
    if (strstr(methods[i]->qualified_name, "Point.origin") != NULL) {
      saw_point_origin = true;
    }
    if (strstr(methods[i]->qualified_name, "Vector.origin") != NULL) {
      saw_vector_origin = true;
    }
  }
  cr_assert(saw_point_origin, "Point::origin should be scoped under Point, not colliding");
  cr_assert(saw_vector_origin, "Vector::origin should be scoped under Vector, not colliding");

  parser_free(ctx);
}

Test(rust_ast, detects_language_from_extension) {
  cr_assert_eq(language_detect_from_extension("lib.rs"), LANG_RUST,
               ".rs should be detected as Rust");
  cr_assert_str_eq(language_to_string(LANG_RUST), "rust");
  cr_assert_eq(language_from_string("rust"), LANG_RUST);
}

Test(rust_ast, extracts_call_sites_and_macro_invocations) {
  static const char *RUST_CALLS_SAMPLE =
      "fn helper(value: usize) -> usize {\n"
      "    value + 1\n"
      "}\n"
      "\n"
      "fn caller() {\n"
      "    let _ = helper(1);\n"
      "    println!(\"done\");\n"
      "}\n";

  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Parser context should be created");
  parser_parse_string(ctx, RUST_CALLS_SAMPLE, strlen(RUST_CALLS_SAMPLE), "calls.rs", LANG_RUST);
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");

  const ASTNode *nodes[32];
  size_t count = parser_get_ast_nodes_by_type(ctx, NODE_IDENTIFIER, nodes, 32);
  bool saw_helper_call = false;
  bool saw_macro_call = false;

  for (size_t i = 0; i < count; i++) {
    if (!nodes[i]->name) {
      continue;
    }
    if (strcmp(nodes[i]->name, "helper") == 0) {
      saw_helper_call = true;
      cr_assert_not_null(nodes[i]->parent, "Call site should have a parent");
      cr_assert_eq(nodes[i]->parent->type, NODE_FUNCTION,
                   "Call site should be owned by the containing function");
    }
    if (strcmp(nodes[i]->name, "println") == 0) {
      saw_macro_call = true;
    }
  }

  cr_assert(saw_helper_call, "Should extract the `helper(...)` call site");
  cr_assert(saw_macro_call, "Should extract the `println!` macro invocation");

  parser_free(ctx);
}

Test(rust_ast, parses_enums_traits_and_other_kinds) {
  ParserContext *ctx = parse_rust_sample();
  const ASTNode *nodes[16];

  cr_assert_gt(parser_get_ast_nodes_by_type(ctx, NODE_ENUM, nodes, 16), 0,
               "Should extract a Rust enum");
  cr_assert_gt(parser_get_ast_nodes_by_type(ctx, NODE_INTERFACE, nodes, 16), 0,
               "Should extract a Rust trait as an interface");
  cr_assert_gt(parser_get_ast_nodes_by_type(ctx, NODE_TYPEDEF, nodes, 16), 0,
               "Should extract a Rust type alias as a typedef");
  cr_assert_gt(parser_get_ast_nodes_by_type(ctx, NODE_MODULE, nodes, 16), 0,
               "Should extract a Rust module");
  cr_assert_gt(parser_get_ast_nodes_by_type(ctx, NODE_UNION, nodes, 16), 0,
               "Should extract a Rust union");
  cr_assert_gt(parser_get_ast_nodes_by_type(ctx, NODE_MACRO, nodes, 16), 0,
               "Should extract a Rust macro definition");

  size_t enum_count = parser_get_ast_nodes_by_type(ctx, NODE_ENUM, nodes, 16);
  bool saw_state = false;
  for (size_t i = 0; i < enum_count; i++) {
    if (nodes[i]->name && strcmp(nodes[i]->name, "State") == 0) {
      saw_state = true;
    }
  }
  cr_assert(saw_state, "Extracted enum node should be named `State`");

  parser_free(ctx);
}
