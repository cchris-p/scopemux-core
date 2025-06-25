/**
 * @file ast_post_processor.h
 * @brief AST post-processing logic
 *
 * This header defines functions for post-processing AST nodes after
 * initial construction. This includes operations like node ordering,
 * relationship building, and cleanup of temporary nodes.
 */

#ifndef SCOPEMUX_AST_POST_PROCESSOR_H
#define SCOPEMUX_AST_POST_PROCESSOR_H

#include "../../scopemux/parser.h"

/**
 * Post-processes the AST tree to ensure consistent ordering and structure
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 * @return The post-processed AST root
 */
ASTNode *post_process_ast(ASTNode *ast_root, ParserContext *ctx);

/**
 * Orders nodes in the AST based on priority
 * Docstrings -> Includes -> Functions -> Other nodes
 *
 * @param ast_root The root AST node
 */
void order_ast_nodes(ASTNode *ast_root);

/**
 * Removes temporary and unwanted nodes from the AST
 *
 * @param ast_root The root AST node
 * @return The number of remaining nodes after cleanup
 */
size_t cleanup_ast_nodes(ASTNode *ast_root);

#endif /* SCOPEMUX_AST_POST_PROCESSOR_H */
