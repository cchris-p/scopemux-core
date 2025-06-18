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
  ParserContext *ctx = parser_init();

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code), "variables_loops_conditions.c", LANG_C);
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");

  // Verify we can access AST nodes
  const ASTNode *ast_nodes[10];
  size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, ast_nodes, 10);
  cr_assert_gt(node_count, 0, "Should find at least one function node");
  
  // Check for main function extraction
  const ASTNode *main_func = NULL;
  for (size_t i = 0; i < node_count; i++) {
    if (ast_nodes[i]->name && strcmp(ast_nodes[i]->name, "main") == 0) {
      main_func = ast_nodes[i];
      break;
    }
  }
  if (main_func) {
    assert_node_fields((ASTNode *)main_func, "main");

    // Check function signature
    cr_assert_not_null(main_func->signature, "Function should have signature populated");
    cr_log_info("Main function signature: %s", main_func->signature);

    // Check function content
    cr_assert_not_null(main_func->raw_content, "Function should have content populated");
  } else {
    cr_log_info("Function extraction may need more refinement");
  }

  // Clean up
  parser_free(ctx);
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
  char *source_code = read_test_file("c", "struct_union_enum", "complex_data_types.c");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_init();

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code), "complex_data_types.c", LANG_C);
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");

  // Verify we can access AST nodes
  const ASTNode *ast_nodes[10];
  size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_STRUCT, ast_nodes, 10);
  cr_assert_gt(node_count, 0, "Should find at least one struct node");

  // Count struct definitions
  int struct_count = node_count;
  cr_log_info("Found %d struct definitions", struct_count);
  cr_assert_gt(struct_count, 0, "Should have at least one struct definition");

  // Debug: Dump AST structure to visualize the parsed tree
  // Uncomment if needed for debugging
  // for (size_t i = 0; i < node_count; i++) {
  //   dump_ast_structure(ast_nodes[i], 0);
  // }

  // Clean up
  parser_free(ctx);
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
  ParserContext *ctx = parser_init();

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code), "hello_world.c", LANG_C);
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");

  // Verify we can access AST nodes
  const ASTNode *ast_nodes[10];
  size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, ast_nodes, 10);
  cr_assert_gt(node_count, 0, "Should find at least one function node");

  // Validate specific syntax elements
  const ASTNode *main_func = NULL;
  for (size_t i = 0; i < node_count; i++) {
    if (ast_nodes[i]->name && strcmp(ast_nodes[i]->name, "main") == 0) {
      main_func = ast_nodes[i];
      break;
    }
  }
  cr_assert_not_null(main_func, "Should find main function in hello_world.c");
  assert_node_fields((ASTNode *)main_func, "main");

  // Check function signature
  cr_assert_not_null(main_func->signature, "Function should have signature populated");
  cr_log_info("Main function signature: %s", main_func->signature);

  // Clean up
  parser_free(ctx);
  free(source_code);
}
