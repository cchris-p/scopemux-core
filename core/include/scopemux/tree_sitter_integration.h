/**
 * @file tree_sitter_integration.h
 * @brief Tree-sitter integration for ScopeMux parser
 *
 * This module provides an interface to Tree-sitter for parsing various
 * programming languages. It handles the initialization of Tree-sitter parsers
 * and the conversion of a raw Tree-sitter tree into either a ScopeMux AST or CST.
 */

#ifndef SCOPEMUX_TREE_SITTER_INTEGRATION_H
#define SCOPEMUX_TREE_SITTER_INTEGRATION_H

#include "./parser.h"
#include <stdbool.h>

// Directly include the Tree-sitter API header
#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"

/**
 * @brief Initializes or retrieves a Tree-sitter parser for the given language.
 *
 * This function is responsible for setting the `ts_parser` field on the
 * ParserContext.
 *
 * @param ctx The parser context to initialize.
 * @param language The language to initialize the parser for.
 * @return True on success, false on failure.
 */
bool ts_init_parser(ParserContext *ctx, LanguageType language);

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Abstract Syntax Tree.
 *
 * This function will be responsible for using Tree-sitter queries to extract
 * semantic information and construct the AST.
 *
 * @param root_node The root node of the Tree-sitter syntax tree.
 * @param ctx The parser context, which contains the source code and other info.
 * @return ASTNode* The root of the generated AST, or NULL on failure.
 */
ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx);

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Concrete Syntax Tree.
 *
 * This function will be responsible for recursively walking the Tree-sitter
 * tree and creating a parallel CST.
 *
 * @param root_node The root node of the Tree-sitter syntax tree.
 * @param ctx The parser context, which contains the source code and other info.
 * @return CSTNode* The root of the generated CST, or NULL on failure.
 */
CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx);

#endif /* SCOPEMUX_TREE_SITTER_INTEGRATION_H */
