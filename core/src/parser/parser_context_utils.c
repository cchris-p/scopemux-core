/**
 * @file parser_context_utils.c
 * @brief Utility functions for ParserContext
 *
 * This file provides additional utility functions for managing parser contexts,
 * including dependency tracking and AST management.
 */

#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include <stdlib.h>
#include <string.h>

/**
 * Add a dependency relationship between two parser contexts
 *
 * This function establishes a dependency relationship where the source
 * context depends on the target context. This is used to track file
 * dependencies such as includes and imports.
 *
 * @param source The source parser context that depends on the target
 * @param target The target parser context that is depended upon
 * @return true if successful, false otherwise
 * @note Implemented in parser_context.c
 */
extern bool parser_context_add_dependency(ParserContext *source, ParserContext *target);

/**
 * Add an AST node to a parser context
 *
 * This function adds an AST node to a parser context.
 * This is used when manually constructing ASTs or
 * when importing ASTs from another source.
 *
 * @param ctx The parser context to add the AST node to
 * @param node The AST node to add
 * @return true if successful, false otherwise
 */
bool parser_context_add_ast(ParserContext *ctx, ASTNode *node) {
  if (!ctx || !node) {
    return false;
  }

  // Check if we need to resize the AST nodes array
  if (ctx->num_ast_nodes >= ctx->ast_nodes_capacity) {
    size_t new_capacity = ctx->ast_nodes_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4; // Initial capacity
    }

    ASTNode **new_nodes = (ASTNode **)realloc(ctx->all_ast_nodes, new_capacity * sizeof(ASTNode *));
    if (!new_nodes) {
      log_error("Failed to resize AST nodes array");
      return false;
    }

    ctx->all_ast_nodes = new_nodes;
    ctx->ast_nodes_capacity = new_capacity;
  }

  // Add the AST node
  ctx->all_ast_nodes[ctx->num_ast_nodes++] = node;

  log_debug("Added AST node to parser context");

  return true;
}

/**
 * Add an AST node to a parser context with filename
 *
 * This function adds an AST node to a parser context and associates it
 * with a filename. This is used when manually constructing ASTs or
 * when importing ASTs from another source.
 *
 * @param ctx The parser context to add the AST node to
 * @param node The AST node to add
 * @param filename The filename associated with the AST node
 * @return true if successful, false otherwise
 * @note Implemented in parser_context.c
 */
extern bool parser_context_add_ast_with_filename(ParserContext *ctx, ASTNode *node,
                                                 const char *filename);

/**
 * Free a parser context (alias for parser_free)
 *
 * This function frees a parser context and all associated resources.
 * It is an alias for parser_free() to maintain consistent naming conventions.
 *
 * @param ctx The parser context to free
 * @note Implemented in parser_context.c
 */
extern void parser_context_free(ParserContext *ctx);
