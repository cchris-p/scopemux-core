/**
 * @file ts_parser_interface.h
 * @brief Interface definition for Tree-sitter parsers
 *
 * This file defines the interface for Tree-sitter parsers in ScopeMux.
 * It follows the Dependency Inversion Principle by providing high-level
 * abstractions that concrete implementations must fulfill.
 */

#ifndef SCOPEMUX_TS_PARSER_INTERFACE_H
#define SCOPEMUX_TS_PARSER_INTERFACE_H

#include "./parser.h"
#include <stdbool.h>

// Forward declaration for Tree-sitter types
typedef struct TSNode TSNode;

/**
 * @brief Interface for Tree-sitter parser implementations
 *
 * This interface defines the contract that concrete parser implementations
 * must fulfill. It allows for different parser strategies to be used
 * interchangeably.
 */
typedef struct TSParserInterface {
  /**
   * @brief Initialize the parser for the given language
   *
   * @param ctx Parser context to initialize
   * @param language Target language type
   * @return bool True if initialization was successful
   */
  bool (*initialize)(ParserContext *ctx, Language language);

  /**
   * @brief Clean up parser resources
   *
   * @param ctx Parser context to clean up
   */
  void (*cleanup)(ParserContext *ctx);

  /**
   * @brief Parse source code to AST
   *
   * @param ctx Parser context with configuration
   * @param source Source code to parse
   * @return ASTNode* Root node of the AST or NULL on failure
   */
  ASTNode *(*parse_to_ast)(ParserContext *ctx, const char *source);

  /**
   * @brief Parse source code to CST
   *
   * @param ctx Parser context with configuration
   * @param source Source code to parse
   * @return CSTNode* Root node of the CST or NULL on failure
   */
  CSTNode *(*parse_to_cst)(ParserContext *ctx, const char *source);

  /**
   * @brief Convert a Tree-sitter node to AST
   *
   * @param root_node Root of Tree-sitter parse tree
   * @param ctx Parser context
   * @return ASTNode* Root of generated AST or NULL on failure
   */
  ASTNode *(*ts_tree_to_ast)(TSNode root_node, ParserContext *ctx);

  /**
   * @brief Convert a Tree-sitter node to CST
   *
   * @param root_node Root of Tree-sitter parse tree
   * @param ctx Parser context
   * @return CSTNode* Root of generated CST or NULL on failure
   */
  CSTNode *(*ts_tree_to_cst)(TSNode root_node, ParserContext *ctx);
} TSParserInterface;

// Global instance of the current parser interface
extern TSParserInterface *current_ts_parser;

/**
 * @brief Initialize the Tree-sitter parser interface
 *
 * This function must be called before using any Tree-sitter parsing functionality.
 *
 * @return bool True if initialization was successful
 */
bool ts_parser_interface_init(void);

/**
 * @brief Clean up the Tree-sitter parser interface
 */
void ts_parser_interface_cleanup(void);

#endif /* SCOPEMUX_TS_PARSER_INTERFACE_H */
