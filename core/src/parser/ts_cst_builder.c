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
#include "cst_node.h"
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAFE_STR(x) ((x) ? (x) : "(null)")

// External declaration for segfault handler
extern void segfault_handler(int sig);

/**
 * @brief Helper to copy the text of a TSNode into a new string
 *
 * @param node Tree-sitter node
 * @param source_code Source code string
 * @return char* Heap-allocated string with node text (caller must free)
 */
static char *ts_node_to_string(TSNode node, const char *source_code) {
  // Ultra-early log for pointer values
  log_debug("ts_node_to_string: entry, node ptr=%p, source_code ptr=%p", (void *)&node,
            (void *)source_code);
  log_debug("ts_node_to_string: ts_node_is_null(node)=%d", ts_node_is_null(node));
  if (ts_node_is_null(node) || !source_code) {
    log_error("ts_node_to_string: Null node or source_code");
    return NULL;
  }

  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  uint32_t length = end_byte > start_byte ? end_byte - start_byte : 0;
  size_t source_len = strlen(source_code);

  log_debug("ts_node_to_string: start_byte=%u, end_byte=%u, length=%u, source_len=%zu", start_byte,
            end_byte, length, source_len);

  // Defensive bounds checks
  if (start_byte > end_byte) {
    log_error("ts_node_to_string: start_byte > end_byte (%u > %u)", start_byte, end_byte);
    return NULL;
  }
  if (end_byte > source_len) {
    log_error("ts_node_to_string: end_byte (%u) out of bounds (source_len=%zu)", end_byte,
              source_len);
    return NULL;
  }
  if (start_byte > source_len) {
    log_error("ts_node_to_string: start_byte (%u) out of bounds (source_len=%zu)", start_byte,
              source_len);
    return NULL;
  }
  if (length == 0) {
    log_warning("ts_node_to_string: zero-length node (start_byte=%u, end_byte=%u)", start_byte,
                end_byte);
    // Allow zero-length but return empty string
    char *result = (char *)malloc(1);
    if (result)
      result[0] = '\0';
    return result;
  }

  char *result = (char *)malloc(length + 1);
  if (!result) {
    log_error("ts_node_to_string: malloc failed for length %u", length);
    return NULL;
  }

  memcpy(result, source_code + start_byte, length);
  result[length] = '\0';

  log_debug("ts_node_to_string: successfully copied node text: '%.20s%s'", SAFE_STR(result),
            length > 20 ? "..." : "");

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
    log_debug("Skipping null Tree-sitter node");
    return NULL;
  }

  if (!source_code) {
    log_error("Null source code pointer passed to create_cst_from_ts_node");
    return NULL;
  }

  // 1. Create a new CSTNode
  const char *type = ts_node_type(ts_node);
  if (!type) {
    log_error("Failed to get node type from Tree-sitter node");
    return NULL;
  }

  log_debug("Creating CST node for type: %s", SAFE_STR(type));

  // Get node content with validation
  char *content = NULL;

  // Ultra-early logging before calling ts_node_to_string
  log_debug("create_cst_from_ts_node: ts_node pointer=%p, source_code pointer=%p", (void *)&ts_node,
            (void *)source_code);
  log_debug("create_cst_from_ts_node: ts_node_is_null=%d", ts_node_is_null(ts_node));

  // Use a try/catch mechanism with setjmp/longjmp to catch potential segfaults
  // in ts_node_to_string which might occur with invalid nodes
  jmp_buf node_content_recovery;
  if (setjmp(node_content_recovery) != 0) {
    log_error("Recovered from potential crash in ts_node_to_string for node type: %s",
              SAFE_STR(type));
    return NULL;
  }

  // Set up signal handler for this operation
  void (*prev_handler)(int) = signal(SIGSEGV, segfault_handler);

  // Try to get content safely
  content = ts_node_to_string(ts_node, source_code);

  // Restore signal handler
  signal(SIGSEGV, prev_handler);

  if (!content) {
    log_error("Failed to get content for node type: %s", SAFE_STR(type));
    return NULL;
  }

  // Create the CST node
  CSTNode *cst_node = cst_node_new(type, content);
  if (!cst_node) {
    log_error("Failed to create CST node for type: %s", SAFE_STR(type));
    if (content) {
      free(content);
    }
    return NULL;
  }

  // 2. Set the source range with validation
  TSPoint start_point = ts_node_start_point(ts_node);
  TSPoint end_point = ts_node_end_point(ts_node);

  // Validate points to avoid invalid memory access
  if (start_point.row < 0 || end_point.row < 0 || start_point.column < 0 || end_point.column < 0) {
    log_error("Invalid source range for node type: %s", SAFE_STR(type));
    cst_node_free(cst_node);
    return NULL;
  }

  cst_node->range.start.line = start_point.row;
  cst_node->range.end.line = end_point.row;
  cst_node->range.start.column = start_point.column;
  cst_node->range.end.column = end_point.column;

  log_debug("CST node range: (%d:%d) - (%d:%d) for type: %s", start_point.row, start_point.column,
            end_point.row, end_point.column, SAFE_STR(type));

  // 3. Recursively process all children with validation
  uint32_t child_count = ts_node_child_count(ts_node);

  if (child_count > 1000) { // Sanity check for unreasonable child counts
    log_warning("Unusually high child count (%u) for node type: %s - limiting to 1000", child_count,
                SAFE_STR(type));
    child_count = 1000; // Cap to avoid excessive recursion
  }

  log_debug("Processing %u children for node type: %s", child_count, SAFE_STR(type));

  for (uint32_t i = 0; i < child_count; ++i) {
    // Get child with validation
    TSNode ts_child = ts_node_child(ts_node, i);
    if (ts_node_is_null(ts_child)) {
      log_debug("Skipping null child at index %u", i);
      continue;
    }

    // Recursively process child
    CSTNode *cst_child = create_cst_from_ts_node(ts_child, source_code);
    if (cst_child) {
      if (!cst_node_add_child(cst_node, cst_child)) {
        log_error("Failed to add child node to parent");
        cst_node_free(cst_child); // Clean up if add failed
      }
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
  log_debug("ts_tree_to_cst_impl: Starting CST generation");

  // Validate input parameters
  if (ts_node_is_null(root_node)) {
    log_error("ts_tree_to_cst_impl: Root node is null");
    parser_set_error(ctx, -1, "Root node is null for CST generation");
    return NULL;
  }

  if (!ctx) {
    log_error("ts_tree_to_cst_impl: Parser context is null");
    return NULL;
  }

  if (!ctx->source_code) {
    log_error("ts_tree_to_cst_impl: Source code is null in parser context");
    parser_set_error(ctx, -1, "Source code is null for CST generation");
    return NULL;
  }

  // Set up protection against segfaults during CST generation
  jmp_buf cst_recovery;
  if (setjmp(cst_recovery) != 0) {
    log_error("ts_tree_to_cst_impl: Recovered from crash in CST generation");
    parser_set_error(ctx, 8, "Parser crashed during CST generation");
    return NULL;
  }

  // Install signal handler for this operation
  void (*prev_handler)(int) = signal(SIGSEGV, segfault_handler);

  // Log detailed information about the root node
  const char *root_type = ts_node_type(root_node);
  log_debug("ts_tree_to_cst_impl: Root node type: %s", SAFE_STR(root_type));

  // Attempt to create CST from root node with crash protection
  CSTNode *result = NULL;

  // Use setjmp/longjmp for error recovery
  if (setjmp(cst_recovery) != 0) {
    log_error("ts_tree_to_cst_impl: Recovered from crash in create_cst_from_ts_node");
    parser_set_error(ctx, 8, "Parser crashed during CST node creation");
    result = NULL;
  } else {
    // Only attempt to create CST if we haven't jumped here from a crash
    result = create_cst_from_ts_node(root_node, ctx->source_code);
  }

  // Restore the previous signal handler
  signal(SIGSEGV, prev_handler);

  if (!result) {
    log_error("ts_tree_to_cst_impl: Failed to create CST from Tree-sitter tree");
    parser_set_error(ctx, -1, "Failed to create CST from Tree-sitter tree");
  } else {
    log_debug("ts_tree_to_cst_impl: Successfully created CST");
  }

  return result;
}
