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
 */
bool parser_context_add_dependency(ParserContext *source, ParserContext *target) {
  if (!source || !target) {
    return false;
  }

  // Check if dependency already exists
  for (size_t i = 0; i < source->num_dependencies; i++) {
    if (source->dependencies[i] == target) {
      return true; // Already exists
    }
  }

  // Check if we need to resize the dependencies array
  if (source->num_dependencies >= source->dependencies_capacity) {
    size_t new_capacity = source->dependencies_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4; // Initial capacity
    }

    ParserContext **new_deps =
        (ParserContext **)realloc(source->dependencies, new_capacity * sizeof(ParserContext *));
    if (!new_deps) {
      log_error("Failed to resize dependencies array");
      return false;
    }

    source->dependencies = new_deps;
    source->dependencies_capacity = new_capacity;
  }

  // Add the dependency
  source->dependencies[source->num_dependencies++] = target;
  log_debug("Added dependency from %s to %s", source->filename ? source->filename : "(unnamed)",
            target->filename ? target->filename : "(unnamed)");

  return true;
}

/**
 * Add an AST node to a parser context
 *
 * This function adds an AST node to a parser context and associates it
 * with a filename. This is used when manually constructing ASTs or
 * when importing ASTs from another source.
 *
 * @param ctx The parser context to add the AST node to
 * @param node The AST node to add
 * @param filename The filename associated with the AST node
 * @return true if successful, false otherwise
 */
bool parser_context_add_ast(ParserContext *ctx, ASTNode *node, const char *filename) {
  if (!ctx || !node) {
    return false;
  }

  // Check if we need to resize the AST nodes array
  if (ctx->num_ast_nodes >= ctx->ast_capacity) {
    size_t new_capacity = ctx->ast_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4; // Initial capacity
    }

    ASTNode **new_nodes = (ASTNode **)realloc(ctx->all_ast_nodes, new_capacity * sizeof(ASTNode *));
    if (!new_nodes) {
      log_error("Failed to resize AST nodes array");
      return false;
    }

    ctx->all_ast_nodes = new_nodes;
    ctx->ast_capacity = new_capacity;
  }

  // Add the AST node
  ctx->all_ast_nodes[ctx->num_ast_nodes++] = node;

  // Set the filename if provided and not already set
  if (filename && !ctx->filename) {
    ctx->filename = strdup(filename);
  }

  log_debug("Added AST node to parser context for %s", ctx->filename ? ctx->filename : "(unnamed)");

  return true;
}

/**
 * Create a new parser context (alias for parser_init)
 *
 * This function creates a new parser context. It is an alias for
 * parser_init() to maintain consistent naming conventions.
 *
 * @return A newly allocated parser context, or NULL on failure
 */
ParserContext *parser_context_create(void) { return parser_init(); }
