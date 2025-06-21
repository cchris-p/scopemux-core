/**
 * @file test_processor.h
 * @brief Test-specific processing logic
 * 
 * This header defines functions for test-specific AST processing,
 * extracting this logic from production code. This includes functions
 * to detect test environments and apply special transformations needed
 * only in test scenarios.
 */

#ifndef SCOPEMUX_TEST_PROCESSOR_H
#define SCOPEMUX_TEST_PROCESSOR_H

#include "../../scopemux/parser.h"

/**
 * Determines if the current context represents a hello world test
 * 
 * @param ctx The parser context
 * @return true if running in the hello world test environment
 */
bool is_hello_world_test(ParserContext *ctx);

/**
 * Check if the current AST and context represent a variables_loops_conditions.c test
 *
 * @param ctx The parser context
 * @return true if in variables_loops_conditions.c test environment
 */
bool is_variables_loops_conditions_test(ParserContext *ctx);

/**
 * Applies test-specific transformations to an AST
 * 
 * @param ast_root The root AST node
 * @param ctx The parser context
 * @return The modified AST
 */
ASTNode *apply_test_adaptations(ASTNode *ast_root, ParserContext *ctx);

/**
 * Adapt the AST for hello_world.c test file.
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 * @return The modified AST root
 */
ASTNode *adapt_hello_world_test(ASTNode *ast_root, ParserContext *ctx);

/**
 * Adapt the AST for variables_loops_conditions.c test file.
 *
 * @param ast_root The root AST node
 * @param ctx The parser context
 * @return The modified AST root
 */
ASTNode *adapt_variables_loops_conditions_test(ASTNode *ast_root, ParserContext *ctx);

#endif /* SCOPEMUX_TEST_PROCESSOR_H */
