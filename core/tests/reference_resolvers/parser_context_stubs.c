/**
 * @file parser_context_stubs.c
 * @brief Stub implementations for parser context API for testing
 *
 * This file provides minimal implementations of parser context functions
 * to enable testing of reference resolvers without requiring the full
 * parser implementation.
 */

#include "scopemux/memory_debug.h"
#include "scopemux/parser.h"
#include <stdlib.h>
#include <string.h>

// Basic parser context structure
typedef struct ParserContextImpl {
  ASTNode **asts;
  char **file_paths;
  size_t num_asts;
  size_t capacity;
} ParserContextImpl;

// Create a new parser context
ParserContext *parser_init(void) {
  ParserContextImpl *ctx = MALLOC(sizeof(ParserContextImpl), "parser_context_impl");
  if (!ctx) {
    return NULL;
  }

  ctx->asts = NULL;
  ctx->file_paths = NULL;
  ctx->num_asts = 0;
  ctx->capacity = 0;

  return (ParserContext *)ctx;
}

// Free parser context resources
void parser_free(ParserContext *ctx) {
  ParserContextImpl *impl = (ParserContextImpl *)ctx;
  if (!impl) {
    return;
  }

  // Free owned resources
  for (size_t i = 0; i < impl->num_asts; i++) {
    // Don't free ASTs as they're owned elsewhere in tests
    FREE(impl->file_paths[i]);
  }

  FREE(impl->asts);
  FREE(impl->file_paths);
  FREE(impl);
}

// Add AST to parser context
bool parser_add_ast_node(ParserContext *ctx, ASTNode *node) {
  ParserContextImpl *impl = (ParserContextImpl *)ctx;
  if (!impl || !node) {
    return false;
  }

  // Grow arrays if needed
  if (impl->num_asts >= impl->capacity) {
    size_t new_capacity = impl->capacity == 0 ? 4 : impl->capacity * 2;
    ASTNode **new_asts =
        REALLOC(impl->asts, new_capacity * sizeof(ASTNode *), "parser_context_asts");
    char **new_paths =
        REALLOC(impl->file_paths, new_capacity * sizeof(char *), "parser_context_paths");

    if (!new_asts || !new_paths) {
      if (new_asts)
        FREE(new_asts);
      if (new_paths)
        FREE(new_paths);
      return false;
    }

    impl->asts = new_asts;
    impl->file_paths = new_paths;
    impl->capacity = new_capacity;
  }

  // Add the AST (file path is now handled separately)
  impl->asts[impl->num_asts] = node;
  impl->file_paths[impl->num_asts] =
      STRDUP("unknown_file", "parser_context_unknown_file"); // Default for stub implementation
  impl->num_asts++;

  return true;
}

// Backward compatibility function for tests
void parser_context_add_ast(ParserContext *ctx, ASTNode *ast, const char *file_path) {
  ParserContextImpl *impl = (ParserContextImpl *)ctx;
  if (!impl || !ast || !file_path) {
    return;
  }

  // Grow arrays if needed
  if (impl->num_asts >= impl->capacity) {
    size_t new_capacity = impl->capacity == 0 ? 4 : impl->capacity * 2;
    ASTNode **new_asts =
        REALLOC(impl->asts, new_capacity * sizeof(ASTNode *), "parser_context_asts");
    char **new_paths =
        REALLOC(impl->file_paths, new_capacity * sizeof(char *), "parser_context_paths");

    if (!new_asts || !new_paths) {
      if (new_asts)
        FREE(new_asts);
      if (new_paths)
        FREE(new_paths);
      return;
    }

    impl->asts = new_asts;
    impl->file_paths = new_paths;
    impl->capacity = new_capacity;
  }

  // Add the AST and file path
  impl->asts[impl->num_asts] = ast;
  impl->file_paths[impl->num_asts] = STRDUP(file_path, "parser_context_file_path");
  impl->num_asts++;
}

// Stub implementation for AST node child access
ASTNode *ast_node_get_child_at_index(ASTNode *node, size_t index) {
  if (!node || !node->children || index >= node->num_children) {
    return NULL;
  }
  return node->children[index];
}
