/**
 * @file tree_sitter_integration.c
 * @brief Implementation of the Tree-sitter integration for ScopeMux
 *
 * This file implements the integration with Tree-sitter, handling the
 * initialization of language-specific parsers and the conversion of raw
 * Tree-sitter trees into ScopeMux's AST or CST representations.
 */

#include "../../include/scopemux/tree_sitter_integration.h"
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/query_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for Tree-sitter language functions from vendor library
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
// JavaScript and TypeScript support removed temporarily
// extern const TSLanguage *tree_sitter_javascript(void);
// extern const TSLanguage *tree_sitter_typescript(void);

/**
 * @brief Initializes or retrieves a Tree-sitter parser for the given language.
 */
bool ts_init_parser(ParserContext *ctx, LanguageType language) {
  if (!ctx) {
    return false;
  }

  // If a parser already exists, no need to re-initialize.
  // The language check should happen in the calling function.
  if (ctx->ts_parser) {
    return true;
  }

  ctx->ts_parser = ts_parser_new();
  if (!ctx->ts_parser) {
    parser_set_error(ctx, -1, "Failed to create Tree-sitter parser");
    return false;
  }

  const TSLanguage *ts_language = NULL;
  switch (language) {
  case LANG_C:
    ts_language = tree_sitter_c();
    break;
  case LANG_CPP:
    ts_language = tree_sitter_cpp();
    break;
  case LANG_PYTHON:
    ts_language = tree_sitter_python();
    break;
  case LANG_JAVASCRIPT:
  case LANG_TYPESCRIPT:
    // JavaScript and TypeScript support removed temporarily
    parser_set_error(ctx, -1, "JavaScript and TypeScript support temporarily disabled");
    return false;
    // ts_language = tree_sitter_javascript();
    // break;
  // case LANG_TYPESCRIPT:
  // For TypeScript, we often use the typescript-tsx grammar
  // ts_language = tree_sitter_typescript();
  // break;
  default:
    parser_set_error(ctx, -1, "Unsupported language for Tree-sitter parser");
    return false;
  }

  if (!ts_parser_set_language(ctx->ts_parser, ts_language)) {
    parser_set_error(ctx, -1, "Failed to set language on Tree-sitter parser");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  return true;
}

// Helper to copy the text of a TSNode into a new string.
static char *ts_node_to_string(TSNode node, const char *source_code) {
  if (ts_node_is_null(node) || !source_code) {
    return NULL;
  }
  uint32_t start = ts_node_start_byte(node);
  uint32_t end = ts_node_end_byte(node);
  size_t length = end - start;
  char *str = (char *)malloc(length + 1);
  if (!str) {
    return NULL;
  }
  strncpy(str, source_code + start, length);
  str[length] = '\0';
  return str;
}

// Helper function to generate a qualified name for an AST node
static char *generate_qualified_name(const char *name, ASTNode *parent) {
  if (!name)
    return NULL;

  if (!parent || parent->type == NODE_UNKNOWN || !parent->qualified_name) {
    return strdup(name);
  }

  size_t len = strlen(parent->qualified_name) + strlen(name) + 2; // +2 for '::' and null terminator
  char *qualified = malloc(len);
  if (qualified) {
    snprintf(qualified, len, "%s::%s", parent->qualified_name, name);
  }
  return qualified;
}

// Helper function to extract the raw content for a node
static char *extract_raw_content(TSNode node, const char *source_code) {
  if (!source_code || ts_node_is_null(node))
    return NULL;

  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  uint32_t length = end_byte - start_byte;

  char *content = malloc(length + 1);
  if (content) {
    memcpy(content, source_code + start_byte, length);
    content[length] = '\0';
  }
  return content;
}

// Helper function to extract AST nodes from a specific query type
static void process_query(const char *query_type, TSNode root_node, ParserContext *ctx,
                          ASTNode *ast_root, ASTNode **node_map) {
  // Get the query for the specified type
  const TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, query_type);
  if (!query) {
    // Skip if query doesn't exist for this language
    return;
  }

  // Execute the query
  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, root_node);

  // Iterate over matches
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    char *node_name = NULL;
    TSNode target_node = {0};
    TSNode body_node __attribute__((unused)) = {0};
    TSNode params_node = {0};
    char *docstring = NULL;
    uint32_t node_type = NODE_UNKNOWN;
    ASTNode *parent_node = ast_root; // Default parent is root

    // Determine node type based on query
    if (strcmp(query_type, "functions") == 0) {
      node_type = NODE_FUNCTION;
    } else if (strcmp(query_type, "classes") == 0) {
      node_type = NODE_CLASS;
    } else if (strcmp(query_type, "methods") == 0) {
      node_type = NODE_METHOD;
    } else if (strcmp(query_type, "variables") == 0) {
      node_type = NODE_UNKNOWN; // No specific variable type in ASTNodeType
    } else if (strcmp(query_type, "imports") == 0) {
      node_type = NODE_MODULE;
    } else if (strcmp(query_type, "control_flow") == 0) {
      node_type = NODE_UNKNOWN; // No specific control flow type in ASTNodeType
    }

    // Process captures to populate node information
    for (uint32_t i = 0; i < match.capture_count; ++i) {
      TSNode captured_node = match.captures[i].node;
      uint32_t capture_index = match.captures[i].index;
      const char *capture_name = ts_query_capture_name_for_id(query, capture_index, NULL);

      if (strstr(capture_name, "function") || strstr(capture_name, "class") ||
          strstr(capture_name, "method") || strstr(capture_name, "variable") ||
          strstr(capture_name, "import") || strstr(capture_name, "if_statement") ||
          strstr(capture_name, "for_loop") || strstr(capture_name, "while_loop") ||
          strstr(capture_name, "try_statement")) {
        target_node = captured_node;
      } else if (strcmp(capture_name, "name") == 0) {
        node_name = ts_node_to_string(captured_node, ctx->source_code);
      } else if (strcmp(capture_name, "body") == 0) {
        body_node = captured_node;
      } else if (strcmp(capture_name, "params") == 0 || strcmp(capture_name, "parameters") == 0) {
        params_node = captured_node;
      } else if (strcmp(capture_name, "docstring") == 0) {
        docstring = ts_node_to_string(captured_node, ctx->source_code);
      } else if (strcmp(capture_name, "class_name") == 0 && strcmp(query_type, "methods") == 0) {
        // Use node_map to find the parent class for methods
        char *class_name = ts_node_to_string(captured_node, ctx->source_code);
        if (class_name && node_map[NODE_CLASS]) {
          parent_node = node_map[NODE_CLASS];
        }
        free(class_name);
      }
    }

    if (node_name && !ts_node_is_null(target_node)) {
      ASTNode *ast_node = ast_node_new(node_type, node_name);
      if (ast_node && !parser_add_ast_node(ctx, ast_node)) {
        // Failed to add to context, must free manually to avoid leak
        ast_node_free(ast_node);
        ast_node = NULL;
      }
      if (ast_node) {
        // Set source range
        ast_node->range.start.line = ts_node_start_point(target_node).row;
        ast_node->range.end.line = ts_node_end_point(target_node).row;
        ast_node->range.start.column = ts_node_start_point(target_node).column;
        ast_node->range.end.column = ts_node_end_point(target_node).column;

        // Populate signature if params are available
        if (!ts_node_is_null(params_node)) {
          ast_node->signature = ts_node_to_string(params_node, ctx->source_code);
        }

        // Populate docstring if available
        if (docstring) {
          ast_node->docstring = docstring;
          // Don't free docstring as it's now owned by the node
        }

        // Extract raw content
        ast_node->raw_content = extract_raw_content(target_node, ctx->source_code);

        // Generate qualified name based on parent
        ast_node->qualified_name = generate_qualified_name(node_name, parent_node);

        // Store parent reference
        ast_node->parent = parent_node;

        // Add to parent (either root or another node)
        ast_node_add_child(parent_node, ast_node);

        // Store in node_map for potential parent-child relationships
        if (node_type < 256) { // Ensure we're within array bounds
          node_map[node_type] = ast_node;
        }
      }
      free(node_name); // Free the copied name
    }
  }

  ts_query_cursor_delete(cursor);
}

ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx) {
    parser_set_error(ctx, -1, "Invalid arguments to ts_tree_to_ast");
    return NULL;
  }

  // Create root AST node
  ASTNode *ast_root = ast_node_new(NODE_MODULE, "Program");
  if (!ast_root) {
    parser_set_error(ctx, -1, "Failed to allocate AST root node");
    return NULL;
  }

  // IMPORTANT: Always register the root node with the parser context first
  // This ensures it will be properly freed during parser_clear
  if (!parser_add_ast_node(ctx, ast_root)) {
    parser_set_error(ctx, -1, "Failed to register AST root node with parser context");
    ast_node_free(ast_root); // Free directly since it's not tracked by context yet
    return NULL;
  }
  
  // Set qualified name for root to be the file path or module name
  if (ctx->filename) {
    ast_root->qualified_name = strdup(ctx->filename);
  }

  // Node map to help build hierarchical relationships
  ASTNode *node_map[256] = {NULL}; // Assuming AST_* constants are < 256

  // Process different query types in order of hierarchy
  const char *query_types[] = {"classes", "structs",      "unions",     "enums",   "typedefs",
                               "methods", "functions",    "variables",  "imports", "includes",
                               "macros",  "control_flow", "docstrings", NULL};

  size_t initial_child_count = ast_root->num_children;

  // Process each query type
  for (int i = 0; query_types[i]; i++) {
    process_query(query_types[i], root_node, ctx, ast_root, node_map);
  }

  // If no semantic nodes were found, treat as special edge case - empty files/invalid syntax
  if (ast_root->num_children == initial_child_count) {
    // Set an error message but don't consider this a failure
    // We still return a valid AST root node, just with no children
    parser_set_error(ctx, -1, "No AST nodes generated (empty or invalid input)");
    
    // The root node is already registered with the context and will be freed
    // when parser_clear or parser_free is called
    return ast_root;
  }

  // Post-processing step to enrich nodes with additional info or establish more complex
  // relationships (placeholder for future enhancement)

  return ast_root;
}

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Concrete Syntax Tree.
 */
// Forward declaration for the recursive helper
static CSTNode *create_cst_from_ts_node(TSNode ts_node, const char *source_code);

CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx || !ctx->source_code) {
    parser_set_error(ctx, -1, "Invalid context for CST generation.");
    return NULL;
  }
  return create_cst_from_ts_node(root_node, ctx->source_code);
}

// Recursive helper to build the CST
static CSTNode *create_cst_from_ts_node(TSNode ts_node, const char *source_code) {
  if (ts_node_is_null(ts_node)) {
    return NULL;
  }

  // 1. Create a new CSTNode
  const char *type = ts_node_type(ts_node);
  char *content = ts_node_to_string(ts_node, source_code);
  CSTNode *cst_node = cst_node_new(type, content);
  if (!cst_node) {
    if (content)
      free(content);
    return NULL;
  }

  // 2. Set the source range
  cst_node->range.start.line = ts_node_start_point(ts_node).row;
  cst_node->range.end.line = ts_node_end_point(ts_node).row;
  cst_node->range.start.column = ts_node_start_point(ts_node).column;
  cst_node->range.end.column = ts_node_end_point(ts_node).column;

  // 3. Recursively process all children
  uint32_t child_count = ts_node_child_count(ts_node);
  for (uint32_t i = 0; i < child_count; ++i) {
    TSNode ts_child = ts_node_child(ts_node, i);
    CSTNode *cst_child = create_cst_from_ts_node(ts_child, source_code);
    if (cst_child) {
      cst_node_add_child(cst_node, cst_child);
    }
  }

  return cst_node;
}
