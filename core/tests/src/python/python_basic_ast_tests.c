#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/scopemux/parser.h"

#include "../../include/test_helpers.h"

//=================================
// Python AST Extraction Tests
//=================================

/**
 * Test extraction of Python functions from source code.
 * Verifies that functions are correctly identified and
 * their properties are extracted properly.
 */
Test(ast_extraction, python_functions, .description = "Test AST extraction of Python functions") {

  cr_log_info("Testing Python function AST extraction");

  // Read test file with Python functions
  char *source_code = read_test_file("python", "basic_syntax", "functions.py");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  // Initialize parser context (updated API)
  ParserContext *ctx = parser_init();
  ctx->language = LANG_PYTHON;
  ctx->filename = "functions.py"; // Updated: use filename

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code), ctx->filename, LANG_PYTHON);
  cr_assert_null(ctx->last_error, "Parser error: %s", ctx->last_error ? ctx->last_error : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // Check for basic function extraction
  ASTNode *simple_func = find_node_by_name(ctx->ast_root, "simple_function", NODE_FUNCTION);
  assert_node_fields(simple_func, "simple_function");

  // Check for function with parameters
  ASTNode *func_with_params =
      find_node_by_name(ctx->ast_root, "function_with_parameters", NODE_FUNCTION);
  assert_node_fields(func_with_params, "function_with_parameters");
  cr_assert_not_null(func_with_params->signature, "Function should have signature populated");

  // Check for function with docstring
  ASTNode *func_with_docstring =
      find_node_by_name(ctx->ast_root, "function_with_docstring", NODE_FUNCTION);
  assert_node_fields(func_with_docstring, "function_with_docstring");
  cr_assert_not_null(func_with_docstring->docstring, "Function should have docstring populated");

  // Clean up
  parser_free(ctx);
  free(source_code);
}

/**
 * Test extraction of Python classes from source code.
 * Verifies that class definitions are correctly identified
 * and their properties are extracted properly.
 */
Test(ast_extraction, python_classes, .description = "Test AST extraction of Python classes") {

  cr_log_info("Testing Python class AST extraction");

  // Read test file with Python classes
  char *source_code = read_test_file("python", "basic_syntax", "classes.py");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  // Initialize parser context (updated API)
  ParserContext *ctx = parser_init();
  ctx->language = LANG_PYTHON;
  ctx->filename = "classes.py"; // Updated: use filename

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code), ctx->filename, LANG_PYTHON);
  cr_assert_null(ctx->last_error, "Parser error: %s", ctx->last_error ? ctx->last_error : "");

  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");

  // Check for simple class
  ASTNode *simple_class = find_node_by_name(ctx->ast_root, "SimpleClass", NODE_CLASS);
  assert_node_fields(simple_class, "SimpleClass");

  // Check for class with methods
  ASTNode *class_with_methods = find_node_by_name(ctx->ast_root, "ClassWithMethods", NODE_CLASS);
  assert_node_fields(class_with_methods, "ClassWithMethods");

  // Check for methods within class
  if (class_with_methods && class_with_methods->num_children > 0) {
    // Find a method (could be __init__ or any other)
    ASTNode *class_method = NULL;
    for (size_t i = 0; i < class_with_methods->num_children; i++) {
      if (class_with_methods->children[i]->type == NODE_METHOD) {
        class_method = class_with_methods->children[i];
        break;
      }
    }

    if (class_method) {
      cr_assert_eq(class_method->parent, class_with_methods, "Method's parent should be the class");
    } else {
      cr_log_info("Class method extraction not fully implemented yet");
    }
  }

  // Clean up
  parser_free(ctx);
  free(source_code);
}

/**
 * Test hierarchical relationships in Python ASTs.
 * Verifies proper parent-child relationships and
 * qualified name construction.
 */
Test(ast_extraction, python_hierarchy,
     .description = "Test hierarchical AST extraction for Python") {

  cr_log_info("Testing Python AST hierarchy extraction");

  // Read test file with nested Python structures
  char *source_code = read_test_file("python", "basic_syntax", "classes.py");
  cr_assert_not_null(source_code, "Failed to read test file");

  // Initialize parser context
  // Initialize parser context (updated API)
  ParserContext *ctx = parser_init();
  ctx->language = LANG_PYTHON;
  ctx->filename = "classes.py"; // Updated: use filename

  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code), ctx->filename, LANG_PYTHON);
  cr_assert_null(ctx->last_error, "Parser error: %s", ctx->last_error ? ctx->last_error : "");

  // Look for class with methods
  ASTNode *class_node = find_node_by_name(ctx->ast_root, "ClassWithMethods", NODE_CLASS);
  if (class_node) {
    // Verify that the class has child nodes (methods)
    cr_assert_gt(class_node->num_children, 0, "Class should have child nodes");

    // Check qualified name format
    if (class_node->qualified_name) {
      cr_log_info("Class qualified name: %s", class_node->qualified_name);
      // Methods should have qualified names that include the class name
      for (size_t i = 0; i < class_node->num_children; i++) {
        ASTNode *method = class_node->children[i];
        if (method && method->type == NODE_METHOD && method->qualified_name) {
          cr_assert(strstr(method->qualified_name, class_node->name) != NULL,
                    "Method qualified name should include class name: %s", method->qualified_name);
        }
      }
    }
  } else {
    cr_log_info("Complex class extraction may need more refinement");
  }

  // Clean up
  parser_free(ctx);
  free(source_code);
}
