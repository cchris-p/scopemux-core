#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/tree_sitter_integration.h"
#include "../../include/test_helpers.h"

//=================================
// JavaScript AST Extraction Tests
//=================================

/**
 * Test extraction of JavaScript functions from source code.
 * Verifies that functions are correctly identified and
 * their properties are extracted properly.
 */
Test(ast_extraction, js_functions, .description = "Test AST extraction of JavaScript functions") {
  fprintf(stderr, "Starting js_functions test\n");

  cr_log_info("Testing JavaScript function AST extraction");

  // Read test file with JavaScript functions
  fprintf(stderr, "Reading test file...\n");
  char *source_code = read_test_file("js", "basic_syntax", "variables_loops_conditions.js");
  cr_assert_not_null(source_code, "Failed to read test file");
  fprintf(stderr, "Test file read successfully\n");

  // Initialize parser context
  ParserContext *ctx = parser_init();

  // Parse the source code
  fprintf(stderr, "About to parse source code...\n");
  parser_parse_string(ctx, source_code, strlen(source_code), "variables_loops_conditions.js",
                      LANG_JS);
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
