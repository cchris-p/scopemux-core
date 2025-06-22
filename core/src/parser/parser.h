/**
 * @file parser.h
 * @brief Main parser module implementation header
 *
 * Contains the central parsing logic implementation that coordinates
 * all parser components.
 */

#ifndef SCOPEMUX_PARSER_IMPL_H
#define SCOPEMUX_PARSER_IMPL_H

#include "../../include/scopemux/parser.h"
#include "ast_node.h"
#include "cst_node.h"
#include "memory_tracking.h"
#include "parser_context.h"
#include "query_processing.h"

/**
 * @brief Parse a file and generate the AST and/or CST
 *
 * @param ctx Parser context
 * @param filename Path to the file
 * @param language Optional language hint (LANG_UNKNOWN to auto-detect)
 * @return bool True on success, false on failure
 */
bool parser_parse_file(ParserContext *ctx, const char *filename, LanguageType language);

/**
 * @brief Parse a string and generate the AST and/or CST
 *
 * @param ctx Parser context
 * @param content String content to parse
 * @param content_length Length of content
 * @param filename Virtual filename for error reporting
 * @param language Optional language hint (LANG_UNKNOWN to auto-detect)
 * @return bool True on success, false on failure
 */
bool parser_parse_string(ParserContext *ctx, const char *content, size_t content_length,
                         const char *filename, LanguageType language);

/**
 * @brief Get the AST node for a specific entity
 *
 * @param ctx Parser context
 * @param qualified_name Fully qualified name of the entity
 * @return const ASTNode* Found node or NULL if not found
 */
const ASTNode *parser_get_ast_node(const ParserContext *ctx, const char *qualified_name);

/**
 * @brief Get all AST nodes of a specific type
 *
 * @param ctx Parser context
 * @param type Node type to filter by
 * @param out_nodes Output array of nodes (can be NULL to just get the count)
 * @param max_nodes Maximum number of nodes to return
 * @return size_t Number of nodes found
 */
size_t parser_get_ast_nodes_by_type(const ParserContext *ctx, ASTNodeType type,
                                    const ASTNode **out_nodes, size_t max_nodes);

/**
 * @brief Get the root node of the Abstract Syntax Tree (AST)
 *
 * @param ctx Parser context
 * @return const ASTNode* Root of the AST or NULL if not available
 */
const ASTNode *parser_get_ast_root(const ParserContext *ctx);

/**
 * @brief Get the root node of the Concrete Syntax Tree (CST)
 *
 * @param ctx Parser context
 * @return const CSTNode* Root of the CST or NULL if not available
 */
const CSTNode *parser_get_cst_root(const ParserContext *ctx);

#endif /* SCOPEMUX_PARSER_IMPL_H */
