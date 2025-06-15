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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for Tree-sitter language functions from vendor library
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

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
    ts_language = tree_sitter_javascript();
    break;
  case LANG_TYPESCRIPT:
    // For TypeScript, we often use the typescript-tsx grammar
    ts_language = tree_sitter_typescript();
    break;
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

ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx || !ctx->q_manager) {
    parser_set_error(ctx, -1, "Invalid context for AST generation.");
    return NULL;
  }

  // 1. Get the query for functions for the current language
  const TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, "functions");
  if (!query) {
    parser_set_error(ctx, -1, "Could not load 'functions' query for the specified language.");
    return NULL; // Error is already logged by query manager
  }

  // 2. Execute the query
  TSQueryCursor *cursor = ts_query_cursor_new();
  ts_query_cursor_exec(cursor, query, root_node);

  // 3. Create a root node for the AST
  ASTNode *ast_root = ast_node_new(AST_ROOT, "ROOT");
  if (!ast_root) {
    parser_set_error(ctx, -1, "Failed to create AST root node.");
    ts_query_cursor_delete(cursor);
    return NULL;
  }

  // 4. Iterate over matches and create AST nodes
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    char *function_name = NULL;
    TSNode function_node = {0};

    for (uint32_t i = 0; i < match.capture_count; ++i) {
      TSNode captured_node = match.captures[i].node;
      uint32_t capture_index = match.captures[i].index;
      const char *capture_name = ts_query_capture_name_for_id(query, capture_index, NULL);

      if (strcmp(capture_name, "function") == 0) {
        function_node = captured_node;
      } else if (strcmp(capture_name, "name") == 0) {
        function_name = ts_node_to_string(captured_node, ctx->source_code);
      }
    }

    if (function_name && !ts_node_is_null(function_node)) {
      ASTNode *func_node = ast_node_new(AST_FUNCTION, function_name);
      if (func_node) {
        // Set source range
        func_node->range.start_line = ts_node_start_point(function_node).row;
        func_node->range.end_line = ts_node_end_point(function_node).row;
        func_node->range.start_column = ts_node_start_point(function_node).column;
        func_node->range.end_column = ts_node_end_point(function_node).column;

        // Add to the root
        ast_node_add_child(ast_root, func_node);
      }
      free(function_name); // Free the copied name
    }
  }

  // 5. Clean up and return
  ts_query_cursor_delete(cursor);

  if (ast_root->num_children == 0) {
    // If no functions were found, it's not an error, but the root is empty.
    // We can return the empty root or NULL depending on desired behavior.
    // For now, returning the root.
  }

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
  cst_node->range.start_line = ts_node_start_point(ts_node).row;
  cst_node->range.end_line = ts_node_end_point(ts_node).row;
  cst_node->range.start_column = ts_node_start_point(ts_node).column;
  cst_node->range.end_column = ts_node_end_point(ts_node).column;

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
