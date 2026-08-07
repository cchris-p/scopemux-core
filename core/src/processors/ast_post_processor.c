/**
 * @file ast_post_processor.c
 * @brief Implementation of AST post-processing operations
 *
 * This file contains functions for ordering, categorizing, and cleaning up
 * AST nodes after initial construction. It extracts the large post-processing
 * section from ts_tree_to_ast() to improve maintainability.
 */

#include "../../core/include/scopemux/processors/ast_post_processor.h"

// File-level logging toggle. Set to true to enable logs for this file.
static bool enable_logging = false;
#include "../../core/include/scopemux/logging.h"

#include "../../core/include/scopemux/common.h"
#include "../../core/include/scopemux/memory_debug.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// For node removal marking during processing
#define NODE_REMOVED 9999

// Forward declarations
static void extract_missing_signatures(ASTNode *ast_root, ParserContext *ctx);
static void extract_signatures_for_node(ASTNode *node, ParserContext *ctx);
static int compare_nodes_by_line(const void *a, const void *b);
static void order_function_children_by_line(ASTNode *function_node);

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

  log_debug("DEBUG: Starting AST post-processing - this function IS being called");

  // Order nodes by priority type
  order_ast_nodes(ast_root);

  // Extract signatures for control flow nodes that don't have them
  extract_missing_signatures(ast_root, ctx);

  // Clean up temporary nodes and finalize structure
  cleanup_ast_nodes(ast_root, ctx);

  if (enable_logging)
    log_debug("AST post-processing complete");
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

  if (enable_logging)
    log_debug("Reordering AST nodes by priority type");

  // Create temporary arrays to categorize nodes
  ASTNode **docstring_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);
  ASTNode **include_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);
  ASTNode **function_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);
  ASTNode **other_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);

  if (!docstring_nodes || !include_nodes || !function_nodes || !other_nodes) {
    // Handle allocation failure
    if (enable_logging)
      log_error("Memory allocation failed during AST node reordering");
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

  if (enable_logging)
    log_debug("Categorized nodes - Docstrings: %zu, Includes: %zu, Functions: %zu, Other: %zu",
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

  // Sort each category by source line number before adding
  if (func_count > 1) {
    qsort(function_nodes, func_count, sizeof(ASTNode *), compare_nodes_by_line);
  }
  if (other_count > 1) {
    qsort(other_nodes, other_count, sizeof(ASTNode *), compare_nodes_by_line);
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

  if (enable_logging)
    log_debug("Reordered AST nodes by priority");

  // Recursively order children of function nodes by source line
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *child = ast_root->children[i];
    if (child && child->type == NODE_FUNCTION) {
      order_function_children_by_line(child);
    }
  }
}

/**
 * Recursively remove a node and all its children from parser context tracking.
 * This must be called before freeing nodes to prevent use-after-free errors.
 *
 * @param ctx Parser context for node tracking
 * @param node The node to remove recursively
 */
static void remove_node_recursively(ParserContext *ctx, ASTNode *node) {
  if (!ctx || !node) {
    return;
  }

  // First, recursively remove all children
  if (node->children && node->num_children > 0) {
    for (size_t i = 0; i < node->num_children; i++) {
      if (node->children[i]) {
        remove_node_recursively(ctx, node->children[i]);
      }
    }
  }

  // Then remove this node from parser context
  parser_remove_ast_node(ctx, node);
}

/**
 * Cleans up temporary nodes and finalizes the AST structure
 *
 * @param ast_root The root AST node
 * @param ctx Parser context for node tracking
 * @return The number of remaining nodes after cleanup
 */
size_t cleanup_ast_nodes(ASTNode *ast_root, ParserContext *ctx) {
  if (!ast_root) {
    return 0;
  }

  if (enable_logging)
    log_debug("Cleaning up temporary and removed nodes");

  // Final cleanup pass - remove any nodes marked as NODE_REMOVED
  size_t final_count = 0;
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *child = ast_root->children[i];
    if (!child || child->type == NODE_REMOVED) {
      // Skip removed nodes but don't free them here - let parser_clear handle it
      continue;
    }

    // Ensure removal of ALL comment and docstring nodes from the final AST
    // This is critical for expected test output
    if (child->type == NODE_COMMENT || child->type == NODE_DOCSTRING) {
      // Skip comment/docstring nodes but don't free them here - let parser_clear handle it
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

  if (enable_logging)
    log_debug("AST cleanup complete, %zu nodes remaining", final_count);
  return final_count;
}

/**
 * Recursively extract signatures for control flow nodes that don't have them
 * This is needed for nodes created outside the query processing (e.g., test nodes)
 *
 * @param node The AST node to process
 * @param ctx The parser context
 */
static void extract_signatures_for_node(ASTNode *node, ParserContext *ctx) {
  if (!node)
    return;

  // Debug: Log ALL nodes we're processing
  if (node->name && (strstr(node->name, "loop") || strstr(node->name, "condition"))) {
    log_debug("DEBUG: Processing node '%s' type=%d signature='%s'", node->name, node->type,
              node->signature ? node->signature : "NULL");
  }

  // Check if this is a control flow node without a signature
  if ((node->type == NODE_FOR_STATEMENT || node->type == NODE_WHILE_STATEMENT ||
       node->type == NODE_IF_STATEMENT || node->type == NODE_SWITCH_STATEMENT) &&
      (!node->signature || strlen(node->signature) == 0)) {

    // Always log this for debugging
    log_debug("Extracting signature for control flow node '%s' (type=%d, raw_content=%s)",
              node->name ? node->name : "(null)", node->type,
              node->raw_content ? "present" : "NULL");

    // Try to extract signature from raw_content if it exists
    if (node->raw_content) {
      char *brace_pos = strchr(node->raw_content, '{');
      if (brace_pos) {
        size_t sig_len = brace_pos - node->raw_content;
        // Trim trailing whitespace
        while (sig_len > 0 && isspace(node->raw_content[sig_len - 1])) {
          sig_len--;
        }

        if (sig_len > 0) {
          char *signature =
              memory_debug_malloc(sig_len + 1, __FILE__, __LINE__, "control_flow_signature");
          if (signature) {
            strncpy(signature, node->raw_content, sig_len);
            signature[sig_len] = '\0';

            // Clean up whitespace
            char *src = signature, *dst = signature;
            bool last_was_space = true;
            while (*src) {
              if (isspace(*src)) {
                if (!last_was_space) {
                  *dst++ = ' ';
                  last_was_space = true;
                }
              } else {
                *dst++ = *src;
                last_was_space = false;
              }
              src++;
            }
            if (dst > signature && dst[-1] == ' ')
              dst--; // Remove trailing space
            *dst = '\0';

            ast_node_set_signature(node, signature, AST_SOURCE_DEBUG_ALLOC);
            if (enable_logging)
              log_debug("Set signature for '%s': '%s'", node->name ? node->name : "(null)",
                        signature);
          }
        }
      }
    }
  }

  // Recursively process children
  for (size_t i = 0; i < node->num_children; i++) {
    extract_signatures_for_node(node->children[i], ctx);
  }
}

/**
 * Extract signatures for control flow nodes that don't have them
 * This handles nodes created outside normal query processing
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 */
static void extract_missing_signatures(ASTNode *ast_root, ParserContext *ctx) {
  log_debug("DEBUG: Starting missing signature extraction");

  extract_signatures_for_node(ast_root, ctx);

  if (enable_logging)
    log_debug("Missing signature extraction complete");
}

/**
 * Comparison function for sorting AST nodes by source line number
 * Used with qsort to ensure nodes appear in source code order
 *
 * @param a First ASTNode pointer (as void*)
 * @param b Second ASTNode pointer (as void*)
 * @return Negative if a comes before b, positive if after, 0 if equal
 */
static int compare_nodes_by_line(const void *a, const void *b) {
  ASTNode *node_a = *(ASTNode **)a;
  ASTNode *node_b = *(ASTNode **)b;

  if (!node_a || !node_b) {
    return 0; // Equal if either is null
  }

  // Compare by start line number
  if (node_a->range.start.line < node_b->range.start.line) {
    return -1;
  } else if (node_a->range.start.line > node_b->range.start.line) {
    return 1;
  } else {
    // Same line, compare by column
    if (node_a->range.start.column < node_b->range.start.column) {
      return -1;
    } else if (node_a->range.start.column > node_b->range.start.column) {
      return 1;
    }
  }

  return 0; // Equal position
}

/**
 * Orders the children of a function node by source line number
 * This ensures variables and control flow appear in source code order
 *
 * @param function_node The function node whose children should be ordered
 */
static void order_function_children_by_line(ASTNode *function_node) {
  if (!function_node || function_node->num_children <= 1) {
    return;
  }

  if (enable_logging)
    log_debug("Ordering function '%s' children by source line",
              function_node->name ? function_node->name : "(null)");

  // Sort all children by source line number
  qsort(function_node->children, function_node->num_children, sizeof(ASTNode *),
        compare_nodes_by_line);

  if (enable_logging)
    log_debug("Ordered %zu children of function '%s'", function_node->num_children,
              function_node->name ? function_node->name : "(null)");
}
