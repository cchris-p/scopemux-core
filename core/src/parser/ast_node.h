/**
 * @file ast_node.h
 * @brief AST node lifecycle and management functions
 *
 * Contains definitions and functions for working with AST nodes,
 * including creation, manipulation, and memory management.
 */

#ifndef SCOPEMUX_AST_NODE_H
#define SCOPEMUX_AST_NODE_H

#include "../../core/include/scopemux/parser.h"

/**
 * @brief Create a new AST node
 *
 * @param type Node type
 * @param name Node name
 * @return ASTNode* Created node or NULL on failure
 */
ASTNode *ast_node_new(ASTNodeType type, const char *name);

/**
 * @brief Free an AST node and all its resources
 *
 * @param node Node to free
 */
void ast_node_free(ASTNode *node);

/**
 * @brief Free an AST node's internal resources but not the node itself or its children
 * This is an internal implementation function used by other cleanup functions
 *
 * @param node Node to free internal resources for
 */
void ast_node_free_internal(ASTNode *node);

/**
 * @brief Add a child node to a parent AST node
 *
 * @param parent Parent node
 * @param child Child node
 * @return bool True on success, false on failure
 */
bool ast_node_add_child(ASTNode *parent, ASTNode *child);

/**
 * @brief Add a reference from one AST node to another
 *
 * @param from Source node
 * @param to Target node
 * @return bool True on success, false on failure
 */
bool ast_node_add_reference(ASTNode *from, ASTNode *to);

/**
 * @brief Set a property on an AST node
 *
 * @param node Node to set property on
 * @param key Property key
 * @param value Property value
 * @return bool True on success, false on failure
 */
bool ast_node_set_property(ASTNode *node, const char *key, const char *value);

/**
 * @brief Create a new AST node with full attributes
 *
 * @param type Node type
 * @param name Node name
 * @param qualified_name Fully qualified name
 * @param range Source range
 * @return ASTNode* Created node or NULL on failure
 */
ASTNode *ast_node_create(ASTNodeType type, const char *name, const char *qualified_name,
                         SourceRange range);

#endif /* SCOPEMUX_AST_NODE_H */
