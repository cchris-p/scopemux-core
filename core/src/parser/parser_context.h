/**
 * @file parser_context.h
 * @brief Parser context lifecycle management functions
 *
 * Contains functions for initializing, clearing, and freeing parser contexts.
 */

#ifndef SCOPEMUX_PARSER_CONTEXT_H
#define SCOPEMUX_PARSER_CONTEXT_H

#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include "../../include/scopemux/query_manager.h"
#include "ast_node.h"
#include "cst_node.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Frees all memory associated with a ParserContext instance.
 *
 * This should be called after parsing is complete to avoid memory leaks.
 * TODO: If ParserContext contains dynamically allocated fields, free them here.
 */
void parser_free(ParserContext *ctx);

/**
 * Compatibility alias for parser_free. Use parser_free in new code.
 */
void parser_context_free(ParserContext *ctx);

/**
 * @brief Initialize a new parser context
 *
 * @return ParserContext* Initialized parser context or NULL on failure
 */
// ParserContext *parser_init(void);

/**
 * @brief Clear all resources associated with a parser context
 * This function is designed to be robust against partially freed or corrupted contexts
 *
 * @param ctx Parser context to clear
 */
void parser_clear(ParserContext *ctx);

/**
 * @brief Free a parser context and all its resources
 *
 * @param ctx Parser context to free
 */
void parser_free(ParserContext *ctx);

/**
 * @brief Set the parsing mode (AST, CST, or both)
 *
 * @param ctx Parser context
 * @param mode Parse mode to set
 */
void parser_set_mode(ParserContext *ctx, ParseMode mode);

/**
 * @brief Add an AST node to the parser context's tracking array
 * This ensures all allocated nodes are properly tracked and can be freed later
 *
 * @param ctx Parser context
 * @param node Node to add for tracking
 * @return bool True on success, false on failure
 */
bool parser_add_ast_node(ParserContext *ctx, ASTNode *node);

/**
 * @brief Set an error message and code in the parser context
 *
 * @param ctx Parser context
 * @param code Error code
 * @param message Error message
 */
void parser_set_error(ParserContext *ctx, int code, const char *message);

/**
 * @brief Get the last error message from the parser context
 *
 * @param ctx Parser context
 * @return const char* Error message or NULL if no error
 */
const char *parser_get_last_error(const ParserContext *ctx);

/**
 * @brief Set the CST root node in the parser context
 * This function properly handles ownership transfer of CST nodes
 * If a new CST root is provided, any existing CST root will be freed first
 *
 * @param ctx Parser context
 * @param cst_root CST root node (can be NULL to clear)
 */
void parser_set_cst_root(ParserContext *ctx, CSTNode *cst_root);

#endif /* SCOPEMUX_PARSER_CONTEXT_H */
