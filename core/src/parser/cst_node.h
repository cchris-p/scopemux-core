/**
 * @file cst_node.h
 * @brief CST node lifecycle and management functions
 *
 * Contains definitions and functions for working with CST nodes,
 * including creation, manipulation, and memory management.
 */

#ifndef SCOPEMUX_CST_NODE_H
#define SCOPEMUX_CST_NODE_H

#include "../../include/scopemux/parser.h"

// Full definition of CSTNode
struct CSTNode {
  const char *type;
  char *content;
  SourceRange range;
  struct CSTNode **children;
  unsigned int children_count;
  int is_freed; // DEBUG: Set to 1 when node is freed
  uint8_t owns_content; // 1 if content is heap-allocated and should be freed, 0 otherwise
};

/**
 * @brief Create a new CST node
 *
 * @param type Node type string (from Tree-sitter, not copied)
 * @param content Source code content of the node (ownership transferred)
 * @param range Source range of the node
 * @return CSTNode* Created node or NULL on failure
 */
CSTNode *cst_node_new(const char *type, char *content);

/**
 * @brief Creates a deep copy of a CST node and all its children
 *
 * @param node The node to copy
 * @return CSTNode* A new allocated copy or NULL on failure
 */
CSTNode *cst_node_copy_deep(const CSTNode *node);

/**
 * @brief Free a CST node and all its children recursively
 *
 * @param node The CST node to free
 */
void cst_node_free(CSTNode *node);

/**
 * @brief Add a child node to a parent CST node
 *
 * @param parent Parent node
 * @param child Child node
 * @return bool True on success, false on failure
 */
bool cst_node_add_child(CSTNode *parent, CSTNode *child);

#endif /* SCOPEMUX_CST_NODE_H */
