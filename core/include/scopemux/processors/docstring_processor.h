/**
 * @file docstring_processor.h
 * @brief Docstring extraction and association logic
 *
 * This header defines functions for processing and associating
 * docstrings with their corresponding AST nodes.
 */

#ifndef SCOPEMUX_DOCSTRING_PROCESSOR_H
#define SCOPEMUX_DOCSTRING_PROCESSOR_H

#include "../../scopemux/parser.h"
#include <tree_sitter/api.h>

/**
 * Structure to hold docstring information
 */
typedef struct {
  char *content;        // The cleaned docstring content
  uint32_t line_number; // Line number where docstring appears
} DocstringInfo;

/**
 * Extracts a clean docstring from a JavaDoc-style comment
 *
 * @param comment The raw comment text
 * @return A newly allocated string with the cleaned documentation
 */
char *extract_doc_comment(const char *comment);

/**
 * Associates docstrings with their target AST nodes
 *
 * @param ast_root The root AST node
 * @param docstring_nodes Array of docstring nodes
 * @param count Number of docstring nodes
 */
void associate_docstrings_with_nodes(ASTNode *ast_root, ASTNode **docstring_nodes, size_t count);

/**
 * Processes all docstrings in an AST
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 */
void process_docstrings(ASTNode *ast_root, ParserContext *ctx);

#endif /* SCOPEMUX_DOCSTRING_PROCESSOR_H */
