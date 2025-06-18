/**
 * @file tree_sitter_parser.h
 * @brief Tree-sitter parser definitions for ScopeMux
 *
 * This file defines the TreeSitterParser structure and related functions
 * needed for initializing and managing tree-sitter parsers.
 */

#ifndef SCOPEMUX_TREE_SITTER_PARSER_H
#define SCOPEMUX_TREE_SITTER_PARSER_H

#include "./parser.h"
#include <tree_sitter/api.h>

/**
 * @brief Structure for Tree-sitter parser.
 *
 * This structure encapsulates a Tree-sitter parser along with
 * language-specific information.
 */
typedef struct {
  LanguageType language;   // Type of language being parsed
  TSParser *ts_parser;     // Tree-sitter parser instance
  TSLanguage *ts_language; // Tree-sitter language definition

  // Additional fields can be added as needed
} TreeSitterParser;

/**
 * @brief Initialize a Tree-sitter parser for the specified language.
 *
 * @param language The language to initialize the parser for.
 * @return TreeSitterParser* Initialized parser or NULL on failure.
 */
TreeSitterParser *ts_parser_init(LanguageType language);

/**
 * @brief Free resources associated with a Tree-sitter parser.
 *
 * @param parser The parser to free.
 */
void ts_parser_free(TreeSitterParser *parser);

#endif /* SCOPEMUX_TREE_SITTER_PARSER_H */
