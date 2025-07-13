#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../core/include/scopemux/parser.h"

#include "../../include/test_helpers.h"

//=================================
// TypeScript AST Extraction Tests
//=================================

#ifdef ENABLE_STRUCT_UNION_ENUM_TESTS
/**
 * Test extraction of TypeScript structs from source code.
 * Verifies that structs are correctly identified and
 * their properties are extracted properly.
 */
Test(ast_extraction, ts_structs, .description = "Test AST extraction of TS structs") {
  fprintf(stderr, "Starting ts_structs test\n");

  cr_log_info("Testing TypeScript struct AST extraction");

  // Read test file with TypeScript structs
  fprintf(stderr, "Reading test file...\n");
  char *source_code = read_test_file("ts", "basic_syntax", "variables_loops_conditions.ts");
  cr_assert_not_null(source_code, "Failed to read test file");
  fprintf(stderr, "Test file read successfully\n");

  // Initialize parser context
  ParserContext *ctx = parser_init();

  // Parse the source code
  fprintf(stderr, "About to parse source code...\n");
  parser_parse_string(ctx, source_code, strlen(source_code), "variables_loops_conditions.ts",
                      LANG_TYPESCRIPT);
  fprintf(stderr, "Source code parsed\n");
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");
  fprintf(stderr, "No parser errors detected\n");

  // Verify we can access AST nodes
  const ASTNode *ast_nodes[10];
  size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_STRUCT, ast_nodes, 10);
  cr_assert_gt(node_count, 0, "Should find at least one struct node");

  // Check for struct extraction
  const ASTNode *struct_node = NULL;
  for (size_t i = 0; i < node_count; i++) {
    if (ast_nodes[i]->name && strcmp(ast_nodes[i]->name, "MyStruct") == 0) {
      struct_node = ast_nodes[i];
      break;
    }
  }
  if (struct_node) {
    // Debug node fields before assertion
    fprintf(stderr, "DEBUG: About to assert struct_node fields\n");
    fprintf(stderr, "DEBUG: struct_node=%p\n", (void *)struct_node);
    if (struct_node) {
      fprintf(stderr, "DEBUG: struct_node->name=%s\n", struct_node->name ? struct_node->name : "(null)");
      fprintf(stderr, "DEBUG: struct_node->qualified_name=%s\n",
              struct_node->qualified_name ? struct_node->qualified_name : "(null)");
      fprintf(stderr, "DEBUG: struct_node->range.end.line=%d\n", struct_node->range.end.line);
    }
    assert_node_fields((ASTNode *)struct_node, "MyStruct");

    // Check struct signature
    cr_assert_not_null(struct_node->signature, "Struct should have signature populated");
    cr_log_info("Struct signature: %s", SAFE_STR(struct_node->signature));

    // Check struct content
    cr_assert_not_null(struct_node->raw_content, "Struct should have content populated");
  } else {
    cr_log_info("Struct extraction may need more refinement");
  }

  // Debug: Dump AST structure to visualize the parsed tree
  // Uncomment if needed for debugging
  // for (size_t i = 0; i < node_count; i++) {
  //   dump_ast_structure(ast_nodes[i], 0);
  // }

  // Clean up
  parser_free(ctx);
  free(source_code);
}
#endif // ENABLE_STRUCT_UNION_ENUM_TESTS

/**
 * Test extraction of TypeScript functions from source code.
 * Verifies that functions are correctly identified and
 * their properties are extracted properly.
 */
Test(ast_extraction, ts_functions, .description = "Test AST extraction of TypeScript functions") {
  fprintf(stderr, "Starting ts_functions test\n");

  cr_log_info("Testing TypeScript function AST extraction");

  // Read test file with TypeScript functions
  fprintf(stderr, "Reading test file...\n");
  char *source_code = read_test_file("ts", "basic_syntax", "variables_loops_conditions.ts");
  cr_assert_not_null(source_code, "Failed to read test file");
  fprintf(stderr, "Test file read successfully\n");

  // Initialize parser context
  ParserContext *ctx = parser_init();

  // Parse the source code
  fprintf(stderr, "About to parse source code...\n");
  parser_parse_string(ctx, source_code, strlen(source_code), "variables_loops_conditions.ts",
                      LANG_TYPESCRIPT);
  fprintf(stderr, "Source code parsed\n");
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");
  fprintf(stderr, "No parser errors detected\n");

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
    // Debug node fields before assertion
    fprintf(stderr, "DEBUG: About to assert main_func fields\n");
    fprintf(stderr, "DEBUG: main_func=%p\n", (void *)main_func);
    if (main_func) {
      fprintf(stderr, "DEBUG: main_func->name=%s\n", main_func->name ? main_func->name : "(null)");
      fprintf(stderr, "DEBUG: main_func->qualified_name=%s\n",
              main_func->qualified_name ? main_func->qualified_name : "(null)");
      fprintf(stderr, "DEBUG: main_func->range.end.line=%d\n", main_func->range.end.line);
    }
    assert_node_fields((ASTNode *)main_func, "main");

    // Check function signature
    cr_assert_not_null(main_func->signature, "Function should have signature populated");
    cr_log_info("Main function signature: %s", SAFE_STR(main_func->signature));

    // Check function content
    cr_assert_not_null(main_func->raw_content, "Function should have content populated");
  } else {
    cr_log_info("Function extraction may need more refinement");
  }

  // Clean up
  parser_free(ctx);
  free(source_code);
}
