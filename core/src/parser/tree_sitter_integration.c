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

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Abstract Syntax Tree.
 */
ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx) {
  // TODO: Implement AST generation using Tree-sitter queries.
  // This function will execute queries to find functions, classes, etc.,
  // and build the corresponding ASTNode tree.
  if (ts_node_is_null(root_node) || !ctx) {
    return NULL;
  }

  parser_set_error(ctx, -1, "AST generation is not yet implemented.");
  return NULL;
}

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Concrete Syntax Tree.
 */
CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx) {
  // TODO: Implement CST generation via recursive traversal.
  // This function will walk the Tree-sitter tree and create a parallel
  // CSTNode tree, preserving the full syntax.
  if (ts_node_is_null(root_node) || !ctx) {
    return NULL;
  }

  parser_set_error(ctx, -1, "CST generation is not yet implemented.");
  return NULL;
}
