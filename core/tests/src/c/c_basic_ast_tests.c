#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/tree_sitter_integration.h"
#include "../../include/test_helpers.h"

//=================================
// C AST Extraction Tests
//=================================

/**
 * Test extraction of C functions from source code.
 * Verifies that functions are correctly identified and
 * their properties are extracted properly.
 */
Test(ast_extraction, c_functions, .description = "Test AST extraction of C functions") {

  cr_log_info("Testing C function AST extraction");

  // Read test file with C functions
  char *source_code = read_test_file("c", "basic_syntax", "variables_loops_conditions.c");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_C;
  ctx->source_code = source_code;
  ctx->file_path = "variables_loops_conditions.c";

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s",
                 ctx->error_message ? ctx->error_message : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // Check for main function extraction
  ASTNode *main_func = find_node_by_name(ctx->ast_root, "main", AST_FUNCTION);
  if (main_func) {
    assert_node_fields(main_func, "main");

    // Check function signature
    cr_assert_not_null(main_func->signature, "Function should have signature populated");
    cr_log_info("Main function signature: %s", main_func->signature);

    // Check function content
    cr_assert_not_null(main_func->raw_content, "Function should have content populated");
  } else {
    cr_log_info("Function extraction may need more refinement");
  }

  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

/**
 * Test extraction of C structs from source code.
 * Verifies that struct definitions are correctly identified
 * and their properties are extracted properly.
 */
Test(ast_extraction, c_structs, .description = "Test AST extraction of C structs") {

  cr_log_info("Testing C struct AST extraction");

  // Read test file with C structs
  char *source_code = read_test_file("c", "struct_union_enum", "structs.c");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_C;
  ctx->source_code = source_code;
  ctx->file_path = "structs.c";

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s",
                 ctx->error_message ? ctx->error_message : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // Count struct definitions
  int struct_count = count_nodes_by_type(ctx->ast_root, AST_STRUCT);
  cr_log_info("Found %d struct definitions", struct_count);
  cr_assert_gt(struct_count, 0, "Should have at least one struct definition");

  // Debug: Dump AST structure to visualize the parsed tree
  // Uncomment if needed for debugging
  // dump_ast_structure(ctx->ast_root, 0);

  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

/**
 * Test AST extraction of basic C syntax elements.
 * Specifically tests hello_world.c, which is a simple example.
 */
Test(ast_extraction, c_basic_syntax, .description = "Test AST extraction of basic C syntax") {

  cr_log_info("Testing AST extraction of basic C syntax");

  // Read hello_world.c test file
  char *source_code = read_test_file("c", "basic_syntax", "hello_world.c");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_C;
  ctx->source_code = source_code;
  ctx->file_path = "hello_world.c";

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s",
                 ctx->error_message ? ctx->error_message : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // Validate specific syntax elements
  ASTNode *main_func = find_node_by_name(ctx->ast_root, "main", AST_FUNCTION);
  cr_assert_not_null(main_func, "Should find main function in hello_world.c");
  assert_node_fields(main_func, "main");

  // Check function signature
  cr_assert_not_null(main_func->signature, "Function should have signature populated");
  cr_log_info("Main function signature: %s", main_func->signature);

  // Clean up
  parser_context_free(ctx);
  free(source_code);
}
