/**
 * @file tree_sitter_stubs.c
 * @brief Stub implementations for Tree-sitter functions
 * 
 * This file contains stub implementations for Tree-sitter functions that are
 * needed by the ScopeMux parser but not available in the Tree-sitter library.
 */

#include <stdlib.h>
#include <string.h>
#include "../../include/scopemux/tree_sitter_stubs.h"

/**
 * @brief Stub implementation for ts_tree_free
 */
void ts_tree_free(void* tree) {
    // Stub implementation
    if (tree) {
        free(tree);
    }
}

/**
 * @brief Stub implementation for ts_parser_new
 */
TSParser* ts_parser_new(void) {
    // Stub implementation
    return (TSParser*)calloc(1, sizeof(TSParser));
}

/**
 * @brief Stub implementation for ts_parser_delete
 */
void ts_parser_delete(TSParser* parser) {
    // Stub implementation
    if (parser) {
        free(parser);
    }
}

/**
 * @brief Stub implementation for ts_parser_set_language
 */
bool ts_parser_set_language(TSParser* parser, const TSLanguage* language) {
    // Stub implementation
    if (!parser || !language) {
        return false;
    }
    return true;
}

/**
 * @brief Stub implementation for ts_parser_parse_string
 */
void* ts_parser_parse_string(TSParser* parser, const char* string, uint32_t length) {
    // Stub implementation
    if (!parser || !string) {
        return NULL;
    }
    return calloc(1, 128); // Allocate some memory for the tree
}

/**
 * @brief Stub implementation for ts_tree_root_node
 */
TSNode ts_tree_root_node(const void* tree) {
    // Stub implementation
    TSNode node = {0};
    return node;
}

/**
 * @brief Stub implementation for ts_node_child_count
 */
uint32_t ts_node_child_count(TSNode node) {
    // Stub implementation
    return 0;
}

/**
 * @brief Stub implementation for ts_node_child
 */
TSNode ts_node_child(TSNode node, uint32_t index) {
    // Stub implementation
    TSNode child = {0};
    return child;
}

/**
 * @brief Stub implementation for ts_node_string
 */
char* ts_node_string(TSNode node) {
    // Stub implementation
    return strdup("node");
}

/**
 * @brief Stub implementation for ts_node_type
 */
const char* ts_node_type(TSNode node) {
    // Stub implementation
    return "unknown";
}

/**
 * @brief Stub implementation for tree_sitter_c
 */
const TSLanguage* tree_sitter_c(void) {
    // Stub implementation
    static TSLanguage c_language = {0};
    return &c_language;
}

/**
 * @brief Stub implementation for tree_sitter_cpp
 */
const TSLanguage* tree_sitter_cpp(void) {
    // Stub implementation
    static TSLanguage cpp_language = {0};
    return &cpp_language;
}

/**
 * @brief Stub implementation for tree_sitter_python
 */
const TSLanguage* tree_sitter_python(void) {
    // Stub implementation
    static TSLanguage python_language = {0};
    return &python_language;
}
