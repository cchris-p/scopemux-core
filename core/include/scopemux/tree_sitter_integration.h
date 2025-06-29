/**
 * @file tree_sitter_integration.h
 * @brief Tree-sitter integration for ScopeMux parser
 *
 * This module provides an interface to Tree-sitter for parsing various
 * programming languages. It handles the initialization of Tree-sitter parsers
 * and the conversion of a raw Tree-sitter tree into either a ScopeMux AST or CST.
 *
 * ScopeMux supports two parsing modes:
 * - CSTs (Concrete Syntax Trees): Full-fidelity syntax trees that include every token,
 *   punctuation, and language-specific structure. These are directly derived from Tree-sitter.
 * - ASTs (Abstract Syntax Trees): Semantic, language-agnostic trees using custom ASTNode
 *   structures derived from Tree-sitter query matches. These provide a standardized
 *   representation across different programming languages.
 *
 * The AST structure follows a standardized node type system defined in parser.h,
 * with common node types like NODE_ROOT, NODE_FUNCTION, NODE_CLASS, etc. that
 * are used consistently across all supported languages.
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
bool ts_init_parser(ParserContext *ctx, Language language);

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Abstract Syntax Tree.
 *
 * This function uses Tree-sitter queries (.scm files) to extract semantic information
 * and construct a standardized AST. The resulting AST follows a common structure
 * across languages with standardized node types like:
 *   - NODE_ROOT: Root of the AST (typically represents a file)
 *   - NODE_FUNCTION: Function definitions
 *   - NODE_CLASS: Class/struct definitions
 *   - NODE_METHOD: Class methods
 *   - NODE_VARIABLE: Variable declarations
 *   - NODE_IMPORT/NODE_INCLUDE: Import/include statements
 *   - NODE_DOCSTRING: Documentation comments
 *
 * Each language implementation maps its specific constructs to these standard
 * node types while preserving language-specific details in node attributes.
 *
 * @param root_node The root node of the Tree-sitter syntax tree.
 * @param ctx The parser context, which contains the source code and other info.
 * @return ASTNode* The root of the generated AST, or NULL on failure.
 */
ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx);

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Concrete Syntax Tree.
 *
 * Unlike AST generation which uses queries to extract semantic information,
 * this function recursively walks the Tree-sitter tree and creates a parallel CST
 * that preserves the full syntax structure including every token and punctuation.
 * CSTs are language-specific and maintain the exact structure of the source code.
 *
 * The CST is useful for:
 * - Precise source range tracking
 * - Code formatting and low-level transformations
 * - Exact reconstruction of the original source code
 *
 * @param root_node The root node of the Tree-sitter syntax tree.
 * @param ctx The parser context, which contains the source code and other info.
 * @return CSTNode* The root of the generated CST, or NULL on failure.
 */
CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx);

#endif /* SCOPEMUX_TREE_SITTER_INTEGRATION_H */
