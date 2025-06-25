#pragma once

#include "../parser.h"
#include <stdint.h>

/**
 * @file docstring_processor.h
 * @brief Interface for docstring extraction and association logic.
 *
 * Provides functions for extracting docstrings from comments and associating
 * them with AST nodes. Used to separate docstring handling from the main parser logic.
 */

// Struct to hold docstring content and its line number
typedef struct {
  char *content;
  uint32_t line_number;
} DocstringInfo;

// Extracts a clean docstring from a JavaDoc-style comment
char *extract_doc_comment(const char *comment);

// Associates docstrings with their target AST nodes
void associate_docstrings_with_nodes(ASTNode *ast_root, ASTNode **docstring_nodes, size_t count);

// Processes all docstrings in an AST
void process_docstrings(ASTNode *ast_root, ParserContext *ctx);
