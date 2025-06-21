/**
 * @file ast_post_processor.c
 * @brief Implementation of AST post-processing operations
 *
 * This file contains functions for ordering, categorizing, and cleaning up
 * AST nodes after initial construction. It extracts the large post-processing
 * section from ts_tree_to_ast() to improve maintainability.
 */

#include "../../include/scopemux/processors/ast_post_processor.h"

// File-level logging toggle. Set to true to enable logs for this file.
static bool enable_logging = false;
#include "../../include/scopemux/logging.h"

#include <stdlib.h>
#include <string.h>

// For node removal marking during processing
#define NODE_REMOVED 9999

/**
 * Post-processes the AST tree to ensure consistent ordering and structure
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 * @return The post-processed AST root
 */
ASTNode *post_process_ast(ASTNode *ast_root, ParserContext *ctx) {
  if (!ast_root) {
    return NULL;
  }

  if (enable_logging) log_debug("Starting AST post-processing");

  // Order nodes by priority type
  order_ast_nodes(ast_root);

  // Clean up temporary nodes and finalize structure
  cleanup_ast_nodes(ast_root);

  if (enable_logging) log_debug("AST post-processing complete");
  return ast_root;
}

/**
 * Orders nodes in the AST based on priority
 * Docstrings -> Includes -> Functions -> Other nodes
 *
 * @param ast_root The root AST node
 */
void order_ast_nodes(ASTNode *ast_root) {
  if (!ast_root || ast_root->num_children == 0) {
    return;
  }

  if (enable_logging) log_debug("Reordering AST nodes by priority type");

  // Create temporary arrays to categorize nodes
  ASTNode **docstring_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);
  ASTNode **include_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);
  ASTNode **function_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);
  ASTNode **other_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);

  if (!docstring_nodes || !include_nodes || !function_nodes || !other_nodes) {
    // Handle allocation failure
    if (enable_logging) log_error("Memory allocation failed during AST node reordering");
    if (docstring_nodes)
      free(docstring_nodes);
    if (include_nodes)
      free(include_nodes);
    if (function_nodes)
      free(function_nodes);
    if (other_nodes)
      free(other_nodes);
    return;
  }

  // Initialize counters
  size_t doc_count = 0;
  size_t inc_count = 0;
  size_t func_count = 0;
  size_t other_count = 0;

  // Categorize nodes by type
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *child = ast_root->children[i];
    if (!child || child->type == NODE_REMOVED)
      continue;

    if (child->type == NODE_DOCSTRING) {
      docstring_nodes[doc_count++] = child;
    } else if (child->type == NODE_INCLUDE) {
      include_nodes[inc_count++] = child;
    } else if (child->type == NODE_FUNCTION) {
      function_nodes[func_count++] = child;
    } else {
      other_nodes[other_count++] = child;
    }
  }

  if (enable_logging) log_debug("Categorized nodes - Docstrings: %zu, Includes: %zu, Functions: %zu, Other: %zu",
            doc_count, inc_count, func_count, other_count);

  // Reconstruct children array in order: DOCSTRING -> INCLUDE -> FUNCTION -> OTHER
  size_t new_index = 0;

  // Add docstring nodes first
  for (size_t i = 0; i < doc_count; i++) {
    ast_root->children[new_index++] = docstring_nodes[i];
  }

  // Add include nodes next
  for (size_t i = 0; i < inc_count; i++) {
    ast_root->children[new_index++] = include_nodes[i];
  }

  // Add function nodes next
  for (size_t i = 0; i < func_count; i++) {
    ast_root->children[new_index++] = function_nodes[i];
  }

  // Add other nodes last
  for (size_t i = 0; i < other_count; i++) {
    ast_root->children[new_index++] = other_nodes[i];
  }

  // Clean up temporary arrays
  free(docstring_nodes);
  free(include_nodes);
  free(function_nodes);
  free(other_nodes);

  if (enable_logging) log_debug("Reordered AST nodes by priority");
}

/**
 * Removes temporary and unwanted nodes from the AST
 *
 * @param ast_root The root AST node
 * @return The number of remaining nodes after cleanup
 */
size_t cleanup_ast_nodes(ASTNode *ast_root) {
  if (!ast_root) {
    return 0;
  }

  if (enable_logging) log_debug("Cleaning up temporary and removed nodes");

  // Final cleanup pass - remove any nodes marked as NODE_REMOVED
  size_t final_count = 0;
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *child = ast_root->children[i];
    if (!child || child->type == NODE_REMOVED) {
      // Free the removed node
      if (child)
        ast_node_free(child);
      continue;
    }

    // Ensure removal of ALL comment and docstring nodes from the final AST
    // This is critical for expected test output
    if (child->type == NODE_COMMENT || child->type == NODE_DOCSTRING) {
      ast_node_free(child);
      continue;
    }

    // Keep this node
    if (final_count != i) {
      ast_root->children[final_count] = child;
    }
    final_count++;
  }

  // Update the child count
  ast_root->num_children = final_count;

  if (enable_logging) log_debug("AST cleanup complete, %zu nodes remaining", final_count);
  return final_count;
}
