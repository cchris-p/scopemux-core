/**
 * @file tree_sitter_integration.h
 * @brief Tree-sitter integration for ScopeMux parser
 * 
 * This module provides an interface to Tree-sitter for parsing various
 * programming languages. It handles the initialization of Tree-sitter parsers,
 * traversal of the syntax tree, and conversion to ScopeMux IR.
 */

#ifndef SCOPEMUX_TREE_SITTER_INTEGRATION_H
#define SCOPEMUX_TREE_SITTER_INTEGRATION_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./parser.h"

/**
 * @brief Tree-sitter parser wrapper
 * 
 * This structure wraps the Tree-sitter parser and provides
 * language-specific functionality.
 */
typedef struct TreeSitterParser {
    void* ts_parser;       // Tree-sitter parser instance
    void* ts_language;     // Tree-sitter language definition
    LanguageType language; // ScopeMux language type
    char* language_name;   // Name of the language
    
    // Error handling
    char* last_error;      // Last error message
    int error_code;        // Error code
} TreeSitterParser;

/**
 * @brief Initialize a Tree-sitter parser for a specific language
 * 
 * @param language Language to initialize
 * @return TreeSitterParser* Initialized parser or NULL on failure
 * 
 * @note The returned parser must be freed with ts_parser_free()
 */
TreeSitterParser* ts_parser_init(LanguageType language);

/**
 * @brief Free a Tree-sitter parser
 * 
 * @param parser Parser to free
 */
void ts_parser_free(TreeSitterParser* parser);

/**
 * @brief Parse a string using Tree-sitter
 * 
 * @param parser Tree-sitter parser
 * @param content Source code content
 * @param content_length Length of the content
 * @return void* Tree-sitter tree or NULL on failure
 * 
 * @note The returned tree must be freed with ts_tree_free()
 */
void* ts_parser_parse_string(TreeSitterParser* parser, 
                             const char* content, 
                             size_t content_length);

/**
 * @brief Free a Tree-sitter syntax tree
 * 
 * @param tree Tree to free
 */
void ts_tree_free(void* tree);

/**
 * @brief Get the last error message from a Tree-sitter parser
 * 
 * @param parser Tree-sitter parser
 * @return const char* Error message or NULL if no error
 */
const char* ts_parser_get_last_error(const TreeSitterParser* parser);

/**
 * @brief Convert a Tree-sitter syntax tree to ScopeMux IR
 * 
 * @param parser Tree-sitter parser
 * @param tree Tree-sitter syntax tree
 * @param parser_ctx ScopeMux parser context to populate
 * @return bool True on success, false on failure
 */
bool ts_tree_to_ir(TreeSitterParser* parser, 
                   void* tree, 
                   ParserContext* parser_ctx);

/**
 * @brief Extract comments and docstrings from a Tree-sitter syntax tree
 * 
 * @param parser Tree-sitter parser
 * @param tree Tree-sitter syntax tree
 * @param parser_ctx ScopeMux parser context to populate
 * @return size_t Number of comments/docstrings extracted
 */
size_t ts_extract_comments(TreeSitterParser* parser, 
                           void* tree, 
                           ParserContext* parser_ctx);

/**
 * @brief Extract functions and methods from a Tree-sitter syntax tree
 * 
 * @param parser Tree-sitter parser
 * @param tree Tree-sitter syntax tree
 * @param parser_ctx ScopeMux parser context to populate
 * @return size_t Number of functions/methods extracted
 */
size_t ts_extract_functions(TreeSitterParser* parser, 
                            void* tree, 
                            ParserContext* parser_ctx);

/**
 * @brief Extract classes and other type definitions from a Tree-sitter syntax tree
 * 
 * @param parser Tree-sitter parser
 * @param tree Tree-sitter syntax tree
 * @param parser_ctx ScopeMux parser context to populate
 * @return size_t Number of classes/types extracted
 */
size_t ts_extract_classes(TreeSitterParser* parser, 
                          void* tree, 
                          ParserContext* parser_ctx);

/**
 * @brief Get the source range for a Tree-sitter node
 * 
 * @param tree_node Tree-sitter node
 * @return SourceRange Source range
 */
SourceRange ts_get_node_range(void* tree_node);

/**
 * @brief Get the text content for a Tree-sitter node
 * 
 * @param tree_node Tree-sitter node
 * @param source_code Source code buffer
 * @param source_code_length Length of source code buffer
 * @return char* Allocated string with node content (must be freed by caller)
 */
char* ts_get_node_text(void* tree_node, 
                        const char* source_code, 
                        size_t source_code_length);

/**
 * @brief Check if a node is a function or method
 * 
 * @param parser Tree-sitter parser
 * @param tree_node Tree-sitter node
 * @return bool True if the node is a function or method
 */
bool ts_is_function(TreeSitterParser* parser, void* tree_node);

/**
 * @brief Check if a node is a class or type definition
 * 
 * @param parser Tree-sitter parser
 * @param tree_node Tree-sitter node
 * @return bool True if the node is a class or type definition
 */
bool ts_is_class(TreeSitterParser* parser, void* tree_node);

/**
 * @brief Check if a node is a comment or docstring
 * 
 * @param parser Tree-sitter parser
 * @param tree_node Tree-sitter node
 * @return bool True if the node is a comment or docstring
 */
bool ts_is_comment(TreeSitterParser* parser, void* tree_node);

/**
 * @brief Extract function/method signature from a Tree-sitter node
 * 
 * @param parser Tree-sitter parser
 * @param tree_node Tree-sitter node
 * @param source_code Source code buffer
 * @param source_code_length Length of source code buffer
 * @return char* Allocated string with signature (must be freed by caller)
 */
char* ts_extract_function_signature(TreeSitterParser* parser, 
                                    void* tree_node, 
                                    const char* source_code, 
                                    size_t source_code_length);

/**
 * @brief Extract class/type name from a Tree-sitter node
 * 
 * @param parser Tree-sitter parser
 * @param tree_node Tree-sitter node
 * @param source_code Source code buffer
 * @param source_code_length Length of source code buffer
 * @return char* Allocated string with class name (must be freed by caller)
 */
char* ts_extract_class_name(TreeSitterParser* parser, 
                            void* tree_node, 
                            const char* source_code, 
                            size_t source_code_length);

#endif /* SCOPEMUX_TREE_SITTER_INTEGRATION_H */
