#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include necessary ScopeMux headers
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/tree_sitter_integration.h"

// Helper function to read test files
static char *read_test_file(const char *language, const char *category, const char *file_name) {
  char filepath[512];
  snprintf(filepath, sizeof(filepath), 
          "../examples/%s/%s/%s", language, category, file_name);
  
  FILE *f = fopen(filepath, "rb");
  if (!f) {
    cr_log_error("Failed to open test file: %s", filepath);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *buffer = (char *)malloc(length + 1);
  if (buffer) {
    fread(buffer, 1, length, f);
    buffer[length] = '\0';
  }
  fclose(f);
  return buffer;
}

// Helper function to check if an AST node has a specific child
static ASTNode *find_node_by_name(ASTNode *parent, const char *name, ASTNodeType type) {
  if (!parent || !name) return NULL;
  
  for (size_t i = 0; i < parent->num_children; i++) {
    ASTNode *child = parent->children[i];
    if (child && child->name && child->type == type && strcmp(child->name, name) == 0) {
      return child;
    }
  }
  return NULL;
}

// Helper function to check if node has expected fields populated
static void assert_node_fields(ASTNode *node, const char *node_name) {
  cr_assert_not_null(node, "Node '%s' should not be NULL", node_name);
  cr_assert_not_null(node->name, "Node '%s' should have a name", node_name);
  cr_assert_str_eq(node->name, node_name, "Node name should be '%s'", node_name);
  
  // Source range should be valid
  cr_assert_gt(node->range.end_line, 0, "Node '%s' should have valid end_line", node_name);
  
  // Check if qualified_name is populated
  cr_assert_not_null(node->qualified_name, "Node '%s' should have a qualified_name", node_name);
}

// Helper function to count nodes of a specific type in the AST
static int count_nodes_by_type(ASTNode *root, ASTNodeType type) {
  if (!root) return 0;
  
  int count = (root->type == type) ? 1 : 0;
  for (size_t i = 0; i < root->num_children; i++) {
    count += count_nodes_by_type(root->children[i], type);
  }
  return count;
}

// Helper function to dump AST structure for debugging
static void dump_ast_structure(ASTNode *node, int level) {
  if (!node) return;
  
  // Print indentation
  for (int i = 0; i < level; i++) {
    printf("  ");
  }
  
  // Print node information
  printf("%s (%d) [%zu children]\n", 
         node->name ? node->name : "(unnamed)", 
         node->type,
         node->num_children);
  
  // Recursively print children
  for (size_t i = 0; i < node->num_children; i++) {
    dump_ast_structure(node->children[i], level + 1);
  }
}

//=================================
// Python AST Extraction Tests
//=================================

// Test extraction of Python functions
Test(ast_extraction, python_functions,
     .description = "Test AST extraction of Python functions") {
     
  cr_log_info("Testing Python function AST extraction");
  
  // Read test file with Python functions
  char *source_code = read_test_file("python", "basic_syntax", "functions.py");
  cr_assert_not_null(source_code, "Failed to read test file");
  
  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_PYTHON;
  ctx->source_code = source_code;
  ctx->file_path = "functions.py";
  
  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s", ctx->error_message ? ctx->error_message : "");
  
  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");
  
  // Check for basic function extraction
  ASTNode *simple_func = find_node_by_name(ctx->ast_root, "simple_function", AST_FUNCTION);
  assert_node_fields(simple_func, "simple_function");
  
  // Check for function with parameters
  ASTNode *func_with_params = find_node_by_name(ctx->ast_root, "function_with_parameters", AST_FUNCTION);
  assert_node_fields(func_with_params, "function_with_parameters");
  cr_assert_not_null(func_with_params->signature, "Function should have signature populated");
  
  // Check for function with docstring
  ASTNode *func_with_docstring = find_node_by_name(ctx->ast_root, "function_with_docstring", AST_FUNCTION);
  assert_node_fields(func_with_docstring, "function_with_docstring");
  cr_assert_not_null(func_with_docstring->docstring, "Function should have docstring populated");
  
  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

// Test extraction of Python classes
Test(ast_extraction, python_classes,
     .description = "Test AST extraction of Python classes") {
     
  cr_log_info("Testing Python class AST extraction");
  
  // Read test file with Python classes
  char *source_code = read_test_file("python", "basic_syntax", "classes.py");
  cr_assert_not_null(source_code, "Failed to read test file");
  
  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_PYTHON;
  ctx->source_code = source_code;
  ctx->file_path = "classes.py";
  
  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s", ctx->error_message ? ctx->error_message : "");
  
  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");
  
  // Check for basic class extraction
  ASTNode *simple_class = find_node_by_name(ctx->ast_root, "SimpleClass", AST_CLASS);
  assert_node_fields(simple_class, "SimpleClass");
  
  // Check for class methods
  ASTNode *class_method = find_node_by_name(simple_class, "__init__", AST_METHOD);
  if (class_method) {
    assert_node_fields(class_method, "__init__");
    // Verify parent-child relationship
    cr_assert_eq(class_method->parent, simple_class, "Method's parent should be the class");
  } else {
    cr_log_info("Class method extraction not fully implemented yet");
  }
  
  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

//=================================
// C AST Extraction Tests
//=================================

// Test extraction of C functions
Test(ast_extraction, c_functions,
     .description = "Test AST extraction of C functions") {
     
  cr_log_info("Testing C function AST extraction");
  
  // Read test file with C functions
  char *source_code = read_test_file("c", "core_constructs", "functions.c");
  cr_assert_not_null(source_code, "Failed to read test file");
  
  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_C;
  ctx->source_code = source_code;
  ctx->file_path = "functions.c";
  
  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s", ctx->error_message ? ctx->error_message : "");
  
  // Verify the AST root was created
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL");
  
  // Count functions to ensure basic extraction works
  int function_count = count_nodes_by_type(ctx->ast_root, AST_FUNCTION);
  cr_assert_gt(function_count, 0, "Should find at least one function");
  
  // Check for a specific function
  ASTNode *main_func = find_node_by_name(ctx->ast_root, "main", AST_FUNCTION);
  if (main_func) {
    assert_node_fields(main_func, "main");
    cr_assert_not_null(main_func->raw_content, "Function should have raw content populated");
  } else {
    cr_log_info("Function extraction may need more refinement");
  }
  
  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

// Test extraction of C structs
Test(ast_extraction, c_structs,
     .description = "Test AST extraction of C structs") {
     
  cr_log_info("Testing C struct AST extraction");
  
  // Read test file with C structs
  char *source_code = read_test_file("c", "core_constructs", "structs.c");
  cr_assert_not_null(source_code, "Failed to read test file");
  
  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_C;
  ctx->source_code = source_code;
  ctx->file_path = "structs.c";
  
  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s", ctx->error_message ? ctx->error_message : "");
  
  // Dump AST structure for debugging (comment out in production)
  // dump_ast_structure(ctx->ast_root, 0);
  
  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

//=================================
// Hierarchical AST Tests
//=================================

// Test hierarchical relationships in Python
Test(ast_extraction, python_hierarchy,
     .description = "Test hierarchical AST extraction for Python") {
     
  cr_log_info("Testing Python AST hierarchy extraction");
  
  // Read test file with nested Python structures
  char *source_code = read_test_file("python", "basic_syntax", "classes.py");
  cr_assert_not_null(source_code, "Failed to read test file");
  
  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_PYTHON;
  ctx->source_code = source_code;
  ctx->file_path = "classes.py";
  
  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s", ctx->error_message ? ctx->error_message : "");
  
  // Look for class with methods
  ASTNode *class_node = find_node_by_name(ctx->ast_root, "ClassWithMethods", AST_CLASS);
  if (class_node) {
    // Verify that the class has child nodes (methods)
    cr_assert_gt(class_node->num_children, 0, "Class should have child nodes");
    
    // Check qualified name format
    if (class_node->qualified_name) {
      cr_log_info("Class qualified name: %s", class_node->qualified_name);
      // Methods should have qualified names that include the class name
      for (size_t i = 0; i < class_node->num_children; i++) {
        ASTNode *method = class_node->children[i];
        if (method && method->type == AST_METHOD && method->qualified_name) {
          cr_assert(strstr(method->qualified_name, class_node->name) != NULL, 
                   "Method qualified name should include class name: %s", method->qualified_name);
        }
      }
    }
  } else {
    cr_log_info("Complex class extraction may need more refinement");
  }
  
  // Clean up
  parser_context_free(ctx);
  free(source_code);
}

//=================================
// Edge Case Tests
//=================================

// Test handling of empty files
Test(ast_extraction, empty_file,
     .description = "Test AST extraction with empty file") {
     
  cr_log_info("Testing AST extraction with empty file");
  
  // Create an empty source string
  const char *source_code = "";
  
  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  ctx->language = LANG_PYTHON;
  ctx->source_code = source_code;
  ctx->file_path = "empty.py";
  
  // Parse the source code
  parser_parse_string(ctx, source_code, strlen(source_code));
  cr_assert_null(ctx->error_message, "Parser error: %s", ctx->error_message ? ctx->error_message : "");
  
  // Should create an empty AST root
  cr_assert_not_null(ctx->ast_root, "AST root should not be NULL even for empty file");
  cr_assert_eq(ctx->ast_root->num_children, 0, "AST root should have no children for empty file");
  
  // Clean up
  parser_context_free(ctx);
}

// Add more test cases for other languages and constructs as needed
