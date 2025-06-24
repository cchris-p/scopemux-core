/**
 * @file ts_cst_builder.c
 * @brief Implementation of Tree-sitter to CST conversion
 *
 * This module handles conversion of raw Tree-sitter trees into ScopeMux's
 * Concrete Syntax Tree (CST) representation. It follows the Single Responsibility
 * Principle by focusing only on CST generation.
 *
 * NOTE: The public interface is provided by tree_sitter_integration.c,
 * which calls into this module via ts_tree_to_cst_impl.
 */

#include "../../core/include/scopemux/logging.h"
#include "../../core/include/scopemux/parser.h"
#include "../../core/include/scopemux/tree_sitter_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Helper to copy the text of a TSNode into a new string
 *
 * @param node Tree-sitter node
 * @param source_code Source code string
 * @return char* Heap-allocated string with node text (caller must free)
 */
static char *ts_node_to_string(TSNode node, const char *source_code) {
  if (ts_node_is_null(node) || !source_code) {
    return NULL;
  }

  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  uint32_t length = end_byte - start_byte;

  char *result = (char *)malloc(length + 1);
  if (!result) {
    return NULL;
  }

  memcpy(result, source_code + start_byte, length);
  result[length] = '\0';

  return result;
}

/**
 * @brief Recursively creates a CST node from a Tree-sitter node
 *
 * @param ts_node Tree-sitter node
 * @param source_code Source code string
 * @return CSTNode* Constructed CST node or NULL on failure
 */
static CSTNode *create_cst_from_ts_node(TSNode ts_node, const char *source_code) {
  if (ts_node_is_null(ts_node)) {
    return NULL;
  }

  // 1. Create a new CSTNode
  const char *type = ts_node_type(ts_node);
  char *content = ts_node_to_string(ts_node, source_code);
  CSTNode *cst_node = cst_node_new(type, content);
  if (!cst_node) {
    if (content) {
      free(content);
    }
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

/**
 * @brief Implementation of Tree-sitter to CST conversion
 *
 * This function is called by the facade ts_tree_to_cst function in
 * tree_sitter_integration.c. It handles the actual conversion of a
 * Tree-sitter parse tree into a ScopeMux CST.
 *
 * @param root_node Root node of the Tree-sitter syntax tree
 * @param ctx Parser context containing source code and other info
 * @return CSTNode* Root of the generated CST or NULL on failure
 */
CSTNode *ts_tree_to_cst_impl(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx || !ctx->source_code) {
    parser_set_error(ctx, -1, "Invalid context for CST generation");
    return NULL;
  }

  return create_cst_from_ts_node(root_node, ctx->source_code);
}
