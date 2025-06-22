/**
 * @file docstring_processor.c
 * @brief Implementation of docstring extraction and association
 *
 * This file contains functions for processing docstrings and associating
 * them with corresponding AST nodes. It extracts docstring processing logic
 * from the main tree_sitter_integration.c file.
 */

#define _GNU_SOURCE /* Required for strdup() function */

#include "../../include/scopemux/processors/docstring_processor.h"

// File-level logging toggle. Set to true to enable logs for this file.
static bool enable_logging = false;
#include "../../include/scopemux/logging.h"

#include <stdlib.h>
#include <string.h> /* This header is needed for strdup */

/**
 * Extracts a clean docstring from a JavaDoc-style comment
 *
 * @param comment The raw comment text
 * @return A newly allocated string with the cleaned documentation
 */
char *extract_doc_comment(const char *comment) {
  if (!comment)
    return NULL;

  // Skip the opening /**
  const char *start = comment;
  while (*start && (*start == '/' || *start == '*' || *start == ' '))
    start++;

  // Allocate space for the cleaned comment
  char *result = malloc(strlen(comment) + 1);
  if (!result)
    return NULL;

  char *dst = result;
  const char *src = start;

  // Process each character
  while (*src) {
    if (*src == '\n') {
      // Keep the newline
      *dst++ = *src++;
      // Skip leading whitespace and asterisks on the next line
      while (*src && (*src == ' ' || *src == '*' || *src == '\t'))
        src++;
    } else {
      *dst++ = *src++;
    }
  }

  // Null terminate
  *dst = '\0';

  // Remove trailing */
  char *end = strstr(result, "*/");
  if (end)
    *end = '\0';

  return result;
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
  DocstringInfo *docstrings = malloc(sizeof(DocstringInfo) * count);
  if (!docstrings) {
    if (enable_logging) log_error("Failed to allocate memory for docstring association");
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

  // Associate docstrings with nodes based on proximity
  for (size_t i = 0; i < ast_root->num_children; i++) {
    ASTNode *node = ast_root->children[i];
    if (!node)
      continue;

    // Skip nodes that aren't appropriate for docstring association
    if (node->type == NODE_COMMENT || node->type == NODE_DOCSTRING || node->type == NODE_UNKNOWN ||
        node->type == NODE_UNKNOWN) {
      continue;
    }

    // Find closest docstring before this node
    DocstringInfo *best_match = NULL;
    uint32_t best_distance = UINT32_MAX;

    for (size_t j = 0; j < count; j++) {
      if (!docstrings[j].content)
        continue;

      // Only consider docstrings that appear before the node
      if (docstrings[j].line_number < node->range.start.line) {
        uint32_t distance = node->range.start.line - docstrings[j].line_number;

        // Update best match if this docstring is closer
        if (distance < best_distance && distance <= 5) { // Within 5 lines
          best_distance = distance;
          best_match = &docstrings[j];
        }
      }
    }

    // Associate the best match docstring with this node
    if (best_match && best_match->content) {
      if (node->docstring) {
        free(node->docstring);
      }
      node->docstring = strdup(best_match->content);

      // Mark this docstring as used
      free(best_match->content);
      best_match->content = NULL;
    }
  }

  // Clean up remaining docstring info
  for (size_t i = 0; i < count; i++) {
    if (docstrings[i].content) {
      free(docstrings[i].content);
    }
  }

  free(docstrings);
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

  if (enable_logging) log_debug("Processing docstrings");

  // Find and separate docstring nodes
  size_t doc_count = 0;
  ASTNode **docstring_nodes = malloc(sizeof(ASTNode *) * ast_root->num_children);

  if (!docstring_nodes) {
    if (enable_logging) log_error("Failed to allocate memory for docstring processing");
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
    if (enable_logging) log_debug("Found %zu docstrings to process", doc_count);
    associate_docstrings_with_nodes(ast_root, docstring_nodes, doc_count);
  } else {
    if (enable_logging) log_debug("No docstrings found to process");
  }

  free(docstring_nodes);
}
