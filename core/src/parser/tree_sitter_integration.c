/**
 * @file tree_sitter_integration.c
 * @brief Implementation of the Tree-sitter integration for ScopeMux
 *
 * This file implements the integration with Tree-sitter, handling the
 * initialization of language-specific parsers, and AST traversal.
 */

#include "../../include/scopemux/tree_sitter_integration.h"
#include "../../include/scopemux/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include Tree-sitter API directly to ensure it's found
#include "../../include/tree_sitter/api.h"

// Forward declarations for Tree-sitter language functions
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);

/* Initialize a Tree-sitter parser */
TreeSitterParser *ts_parser_init(LanguageType language) {
  // Allocate memory for the parser struct
  TreeSitterParser *parser = (TreeSitterParser *)malloc(sizeof(TreeSitterParser));
  if (!parser) {
    return NULL;
  }

  // Initialize the parser fields
  parser->ts_parser = ts_parser_new();
  if (!parser->ts_parser) {
    free(parser);
    return NULL;
  }

  // Set the language based on the requested language
  parser->language = language;
  parser->last_error = NULL;

  // Set up the Tree-sitter language
  bool success = false;

  switch (language) {
  case LANG_C:
    if (tree_sitter_c) {
      success = ts_parser_set_language(parser->ts_parser, tree_sitter_c());
    }
    break;

  case LANG_CPP:
    if (tree_sitter_cpp) {
      success = ts_parser_set_language(parser->ts_parser, tree_sitter_cpp());
    }
    break;

  case LANG_PYTHON:
    if (tree_sitter_python) {
      success = ts_parser_set_language(parser->ts_parser, tree_sitter_python());
    }
    break;

  default:
    // Language not supported
    break;
  }

  if (!success) {
    parser->last_error = strdup("Failed to set language for parser");
    ts_parser_delete(parser->ts_parser);
    free(parser);
    return NULL;
  }

  return parser;
}

/* Free a Tree-sitter parser */
void ts_parser_free(TreeSitterParser *parser) {
  if (!parser) {
    return;
  }

  // Free the Tree-sitter parser
  if (parser->ts_parser) {
    ts_parser_delete(parser->ts_parser);
    parser->ts_parser = NULL;
  }

  // Free the last error message if any
  if (parser->last_error) {
    free(parser->last_error);
    parser->last_error = NULL;
  }

  // Free the parser struct itself
  free(parser);
}

/* Parse a string with Tree-sitter */
void *scopemux_ts_parser_parse_string(TreeSitterParser *parser, const char *source_code,
                                      size_t source_code_length) {
  if (!parser || !parser->ts_parser || !source_code) {
    return NULL;
  }

  // Clear any previous error
  if (parser->last_error) {
    free(parser->last_error);
    parser->last_error = NULL;
  }

  // Parse the source code
  TSTree *tree = ts_parser_parse_string(parser->ts_parser,
                                        NULL, // No previous tree for incremental parsing
                                        source_code, source_code_length);

  if (!tree) {
    parser->last_error = strdup("Failed to parse source code");
    return NULL;
  }

  return (void *)tree;
}

/* Tree-sitter syntax tree cleanup */
void ts_tree_free(void *tree) {
  // Properly free the Tree-sitter syntax tree
  if (tree) {
    ts_tree_delete((TSTree *)tree);
  }
}

/* Error handling */
const char *ts_parser_get_last_error(const TreeSitterParser *parser) {
  if (!parser) {
    return "Invalid parser";
  }
  return parser->last_error ? parser->last_error : "No error";
}

/* Convert Tree-sitter syntax tree to ScopeMux IR */
bool ts_tree_to_ir(TreeSitterParser *parser, void *tree, ParserContext *parser_ctx) {
  if (!parser || !tree || !parser_ctx) {
    return false;
  }

  // Get the root node of the tree
  TSTree *ts_tree = (TSTree *)tree;
  TSNode root_node = ts_tree_root_node(ts_tree);

  // Skip conversion if the root node is not valid
  if (ts_node_is_null(root_node)) {
    if (parser->last_error)
      free(parser->last_error);
    parser->last_error = strdup("Invalid syntax tree");
    return false;
  }

  // Create the root IR node if it doesn't exist yet
  if (!parser_ctx->root_node) {
    SourceRange file_range = {{0, 0, 0}, {0, 0, 0}};
    parser_ctx->root_node =
        ir_node_create(NODE_MODULE, parser_ctx->filename ? parser_ctx->filename : "<unknown>",
                       parser_ctx->filename ? parser_ctx->filename : "<unknown>", file_range);
    if (!parser_ctx->root_node) {
      if (parser->last_error)
        free(parser->last_error);
      parser->last_error = strdup("Failed to create root IR node");
      return false;
    }
  }

  // Process the tree by extracting various elements
  size_t comments_count = ts_extract_comments(parser, tree, parser_ctx);
  size_t functions_count = ts_extract_functions(parser, tree, parser_ctx);
  size_t classes_count = ts_extract_classes(parser, tree, parser_ctx);

  // Return success if we extracted at least something
  return (comments_count > 0 || functions_count > 0 || classes_count > 0);
}

/* Extract comments and docstrings */
size_t ts_extract_comments(TreeSitterParser *parser, void *tree, ParserContext *parser_ctx) {
  if (!parser || !tree || !parser_ctx || !parser_ctx->root_node) {
    return 0;
  }

  TSTree *ts_tree = (TSTree *)tree;
  TSNode root_node = ts_tree_root_node(ts_tree);

  // Ensure root node exists and we have source code to work with
  if (ts_node_symbol(root_node) == 0 || !parser_ctx->source_code) {
    return 0;
  }

  size_t comments_found = 0;
  size_t cursor_index = 0;

  // Create a tree cursor for traversing the tree
  TSTreeCursor cursor = ts_tree_cursor_new(root_node);

  // First visit is the root node
  do {
    TSNode node = ts_tree_cursor_current_node(&cursor);

    // Check if this node is a comment
    if (ts_is_comment(parser, &node)) {
      // Get the text range of the comment
      TSPoint start_point = ts_node_start_point(node);
      TSPoint end_point = ts_node_end_point(node);
      uint32_t start_byte = ts_node_start_byte(node);
      uint32_t end_byte = ts_node_end_byte(node);

      // Create source range
      SourceRange range = {{start_point.row, start_point.column, start_byte},
                           {end_point.row, end_point.column, end_byte}};

      // Extract the comment text
      char *comment_text =
          ts_get_node_text(&node, parser_ctx->source_code, parser_ctx->source_code_length);
      if (comment_text) {
        // Create an IR node for the comment
        char name[32];
        snprintf(name, sizeof(name), "comment_%zu", comments_found);

        char qualified_name[256];
        snprintf(qualified_name, sizeof(qualified_name), "%s.%s",
                 parser_ctx->filename ? parser_ctx->filename : "<unknown>", name);

        IRNode *comment_node = ir_node_create(NODE_COMMENT, name, qualified_name, range);
        if (comment_node) {
          // Set the raw content to the comment text
          comment_node->raw_content = comment_text;

          // Add the comment node as a child of the root node
          if (ir_node_add_child(parser_ctx->root_node, comment_node)) {
            comments_found++;
          } else {
            // Failed to add as child, free memory
            free(comment_text);
            ir_node_free(comment_node);
          }
        } else {
          // Failed to create comment node, free memory
          free(comment_text);
        }
      }
    }

    // Move to the next node in the tree
    if (ts_tree_cursor_goto_first_child(&cursor)) {
      // Moved down to first child
      cursor_index++;
    } else {
      // Try to move to next sibling
      while (!ts_tree_cursor_goto_next_sibling(&cursor) && cursor_index > 0) {
        ts_tree_cursor_goto_parent(&cursor);
        cursor_index--;
      }
    }
  } while (cursor_index > 0);

  // Clean up the cursor
  ts_tree_cursor_delete(&cursor);

  return comments_found;
}

/* Extract functions and methods */
size_t ts_extract_functions(TreeSitterParser *parser, void *tree, ParserContext *parser_ctx) {
  if (!parser || !tree || !parser_ctx || !parser_ctx->root_node) {
    return 0;
  }

  TSTree *ts_tree = (TSTree *)tree;
  TSNode root_node = ts_tree_root_node(ts_tree);

  // Ensure root node exists and we have source code to work with
  if (ts_node_is_null(root_node) || !parser_ctx->source_code) {
    return 0;
  }

  size_t functions_found = 0;
  size_t cursor_index = 0;

  // Create a tree cursor for traversing the tree
  TSTreeCursor cursor = ts_tree_cursor_new(root_node);

  // First visit is the root node
  do {
    TSNode node = ts_tree_cursor_current_node(&cursor);

    // Check if this node is a function or method
    if (ts_is_function(parser, &node)) {
      // Get the text range of the function
      TSPoint start_point = ts_node_start_point(node);
      TSPoint end_point = ts_node_end_point(node);
      uint32_t start_byte = ts_node_start_byte(node);
      uint32_t end_byte = ts_node_end_byte(node);

      // Create source range
      SourceRange range = {{start_point.row, start_point.column, start_byte},
                           {end_point.row, end_point.column, end_byte}};

      // Extract the function's signature
      char *signature = ts_extract_function_signature(parser, &node, parser_ctx->source_code,
                                                      parser_ctx->source_code_length);

      // Get function name
      const char *func_type = ts_node_type(node);
      char *name = NULL;

      // Find the identifier node for the function name
      TSNode identifier = ts_node_child_by_field_name(node, "name", 4);
      if (!ts_node_is_null(identifier)) {
        name =
            ts_get_node_text(&identifier, parser_ctx->source_code, parser_ctx->source_code_length);
      }

      // If no name was found, use a generic name
      if (!name) {
        name = strdup("unnamed_function");
      }

      // Create qualified name
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s",
               parser_ctx->filename ? parser_ctx->filename : "<unknown>", name);

      // Create an IR node for the function
      IRNode *function_node = ir_node_create(NODE_FUNCTION, name, qualified_name, range);
      if (function_node) {
        // Set the signature
        if (signature) {
          function_node->signature = signature;
        }

        // Extract function body text
        TSNode body = ts_node_child_by_field_name(node, "body", 4);
        if (!ts_node_is_null(body)) {
          function_node->raw_content =
              ts_get_node_text(&body, parser_ctx->source_code, parser_ctx->source_code_length);
        }

        // Look for docstring (comment before function)
        // This would be specific to the language's conventions

        // Add the function node as a child of the root node
        if (ir_node_add_child(parser_ctx->root_node, function_node)) {
          functions_found++;

          // Add to flat list of nodes for quick lookup
          if (parser_ctx->num_nodes < parser_ctx->nodes_capacity ||
              ir_context_resize_nodes(parser_ctx, parser_ctx->num_nodes + 1)) {
            parser_ctx->all_nodes[parser_ctx->num_nodes++] = function_node;
          }
        } else {
          // Failed to add as child, free memory
          if (function_node->signature)
            free(function_node->signature);
          if (function_node->raw_content)
            free(function_node->raw_content);
          ir_node_free(function_node);
        }
      } else {
        // Failed to create function node, free memory
        if (signature)
          free(signature);
        free(name);
      }
    }

    // Move to the next node in the tree
    if (ts_tree_cursor_goto_first_child(&cursor)) {
      // Moved down to first child
      cursor_index++;
    } else {
      // Try to move to next sibling
      while (!ts_tree_cursor_goto_next_sibling(&cursor) && cursor_index > 0) {
        ts_tree_cursor_goto_parent(&cursor);
        cursor_index--;
      }
    }
  } while (cursor_index > 0);

  // Clean up the cursor
  ts_tree_cursor_delete(&cursor);

  return functions_found;
}

/* Extract classes and type definitions */
size_t ts_extract_classes(TreeSitterParser *parser, void *tree, ParserContext *parser_ctx) {
  if (!parser || !tree || !parser_ctx || !parser_ctx->root_node) {
    return 0;
  }

  TSTree *ts_tree = (TSTree *)tree;
  TSNode root_node = ts_tree_root_node(ts_tree);

  // Ensure root node exists and we have source code to work with
  if (ts_node_is_null(root_node) || !parser_ctx->source_code) {
    return 0;
  }

  size_t classes_found = 0;
  size_t cursor_index = 0;

  // Create a tree cursor for traversing the tree
  TSTreeCursor cursor = ts_tree_cursor_new(root_node);

  // First visit is the root node
  do {
    TSNode node = ts_tree_cursor_current_node(&cursor);

    // Check if this node is a class or type definition
    if (ts_is_class(parser, &node)) {
      // Get the text range of the class
      TSPoint start_point = ts_node_start_point(node);
      TSPoint end_point = ts_node_end_point(node);
      uint32_t start_byte = ts_node_start_byte(node);
      uint32_t end_byte = ts_node_end_byte(node);

      // Create source range
      SourceRange range = {{start_point.row, start_point.column, start_byte},
                           {end_point.row, end_point.column, end_byte}};

      // Extract the class name
      char *name = ts_extract_class_name(parser, &node, parser_ctx->source_code,
                                         parser_ctx->source_code_length);

      // If no name was found, use a generic name
      if (!name) {
        name = strdup("unnamed_class");
      }

      // Create qualified name
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s",
               parser_ctx->filename ? parser_ctx->filename : "<unknown>", name);

      // Create an IR node for the class
      IRNode *class_node = ir_node_create(NODE_CLASS, name, qualified_name, range);
      if (class_node) {
        // Extract class body text
        TSNode body = ts_node_child_by_field_name(node, "body", 4);
        if (ts_node_is_null(body)) {
          // Try a second common field name
          body = ts_node_child_by_field_name(node, "declarator", 10);
        }

        if (!ts_node_is_null(body)) {
          class_node->raw_content =
              ts_get_node_text(&body, parser_ctx->source_code, parser_ctx->source_code_length);
        }

        // Add the class node as a child of the root node
        if (ir_node_add_child(parser_ctx->root_node, class_node)) {
          classes_found++;

          // Add to flat list of nodes for quick lookup
          if (parser_ctx->num_nodes < parser_ctx->nodes_capacity ||
              ir_context_resize_nodes(parser_ctx, parser_ctx->num_nodes + 1)) {
            parser_ctx->all_nodes[parser_ctx->num_nodes++] = class_node;
          }
        } else {
          // Failed to add as child, free memory
          if (class_node->raw_content)
            free(class_node->raw_content);
          ir_node_free(class_node);
        }
      } else {
        // Failed to create class node, free memory
        free(name);
      }
    }

    // Move to the next node in the tree
    if (ts_tree_cursor_goto_first_child(&cursor)) {
      // Moved down to first child
      cursor_index++;
    } else {
      // Try to move to next sibling
      while (!ts_tree_cursor_goto_next_sibling(&cursor) && cursor_index > 0) {
        ts_tree_cursor_goto_parent(&cursor);
        cursor_index--;
      }
    }
  } while (cursor_index > 0);

  // Clean up the cursor
  ts_tree_cursor_delete(&cursor);

  return classes_found;
}

/* Get source range for a Tree-sitter node */
SourceRange ts_get_node_range(void *tree_node) {
  if (!tree_node) {
    return (SourceRange){{0, 0, 0}, {0, 0, 0}};
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return (SourceRange){{0, 0, 0}, {0, 0, 0}};
  }

  // Get the start and end points of the node
  TSPoint start_point = ts_node_start_point(*node);
  TSPoint end_point = ts_node_end_point(*node);

  // Get the start and end bytes of the node
  uint32_t start_byte = ts_node_start_byte(*node);
  uint32_t end_byte = ts_node_end_byte(*node);

  // Create the source range
  SourceRange range = {{start_point.row, start_point.column, start_byte},
                       {end_point.row, end_point.column, end_byte}};

  return range;
}

/* Get the text content of a node */
char *ts_get_node_text(void *tree_node, const char *source, size_t source_len) {
  if (!tree_node || !source || source_len == 0) {
    return NULL;
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return NULL;
  }

  // Get the range of bytes for the node
  uint32_t start_byte = ts_node_start_byte(*node);
  uint32_t end_byte = ts_node_end_byte(*node);

  // Validate the range
  if (start_byte >= source_len || end_byte > source_len || end_byte <= start_byte) {
    return NULL;
  }

  // Calculate the length of the text
  uint32_t length = end_byte - start_byte;

  // Allocate memory for the text
  char *text = (char *)malloc(length + 1);
  if (!text) {
    return NULL;
  }

  // Copy the text from the source
  memcpy(text, source + start_byte, length);
  text[length] = '\0'; // Null-terminate

  return text;
}

/* Check if a node is a function or method */
bool ts_is_function(TreeSitterParser *parser, void *tree_node) {
  if (!parser || !tree_node) {
    return false;
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return false;
  }

  // Get the type of the node
  const char *node_type = ts_node_type(*node);
  if (!node_type) {
    return false;
  }

  // Different languages have different node types for functions
  switch (parser->language) {
  case LANG_C:
    // C function declarations
    return (strcmp(node_type, "function_definition") == 0 || strcmp(node_type, "declaration") == 0);

  case LANG_CPP:
    // C++ function declarations (includes C plus more)
    return (strcmp(node_type, "function_definition") == 0 ||
            strcmp(node_type, "declaration") == 0 || strcmp(node_type, "method_definition") == 0 ||
            strcmp(node_type, "lambda_expression") == 0);

  case LANG_PYTHON:
    // Python function declarations
    return (strcmp(node_type, "function_definition") == 0 ||
            strcmp(node_type, "decorated_definition") == 0);

  default:
    return false;
  }
}

/* Check if a node is a class or type definition */
bool ts_is_class(TreeSitterParser *parser, void *tree_node) {
  if (!parser || !tree_node) {
    return false;
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return false;
  }

  // Get the type of the node
  const char *node_type = ts_node_type(*node);
  if (!node_type) {
    return false;
  }

  // Different languages have different node types for classes and type definitions
  switch (parser->language) {
  case LANG_C:
    // C struct, enum, union, and typedef declarations
    return (strcmp(node_type, "struct_specifier") == 0 ||
            strcmp(node_type, "enum_specifier") == 0 || strcmp(node_type, "union_specifier") == 0 ||
            strcmp(node_type, "typedef_declaration") == 0);

  case LANG_CPP:
    // C++ class, struct, enum, union, and typedef declarations
    return (strcmp(node_type, "class_specifier") == 0 ||
            strcmp(node_type, "struct_specifier") == 0 ||
            strcmp(node_type, "enum_specifier") == 0 || strcmp(node_type, "union_specifier") == 0 ||
            strcmp(node_type, "typedef_declaration") == 0 ||
            strcmp(node_type, "namespace_definition") == 0);

  case LANG_PYTHON:
    // Python class declarations
    return (strcmp(node_type, "class_definition") == 0 ||
            strcmp(node_type, "decorated_definition") == 0);

  default:
    return false;
  }
}

/* Check if a node is a comment or docstring */
bool ts_is_comment(TreeSitterParser *parser, void *tree_node) {
  if (!parser || !tree_node) {
    return false;
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return false;
  }

  // Get the type of the node
  const char *node_type = ts_node_type(*node);
  if (!node_type) {
    return false;
  }

  // Different languages have different node types for comments
  switch (parser->language) {
  case LANG_C:
  case LANG_CPP:
    // C/C++ comment types
    return (strcmp(node_type, "comment") == 0 || strcmp(node_type, "line_comment") == 0 ||
            strcmp(node_type, "block_comment") == 0);

  case LANG_PYTHON:
    // Python comment types
    return (strcmp(node_type, "comment") == 0 ||
            strcmp(node_type, "string") == 0); // Python docstrings are strings

  default:
    // Generic fallback
    return (strcmp(node_type, "comment") == 0);
  }
}

/* Extract function/method signature */
char *ts_extract_function_signature(TreeSitterParser *parser, void *tree_node,
                                    const char *source_code, size_t source_code_length) {
  if (!parser || !tree_node || !source_code || source_code_length == 0) {
    return NULL;
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return NULL;
  }

  // Function signatures are language-specific
  switch (parser->language) {
  case LANG_C:
  case LANG_CPP: {
    // For C/C++, we want the function name and parameters
    // First try to find the declarator node which contains the function name and parameters
    TSNode declarator;

    // Different node types have different structures
    const char *node_type = ts_node_type(*node);
    if (strcmp(node_type, "function_definition") == 0) {
      declarator = ts_node_child_by_field_name(*node, "declarator", 10);
    } else if (strcmp(node_type, "declaration") == 0) {
      declarator = ts_node_child(*node, 1); // Second child is usually the declarator
    } else if (strcmp(node_type, "method_definition") == 0) {
      declarator = ts_node_child_by_field_name(*node, "declarator", 10);
    } else {
      // Unknown node type
      return NULL;
    }

    if (ts_node_is_null(declarator)) {
      return NULL;
    }

    // Extract the text for the declarator
    char *signature_text = ts_get_node_text(&declarator, source_code, source_code_length);
    if (!signature_text) {
      return NULL;
    }

    return signature_text;
  }

  case LANG_PYTHON: {
    // For Python, we want the function name and parameters (def name(params))
    TSNode name_node = ts_node_child_by_field_name(*node, "name", 4);
    TSNode parameters = ts_node_child_by_field_name(*node, "parameters", 10);

    if (ts_node_is_null(name_node) || ts_node_is_null(parameters)) {
      return NULL;
    }

    // Extract the name and parameters texts
    char *name_text = ts_get_node_text(&name_node, source_code, source_code_length);
    char *parameters_text = ts_get_node_text(&parameters, source_code, source_code_length);

    if (!name_text || !parameters_text) {
      if (name_text)
        free(name_text);
      if (parameters_text)
        free(parameters_text);
      return NULL;
    }

    // Allocate memory for the signature (def name(params))
    size_t signature_len = strlen("def ") + strlen(name_text) + strlen(parameters_text) + 1;
    char *signature = (char *)malloc(signature_len);
    if (!signature) {
      free(name_text);
      free(parameters_text);
      return NULL;
    }

    // Construct the signature
    snprintf(signature, signature_len, "def %s%s", name_text, parameters_text);

    // Free temporary strings
    free(name_text);
    free(parameters_text);

    return signature;
  }

  default:
    return NULL;
  }
}

/* Extract class/type name */
char *ts_extract_class_name(TreeSitterParser *parser, void *tree_node, const char *source_code,
                            size_t source_code_length) {
  if (!parser || !tree_node || !source_code || source_code_length == 0) {
    return NULL;
  }

  TSNode *node = (TSNode *)tree_node;
  if (ts_node_is_null(*node)) {
    return NULL;
  }

  // Class name extraction is language-specific
  switch (parser->language) {
  case LANG_C:
  case LANG_CPP: {
    // For C/C++, we need to handle different node types differently
    const char *node_type = ts_node_type(*node);
    TSNode name_node;

    if (strcmp(node_type, "struct_specifier") == 0 || strcmp(node_type, "class_specifier") == 0 ||
        strcmp(node_type, "enum_specifier") == 0 || strcmp(node_type, "union_specifier") == 0) {
      // These have a "name" field
      name_node = ts_node_child_by_field_name(*node, "name", 4);
    } else if (strcmp(node_type, "typedef_declaration") == 0) {
      // Typedefs have a different structure
      // Typically the identifier is the last child
      int child_count = ts_node_child_count(*node);
      if (child_count > 0) {
        // Last child is usually the typedef name
        name_node = ts_node_child(*node, child_count - 1);
      } else {
        return NULL;
      }
    } else if (strcmp(node_type, "namespace_definition") == 0) {
      name_node = ts_node_child_by_field_name(*node, "name", 4);
    } else {
      // Unknown node type
      return NULL;
    }

    // Extract the name
    if (ts_node_is_null(name_node)) {
      // If no name was found, use a placeholder based on the type
      const char *placeholder = strstr(node_type, "_");
      if (placeholder) {
        // Extract the type part (e.g., "struct" from "struct_specifier")
        size_t len = placeholder - node_type;
        char *type_name = (char *)malloc(len + 11); // +11 for "unnamed_" and null terminator
        if (!type_name)
          return NULL;

        snprintf(type_name, len + 11, "unnamed_%.*s", (int)len, node_type);
        return type_name;
      } else {
        return strdup("unnamed_type");
      }
    }

    return ts_get_node_text(&name_node, source_code, source_code_length);
  }

  case LANG_PYTHON: {
    // For Python, the class name is in the name field
    TSNode name_node = ts_node_child_by_field_name(*node, "name", 4);
    if (!ts_node_is_null(name_node)) {
      return ts_get_node_text(&name_node, source_code, source_code_length);
    } else {
      return strdup("unnamed_class");
    }
  }

  default:
    return NULL;
  }
}
