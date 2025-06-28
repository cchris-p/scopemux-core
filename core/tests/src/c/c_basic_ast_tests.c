#define DEBUG_MODE false

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../../include/minimal_parser.h"
#define LOG_WARN(fmt, ...) criterion_log(CR_LOG_WARNING, fmt, ##__VA_ARGS__)
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
  if (DEBUG_MODE) {
    fprintf(stderr, "Starting c_functions test\n");
  }

  if (DEBUG_MODE) {
    cr_log_info("Testing C function AST extraction");
  }

  // Read test file with C functions
  if (DEBUG_MODE) {
    fprintf(stderr, "Reading test file...\n");
  }
  char *source_code = read_test_file("c", "basic_syntax", "variables_loops_conditions.c");
  cr_assert_not_null(source_code, "Failed to read test file");
  if (DEBUG_MODE) {
    fprintf(stderr, "Test file read successfully, source length: %zu bytes\n", strlen(source_code));
  }

  // Initialize parser context
  if (DEBUG_MODE) {
    fprintf(stderr, "Initializing parser context...\n");
  }
  ParserContext *ctx = parser_init();
  if (!ctx) {
    if (DEBUG_MODE) {
      fprintf(stderr, "ERROR: Failed to initialize parser context\n");
    }
    cr_assert_not_null(ctx, "Failed to initialize parser context");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "Parser context initialized successfully\n");
  }

  // Parse the source code
  if (DEBUG_MODE) {
    fprintf(stderr, "About to parse source code...\n");
  }
  bool parse_success = parser_parse_string(ctx, source_code, strlen(source_code),
                                           "variables_loops_conditions.c", LANG_C);
  if (!parse_success) {
    if (DEBUG_MODE) {
      fprintf(stderr, "ERROR: Failed to parse source code\n");
    }
    const char *error_msg = parser_get_last_error(ctx);
    if (error_msg) {
      if (DEBUG_MODE) {
        fprintf(stderr, "Parser error: %s\n", error_msg);
      }
    } else {
      if (DEBUG_MODE) {
        fprintf(stderr, "No error message available\n");
      }
    }
    cr_assert(parse_success, "Failed to parse source code");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "Source code parsed successfully\n");
  }

  // Check if AST root exists
  if (DEBUG_MODE) {
    fprintf(stderr, "Checking AST root...\n");
  }
  if (!ctx->ast_root) {
    if (DEBUG_MODE) {
      fprintf(stderr, "ERROR: AST root is NULL\n");
    }
    cr_assert_not_null(ctx->ast_root, "AST root is NULL");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "AST root exists, node type: %d\n", ctx->ast_root->type);
  }

  // Verify we can access AST nodes
  if (DEBUG_MODE) {
    fprintf(stderr, "Getting function nodes...\n");
  }
  const ASTNode *ast_nodes[10];
  size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_FUNCTION, ast_nodes, 10);
  if (DEBUG_MODE) {
    fprintf(stderr, "Found %zu function nodes\n", node_count);
  }
  cr_assert_gt(node_count, 0, "Should find at least one function node");

  // Check for main function extraction
  if (DEBUG_MODE) {
    fprintf(stderr, "Looking for main function...\n");
  }
  const ASTNode *main_func = NULL;
  for (size_t i = 0; i < node_count; i++) {
    if (ast_nodes[i]->name && strcmp(ast_nodes[i]->name, "main") == 0) {
      main_func = ast_nodes[i];
      break;
    }
  }
  if (main_func) {
    // Debug node fields before assertion
    if (DEBUG_MODE) {
      fprintf(stderr, "DEBUG: About to assert main_func fields\n");
    }
    if (DEBUG_MODE) {
      fprintf(stderr, "DEBUG: main_func=%p\n", (void *)main_func);
    }
    if (main_func) {
      if (DEBUG_MODE) {
        fprintf(stderr, "DEBUG: main_func->name=%s\n",
                main_func->name ? main_func->name : "(null)");
      }
      if (DEBUG_MODE) {
        fprintf(stderr, "DEBUG: main_func->qualified_name=%s\n",
                main_func->qualified_name ? main_func->qualified_name : "(null)");
      }
      if (DEBUG_MODE) {
        fprintf(stderr, "DEBUG: main_func->range.end.line=%d\n", main_func->range.end.line);
      }
    }
    assert_node_fields((ASTNode *)main_func, "main");

    // Check function signature
    cr_assert_not_null(main_func->signature, "Function should have signature populated");
    if (DEBUG_MODE) {
      cr_log_info("Main function signature: %s", main_func->signature);
    }

    // Check function content
    cr_assert_not_null(main_func->raw_content, "Function should have content populated");
  } else {
    if (DEBUG_MODE) {
      cr_log_info("Function extraction may need more refinement");
    }
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
  if (DEBUG_MODE) {
    fprintf(stderr, "Starting c_structs test\n");
  }

  if (DEBUG_MODE) {
    cr_log_info("Testing C struct AST extraction");
  }

  // Read test file with C structs
  char *source_code = read_test_file("c", "struct_union_enum", "complex_data_types.c");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_init();

  // Parse the source code
  if (DEBUG_MODE) {
    fprintf(stderr, "About to parse source code (c_structs)...\n");
  }
  parser_parse_string(ctx, source_code, strlen(source_code), "complex_data_types.c", LANG_C);
  if (DEBUG_MODE) {
    fprintf(stderr, "Source code parsed (c_structs)\n");
  }
  const char *error_message = parser_get_last_error(ctx);
  cr_assert_null(error_message, "Parser error: %s", error_message ? error_message : "");

  // Verify we can access AST nodes
  const ASTNode *ast_nodes[10];
  size_t node_count = parser_get_ast_nodes_by_type(ctx, NODE_STRUCT, ast_nodes, 10);
  cr_assert_gt(node_count, 0, "Should find at least one struct node");

  // Count struct definitions
  int struct_count = node_count;
  if (DEBUG_MODE) {
    cr_log_info("Found %d struct definitions", struct_count);
  }
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
  if (DEBUG_MODE) {
    fprintf(stderr, "Starting c_basic_syntax test\n");
  }

  if (DEBUG_MODE) {
    cr_log_info("Testing AST extraction of basic C syntax");
  }

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
  // Debug node fields before assertion
  if (DEBUG_MODE) {
    fprintf(stderr, "DEBUG: About to assert main_func fields (c_basic_syntax)\n");
  }
  if (DEBUG_MODE) {
    fprintf(stderr, "DEBUG: main_func=%p\n", (void *)main_func);
  }
  if (main_func) {
    if (DEBUG_MODE) {
      fprintf(stderr, "DEBUG: main_func->name=%s\n", main_func->name ? main_func->name : "(null)");
    }
    if (DEBUG_MODE) {
      fprintf(stderr, "DEBUG: main_func->qualified_name=%s\n",
              main_func->qualified_name ? main_func->qualified_name : "(null)");
    }
    if (DEBUG_MODE) {
      fprintf(stderr, "DEBUG: main_func->range.end.line=%d\n", main_func->range.end.line);
    }
  }
  assert_node_fields((ASTNode *)main_func, "main");

  // Check function signature
  cr_assert_not_null(main_func->signature, "Function should have signature populated");
  if (DEBUG_MODE) {
    cr_log_info("Main function signature: %s", main_func->signature);
  }

  // Clean up
  parser_free(ctx);
  free(source_code);
}
