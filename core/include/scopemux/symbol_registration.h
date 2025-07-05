/**
 * @file symbol_registration.h
 * @brief Symbol registration functionality for ProjectContext
 *
 * Handles registration of symbols from parsed files into the global symbol table.
 */

#pragma once

#include "../src/parser/parser_context.h"
#include "project_context.h"

/**
 * @brief Register symbols from a file into the project context
 *
 * @param project The project context
 * @param ctx The parser context containing AST nodes
 * @param filepath Path to the source file
 */
void register_file_symbols(ProjectContext *project, ParserContext *ctx, const char *filepath);

/**
 * @brief Implementation for extracting symbols from a parser context
 *
 * @param project The project context
 * @param ctx The parser context containing AST nodes
 * @param symbols The symbol table to populate
 * @return true if symbols were extracted successfully, false otherwise
 */
bool project_context_extract_symbols_impl(ProjectContext *project, ParserContext *ctx,
                                          void *symbols);

/**
 * @brief Extract symbols from an AST node and its children
 *
 * @param node The AST node to extract symbols from
 * @param symbols The symbol table to populate
 */
static void extract_symbols_from_ast(ASTNode *node, void *symbols);
