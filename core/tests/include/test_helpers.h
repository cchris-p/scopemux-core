#ifndef SCOPEMUX_TEST_HELPERS_H
#define SCOPEMUX_TEST_HELPERS_H

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../core/include/scopemux/parser.h"


/**
 * Reads test file content from the examples directory
 *
 * @param language Language subdirectory name
 * @param category Category subdirectory name
 * @param file_name Name of the test file
 * @return Dynamically allocated string with file content, must be freed by caller
 */
char *read_test_file(const char *language, const char *category, const char *file_name);

/**
 * Finds a node with the specified name and type in the AST
 *
 * @param parent Parent node to search within
 * @param name Name to search for
 * @param type Node type to search for
 * @return Pointer to the found node, or NULL if not found
 */
ASTNode *find_node_by_name(ASTNode *parent, const char *name, ASTNodeType type);

/**
 * Verifies that a node has expected fields populated
 *
 * @param node Node to validate
 * @param node_name Expected name for validation messages
 */
void assert_node_fields(ASTNode *node, const char *node_name);

/**
 * Counts nodes of a specific type in the AST
 *
 * @param root Root node to start counting from
 * @param type Type of node to count
 * @return Number of nodes of the specified type
 */
int count_nodes_by_type(ASTNode *root, ASTNodeType type);

/**
 * Dumps AST structure for debugging purposes
 *
 * @param node Root node to dump
 * @param level Indentation level (start with 0)
 */
void dump_ast_structure(ASTNode *node, int level);

/**
 * Parse C++ source code into an AST
 *
 * @param ctx Parser context to use
 * @param source Source code to parse
 * @return Root AST node or NULL on failure
 */
ASTNode *parse_cpp_ast(ParserContext *ctx, const char *source);

#endif /* SCOPEMUX_TEST_HELPERS_H */
