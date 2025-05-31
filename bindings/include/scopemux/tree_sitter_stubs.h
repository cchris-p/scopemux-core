/**
 * @file tree_sitter_stubs.h
 * @brief Stub declarations for Tree-sitter functions
 *
 * This file contains declarations for Tree-sitter functions that are
 * needed by the ScopeMux parser but not available in the Tree-sitter library.
 */

#ifndef SCOPEMUX_TREE_SITTER_STUBS_H
#define SCOPEMUX_TREE_SITTER_STUBS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Tree-sitter type definitions
 */
typedef struct TSTree {
  char _dummy;
} TSTree;

typedef struct TSParser {
  char _dummy;
} TSParser;

typedef struct TSLanguage {
  char _dummy;
} TSLanguage;

/**
 * @brief Tree-sitter node structure
 */
typedef struct {
  const void *tree;
  const void *id;
  uint32_t context[4];
} TSNode;

/**
 * @brief Free a tree
 */
void ts_tree_free(void *tree);

/**
 * @brief Create a new parser
 */
TSParser *ts_parser_new(void);

/**
 * @brief Delete a parser
 */
void ts_parser_delete(TSParser *parser);

/**
 * @brief Set the language for a parser
 */
bool ts_parser_set_language(TSParser *parser, const TSLanguage *language);

/**
 * @brief Parse a string
 */
void *ts_parser_parse_string(TSParser *parser, const char *string, uint32_t length);

/**
 * @brief Get the root node of a tree
 */
TSNode ts_tree_root_node(const void *tree);

/**
 * @brief Get the number of children of a node
 */
uint32_t ts_node_child_count(TSNode node);

/**
 * @brief Get a child of a node
 */
TSNode ts_node_child(TSNode node, uint32_t index);

/**
 * @brief Get a string representation of a node
 */
char *ts_node_string(TSNode node);

/**
 * @brief Get the type of a node
 */
const char *ts_node_type(TSNode node);

/**
 * @brief Get the C language
 */
const TSLanguage *tree_sitter_c(void);

/**
 * @brief Get the C++ language
 */
const TSLanguage *tree_sitter_cpp(void);

/**
 * @brief Get the Python language
 */
const TSLanguage *tree_sitter_python(void);

#endif /* SCOPEMUX_TREE_SITTER_STUBS_H */
