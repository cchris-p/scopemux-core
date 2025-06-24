/**
 * @file cpp_basic_ast_tests.c
 * @brief Tests for C++ AST extraction
 */

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Fix include paths to correctly reference the header files
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/tree_sitter_integration.h"
#include "../../include/test_helpers.h"

//=================================
// C++ AST Extraction Tests
//=================================

/**
 * Test extraction of C++ functions from source code.
 * Verifies that functions are correctly identified and
 * their properties are extracted properly.
 */
Test(cpp_ast, functions, .description = "Test AST extraction of C++ functions") {

  cr_log_info("Testing C++ function AST extraction");

  // Read test file with C++ functions
  char *source_code = read_test_file("cpp", "basic_syntax", "hello_world.cpp");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Failed to initialize parser context");

  // Parse the source code
  bool parse_result =
      parser_parse_string(ctx, source_code, strlen(source_code), "hello_world.cpp", LANG_CPP);
  cr_assert(parse_result, "Parsing should succeed");
  cr_assert_null(parser_get_last_error(ctx), "Parser error: %s",
                 parser_get_last_error(ctx) ? parser_get_last_error(ctx) : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // TODO: Add specific C++ function tests once example files are created

  // Clean up
  parser_free(ctx);
  free(source_code);
}

/**
 * Test extraction of C++ classes from source code.
 * Verifies that class definitions are correctly identified
 * and their properties are extracted properly.
 */
Test(cpp_ast, classes, .description = "Test AST extraction of C++ classes") {

  cr_log_info("Testing C++ class AST extraction");

  // Read test file with C++ classes
  char *source_code = read_test_file("cpp", "basic_syntax", "variables_loops_conditions.cpp");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Failed to initialize parser context");

  // Parse the source code
  bool parse_result = parser_parse_string(ctx, source_code, strlen(source_code),
                                          "variables_loops_conditions.cpp", LANG_CPP);
  cr_assert(parse_result, "Parsing should succeed");
  cr_assert_null(parser_get_last_error(ctx), "Parser error: %s",
                 parser_get_last_error(ctx) ? parser_get_last_error(ctx) : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // TODO: Add specific C++ class tests once example files are created

  // Clean up
  parser_free(ctx);
  free(source_code);
}

/**
 * Test extraction of C++ templates from source code.
 * Verifies that template definitions are correctly identified
 * and their properties are extracted properly.
 */
Test(cpp_ast, templates, .description = "Test AST extraction of C++ templates") {

  cr_log_info("Testing C++ template AST extraction");

  // Read test file with C++ templates
  char *source_code = read_test_file("cpp", "templates", "templates_basics.cpp");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Failed to initialize parser context");

  // Parse the source code
  bool parse_result =
      parser_parse_string(ctx, source_code, strlen(source_code), "templates_basics.cpp", LANG_CPP);
  cr_assert(parse_result, "Parsing should succeed");
  cr_assert_null(parser_get_last_error(ctx), "Parser error: %s",
                 parser_get_last_error(ctx) ? parser_get_last_error(ctx) : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // TODO: Add specific C++ template tests once example files are created

  // Clean up
  parser_free(ctx);
  free(source_code);
}

// Add more C++-specific tests as needed
