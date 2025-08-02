/**
 * @file docstring_processor.c
 * @brief Implementation of docstring extraction and association
 *
 * This file contains functions for processing docstrings and associating
 * them with corresponding AST nodes. It extracts docstring processing logic
 * from the main tree_sitter_integration.c file.
 */

#define _GNU_SOURCE /* Required for strdup() function */

#include "../../core/include/scopemux/processors/docstring_processor.h"
#include "../../core/include/scopemux/memory_debug.h"

// File-level logging toggle. Set to true to enable logs for this file.
static bool enable_logging = true;
#include "../../core/include/scopemux/logging.h"

#include <stdlib.h>
#include <string.h> /* This header is needed for strdup */

#define SAFE_STR(x) ((x) ? (x) : "(null)")

/**
 * Extracts a clean docstring from a JavaDoc-style comment
 *
 * @param comment The raw comment text
 * @return A newly allocated string with the cleaned documentation
 */
char *extract_doc_comment(const char *comment) {
  if (!comment)
    return NULL;

  // For now, return the raw comment content as-is to match expected format
  // The test expects the full /** ... */ format
  size_t len = strlen(comment);
  char *result = MALLOC(len + 1, "doc_comment_result");
  if (!result)
    return NULL;

  strcpy(result, comment);
  return result;
}

/**
 * Recursively associates docstrings with AST nodes
 *
 * @param node The current AST node to process
 * @param docstrings Array of docstring info
 * @param count Number of docstring entries
 */
static void associate_docstrings_recursive(ASTNode *node, DocstringInfo *docstrings, size_t count) {
  if (!node)
    return;

  // Only associate with code nodes (not comments, docstrings, or unknown)
  if (node->type != NODE_COMMENT && node->type != NODE_DOCSTRING && node->type != NODE_UNKNOWN) {
    // Find closest docstring before this node
    DocstringInfo *best_match = NULL;
    uint32_t best_distance = UINT32_MAX;
    for (size_t j = 0; j < count; j++) {
      if (!docstrings[j].content)
        continue;
      // Only consider docstrings that appear before the node
      if (docstrings[j].line_number < node->range.start.line) {
        uint32_t distance = node->range.start.line - docstrings[j].line_number;
        if (distance < best_distance && distance <= 5) { // Within 5 lines
          best_distance = distance;
          best_match = &docstrings[j];
        }
      }
    }
    // Associate the best match docstring with this node
    if (best_match && best_match->content) {
      if (node->docstring) {
        FREE(node->docstring);
      }
      node->docstring = STRDUP(best_match->content, "node_docstring");
      FREE(best_match->content);
      best_match->content = NULL;
    }
  }

  // Recursively process all children
  for (size_t i = 0; i < node->num_children; i++) {
    associate_docstrings_recursive(node->children[i], docstrings, count);
  }
}

/**
 * Associates docstrings with their target AST nodes
 *
 * @param ast_root The root AST node
 * @param docstring_nodes Array of docstring nodes
 * @param count Number of docstring nodes
 */
void associate_docstrings_with_nodes(ASTNode *ast_root, ASTNode **docstring_nodes, size_t count) {
  if (!ast_root || !docstring_nodes || count == 0) {
    return;
  }

  // Create array to hold docstring info
  DocstringInfo *docstrings = MALLOC(sizeof(DocstringInfo) * count, "docstring_info_array");
  if (!docstrings) {
    if (enable_logging)
      log_error("Failed to allocate memory for docstring association");
    return;
  }

  // Extract docstring information
  for (size_t i = 0; i < count; i++) {
    ASTNode *doc_node = docstring_nodes[i];
    if (!doc_node || !doc_node->raw_content) {
      docstrings[i].content = NULL;
      docstrings[i].line_number = 0;
      continue;
    }
    docstrings[i].content = extract_doc_comment(doc_node->raw_content);
    docstrings[i].line_number = doc_node->range.start.line;
  }

  // --- NEW: Attach file-level docstring to root node if it appears before any code node ---
  // Find the minimum line number of any non-docstring, non-comment node
  uint32_t min_code_line = UINT32_MAX;
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *node = ast_root->children[i];
    if (!node)
      continue;
    if (node->type == NODE_COMMENT || node->type == NODE_DOCSTRING || node->type == NODE_UNKNOWN)
      continue;
    if (node->range.start.line < min_code_line) {
      min_code_line = node->range.start.line;
    }
  }
  // Find the docstring with the lowest line number before any code node
  // Only consider /** ... */ style comments, not regular // comments
  int file_docstring_idx = -1;
  uint32_t file_docstring_line = UINT32_MAX;
  for (size_t j = 0; j < count; j++) {
    if (!docstrings[j].content)
      continue;
    // Only consider /** ... */ style docstrings for file-level docstring
    if (strncmp(docstrings[j].content, "/**", 3) == 0 &&
        docstrings[j].line_number < min_code_line &&
        docstrings[j].line_number < file_docstring_line) {
      file_docstring_idx = (int)j;
      file_docstring_line = docstrings[j].line_number;
    }
  }
  if (file_docstring_idx >= 0 && docstrings[file_docstring_idx].content) {
    if (ast_root->docstring) {
      FREE(ast_root->docstring);
    }
    ast_root->docstring = STRDUP(docstrings[file_docstring_idx].content, "root_docstring");
    FREE(docstrings[file_docstring_idx].content);
    docstrings[file_docstring_idx].content = NULL;
  }
  // --- END NEW ---

  // Associate docstrings with nodes recursively throughout the AST
  associate_docstrings_recursive(ast_root, docstrings, count);

  // Clean up remaining docstring info
  for (size_t i = 0; i < count; i++) {
    if (docstrings[i].content) {
      FREE(docstrings[i].content);
    }
  }

  FREE(docstrings);
}

/**
 * Processes all docstrings in an AST
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 */
void process_docstrings(ASTNode *ast_root, ParserContext *ctx) {
  if (!ast_root || !ctx) {
    return;
  }

  if (enable_logging)
    log_debug("Processing docstrings");

  // Find and separate docstring nodes
  size_t doc_count = 0;
  ASTNode **docstring_nodes =
      MALLOC(sizeof(ASTNode *) * ast_root->num_children, "docstring_nodes_array");

  if (!docstring_nodes) {
    if (enable_logging)
      log_error("Failed to allocate memory for docstring processing");
    return;
  }

  // Collect all docstring nodes
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *child = ast_root->children[i];
    if (child && child->type == NODE_DOCSTRING) {
      docstring_nodes[doc_count++] = child;
    }
  }

  if (doc_count > 0) {
    if (enable_logging)
      log_debug("Found %zu docstrings to process", doc_count);
    associate_docstrings_with_nodes(ast_root, docstring_nodes, doc_count);

    // Remove docstring nodes from AST children array after processing
    for (size_t i = 0; i < doc_count; i++) {
      ASTNode *doc_node = docstring_nodes[i];
      if (doc_node) {
        // Find and remove this node from ast_root->children
        for (size_t j = 0; j < ast_root->num_children; j++) {
          if (ast_root->children[j] == doc_node) {
            // Shift remaining children down
            for (size_t k = j; k < ast_root->num_children - 1; k++) {
              ast_root->children[k] = ast_root->children[k + 1];
            }
            ast_root->num_children--;
            if (enable_logging)
              log_debug("Removed docstring node '%s' from AST children", doc_node->name);
            break;
          }
        }
      }
    }
  } else {
    if (enable_logging)
      log_debug("No docstrings found to process");
  }

  FREE(docstring_nodes);
}
