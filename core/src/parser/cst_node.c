/**
 * @file cst_node.c
 * @brief Implementation of CST node lifecycle and management functions
 *
 * This file contains implementation for CST node creation, manipulation,
 * and memory management to ensure proper handling of concrete syntax trees.
 */

#include "cst_node.h"
#include "memory_tracking.h"
#include "parser_internal.h"

/**
 * Creates a new CST node with the specified type and content.
 */
CSTNode *cst_node_new(const char *type, char *content) {
  if (!type) {
    log_error("Cannot create CST node with NULL type");
    return NULL;
  }

  CSTNode *node = (CSTNode *)memory_debug_malloc(sizeof(CSTNode), __FILE__, __LINE__, "cst_node");
  if (!node) {
    log_error("Failed to allocate memory for CST node");
    return NULL;
  }

  // Initialize the node with the provided values
  node->type = type;       // Note: type is not copied, assumed to be static or managed elsewhere
  node->content = content; // Content ownership is transferred
  // Initialize range to zero
  memset(&node->range, 0, sizeof(SourceRange));
  node->children = NULL;
  node->children_count = 0;

  // Register the node for tracking
  register_cst_node(node, type);

  return node;
}

/**
 * Creates a deep copy of a CST node and all its children.
 */
CSTNode *cst_node_copy_deep(const CSTNode *node) {
  if (!node) {
    return NULL;
  }

  // Create a new node with the same type
  char *content_copy = NULL;
  if (node->content) {
    content_copy = strdup(node->content);
    if (!content_copy) {
      log_error("Failed to duplicate CST node content");
      return NULL;
    }
  }

  // Create a new node with copied content
  CSTNode *new_node = cst_node_new(node->type, content_copy);

  // Manually copy the range information
  if (new_node) {
    new_node->range = node->range;
  }
  if (!new_node) {
    if (content_copy) {
      free(content_copy);
    }
    return NULL;
  }

  // Recursively copy all children
  if (node->children_count > 0 && node->children) {
    new_node->children = (CSTNode **)memory_debug_malloc(sizeof(CSTNode *) * node->children_count,
                                                         __FILE__, __LINE__, "cst_node_children");

    if (!new_node->children) {
      log_error("Failed to allocate memory for CST node children");
      cst_node_free(new_node);
      return NULL;
    }

    // Copy each child recursively
    for (unsigned int i = 0; i < node->children_count; i++) {
      if (node->children[i]) {
        new_node->children[i] = cst_node_copy_deep(node->children[i]);
        if (!new_node->children[i]) {
          // Clean up already copied children on failure
          for (unsigned int j = 0; j < i; j++) {
            cst_node_free(new_node->children[j]);
          }
          memory_debug_free(new_node->children, __FILE__, __LINE__);
          new_node->children = NULL;
          cst_node_free(new_node);
          return NULL;
        }
        new_node->children_count++;
      }
    }
  }

  return new_node;
}

/**
 * Frees a CST node and all its children recursively.
 */
void cst_node_free(CSTNode *node) {
  if (!node) {
    return;
  }

  // Track this free operation
  mark_cst_node_freed(node);

  // First free all children recursively
  if (node->children && node->children_count > 0) {
    for (unsigned int i = 0; i < node->children_count; i++) {
      if (node->children[i]) {
        cst_node_free(node->children[i]);
        node->children[i] = NULL; // Prevent double-free
      }
    }
    memory_debug_free(node->children, __FILE__, __LINE__);
    node->children = NULL;
  }

  // Free the content if it exists
  if (node->content) {
    memory_debug_free(node->content, __FILE__, __LINE__);
    node->content = NULL;
  }

  // Finally free the node itself
  memory_debug_free(node, __FILE__, __LINE__);
}

/**
 * Adds a child node to a parent CST node.
 */
bool cst_node_add_child(CSTNode *parent, CSTNode *child) {
  if (!parent || !child) {
    log_error("Cannot add child to CST node: %s", !parent ? "parent is NULL" : "child is NULL");
    return false;
  }

  // Allocate or reallocate the children array
  unsigned int new_count = parent->children_count + 1;
  CSTNode **new_children;

  if (!parent->children) {
    // Initial allocation
    new_children = (CSTNode **)memory_debug_malloc(sizeof(CSTNode *) * new_count, __FILE__,
                                                   __LINE__, "cst_node_children");
  } else {
    // Reallocation for additional child
    new_children = (CSTNode **)memory_debug_realloc(parent->children, sizeof(CSTNode *) * new_count,
                                                    __FILE__, __LINE__, "cst_node_children");
  }

  if (!new_children) {
    log_error("Failed to allocate memory for CST node child");
    return false;
  }

  // Store the new child
  parent->children = new_children;
  parent->children[parent->children_count] = child;
  parent->children_count = new_count;

  return true;
}
