/**
 * @file query_processing.h
 * @brief Functions for executing and processing Tree-sitter queries
 *
 * Contains functions for executing queries against the parsed syntax trees
 * and processing the results to build AST nodes.
 */

#ifndef SCOPEMUX_QUERY_PROCESSING_H
#define SCOPEMUX_QUERY_PROCESSING_H

#include "../../core/include/scopemux/parser.h"

/**
 * @brief Execute a Tree-sitter query and process results for AST generation
 *
 * @param ctx Parser context with the parsed tree
 * @param query_name Name of the query to execute
 * @return bool True on success, false on failure
 */
bool parser_execute_query(ParserContext *ctx, const char *query_name);

/**
 * @brief Process query results and add nodes to the AST
 *
 * @param ctx Parser context
 * @param results Query results from Tree-sitter
 * @param query_name Name of the query that was executed
 * @return bool True on success, false on failure
 */
bool process_query_results(ParserContext *ctx, void *results, const char *query_name);

/**
 * @brief Detect programming language from file extension and content
 *
 * @param filename Path to the file
 * @param content File content
 * @param content_length Length of the content
 * @return Language Detected language or LANG_UNKNOWN
 */
Language parser_detect_language(const char *filename, const char *content, size_t content_length);

#endif /* SCOPEMUX_QUERY_PROCESSING_H */
