/**
 * @file tree_sitter_parser.c
 * @brief Implementation of Tree-sitter parser wrapper
 *
 * This file provides implementations for the TreeSitterParser structure
 * and related functions for tree-sitter integration.
 */

#include "tree_sitter_test_util.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for Tree-sitter language functions
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);

/**
 * @brief Map a language type to the corresponding Tree-sitter language
 *
 * @param language Language type to map
 * @return TSLanguage* Tree-sitter language or NULL on error
 */
static TSLanguage *get_language_for_type(Language language) {
  switch (language) {
  case LANG_C:
    return (TSLanguage *)tree_sitter_c();
  case LANG_CPP:
    return (TSLanguage *)tree_sitter_cpp();
  case LANG_PYTHON:
    return (TSLanguage *)tree_sitter_python();
  case LANG_JAVASCRIPT:
  case LANG_TYPESCRIPT:
    // JavaScript and TypeScript support removed temporarily
    return NULL;
  default:
    return NULL;
  }
}

/**
 * @brief Initialize a Tree-sitter parser for the specified language.
 *
 * @param language The language to initialize the parser for.
 * @return TreeSitterParser* Initialized parser or NULL on failure.
 */
TreeSitterParser *ts_parser_init(Language language) {
  if (language == LANG_UNKNOWN) {
    return NULL;
  }

  // Allocate memory for the parser structure
  TreeSitterParser *parser = (TreeSitterParser *)calloc(1, sizeof(TreeSitterParser));
  if (!parser) {
    return NULL;
  }

  // Initialize the Tree-sitter parser
  parser->ts_parser = ts_parser_new();
  if (!parser->ts_parser) {
    free(parser);
    return NULL;
  }

  // Get the language and set it on the parser
  parser->ts_language = get_language_for_type(language);

  // Set the language on the parser if available
  if (parser->ts_language) {
    ts_parser_set_language(parser->ts_parser, parser->ts_language);
  }

  // For now, we'll just set the language type for testing
  parser->language = language;

  return parser;
}

/**
 * @brief Free resources associated with a Tree-sitter parser.
 *
 * @param parser The parser to free.
 */
void ts_parser_free(TreeSitterParser *parser) {
  if (!parser) {
    return;
  }

  // Free the Tree-sitter parser
  if (parser->ts_parser) {
    ts_parser_delete(parser->ts_parser);
  }

  // Free the parser structure
  free(parser);
}
