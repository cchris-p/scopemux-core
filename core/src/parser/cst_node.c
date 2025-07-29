/**
 * @file cst_node.c
 * @brief Implementation of CST node lifecycle and management functions
 *
 * This file contains implementation for CST node creation, manipulation,
 * and memory management to ensure proper handling of concrete syntax trees.
 */

#include "../../core/src/parser/cst_node.h"
#include "../../core/include/scopemux/memory_debug.h"
#include "../../core/include/scopemux/memory_management.h"
#include "../../core/src/parser/memory_tracking.h"
#include "../../core/src/parser/parser_internal.h"
#include <assert.h>

/**
 * Creates a new CST node with the specified type and content.
 */
CSTNode *cst_node_new(const char *type, char *content) {
  if (!type) {
    log_error("Cannot create CST node with NULL type");
    return NULL;
  }

  CSTNode *node = (CSTNode *)safe_malloc(sizeof(CSTNode));
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
  node->is_freed = 0; // DEBUG: Not freed yet
  node->owns_content = (content != NULL) ? 1 : 0;

  // Register the node for tracking
  register_cst_node(node, type);

  log_debug("[CSTNode NEW] node=%p type=%s content=%p", (void *)node, SAFE_STR(type),
            (void *)content);

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
    content_copy = safe_strdup(node->content);
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
      safe_free(content_copy);
    }
    return NULL;
  }

  // Recursively copy all children
  if (node->children_count > 0 && node->children) {
    new_node->children = (CSTNode **)safe_malloc(sizeof(CSTNode *) * node->children_count);

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
          safe_free(new_node->children);
          cst_node_free(new_node);
          return NULL;
        }
      } else {
        new_node->children[i] = NULL;
      }
    }
    new_node->children_count = node->children_count;
  }

  return new_node;
}

// --- Public CSTNode getters ---
const char *cst_node_get_type(const CSTNode *node) { return node ? node->type : NULL; }

size_t cst_node_get_child_count(const CSTNode *node) { return node ? node->children_count : 0; }

const CSTNode *cst_node_get_child(const CSTNode *node, size_t index) {
  if (!node || !node->children || index >= node->children_count)
    return NULL;
  return node->children[index];
}

SourceRange cst_node_get_range(const CSTNode *node) {
  static SourceRange empty = {0, 0, 0, 0, 0, 0};
  return node ? node->range : empty;
}

/**
 * Frees a CST node and all its children recursively.
 */
void cst_node_free(CSTNode *node) {
  if (!node) {
    return;
  }

  fprintf(stderr, "[CST_FREE] Entered for node=%p type=%s content=%p is_freed=%d\n", (void *)node,
          SAFE_STR(node->type), (void *)node->content, node->is_freed);
  log_debug("[CSTNode FREE] node=%p type=%s content=%p is_freed=%d", (void *)node,
            SAFE_STR(node->type), (void *)node->content, node->is_freed);

  // DEBUG: Assert not already freed
  if (node->is_freed) {
    log_error("[CSTNode DOUBLE FREE DETECTED] node=%p type=%s", (void *)node, SAFE_STR(node->type));
    assert(0 && "Double free detected for CSTNode");
  }
  node->is_freed = 1;

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
    fprintf(stderr, "[CST_FREE] Freeing children array for node=%p children=%p\n", (void *)node,
            (void *)node->children);
    safe_free(node->children);
    node->children = NULL;
  }

  // Ownership and freeing policy:
  // - type: NEVER freed here (not heap-allocated, not owned by CSTNode)
  // - content: Only free if non-NULL (ownership transferred at creation)
  // - children: Always free if allocated

  // Defensive: Never free node->type (not owned, not duplicated)
  // (If you change cst_node_new to strdup type, then update this logic)

  // Free the content if it exists and is owned
  if (node->content) {
    if (node->owns_content) {
      fprintf(stderr, "[CST_FREE] Freeing content for node=%p content=%p (owns_content=1)\n",
              (void *)node, (void *)node->content);
      safe_free(node->content);
      node->content = NULL;
    } else {
      fprintf(stderr,
              "[CST_FREE] Skipping free of content for node=%p content=%p (owns_content=0)\n",
              (void *)node, (void *)node->content);
    }
  }

  // Finally free the node itself
  fprintf(stderr, "[CST_FREE] Freeing node struct itself: node=%p\n", (void *)node);
  safe_free(node);
}

/**
 * Adds a child node to a parent CST node.
 */
bool cst_node_add_child(CSTNode *parent, CSTNode *child) {
  if (!parent || !child) {
    log_error("Cannot add child to CST node: %s", !parent ? "parent is NULL" : "child is NULL");
    return false;
  }

  log_debug("[CSTNode ADD_CHILD] parent=%p child=%p", (void *)parent, (void *)child);

  // Allocate or reallocate the children array
  unsigned int new_count = parent->children_count + 1;
  CSTNode **new_children;

  if (!parent->children) {
    // Initial allocation
    new_children = (CSTNode **)safe_malloc(sizeof(CSTNode *) * new_count);
  } else {
    // Reallocation for additional child
    new_children = (CSTNode **)safe_realloc(parent->children, sizeof(CSTNode *) * new_count);
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
