/**
 * @file symbol_management.h
 * @brief Symbol registration and reference resolution functionality for ProjectContext
 *
 * Handles the registration of symbols from parsed files into the global symbol table,
 * and the resolution of references between symbols across different files.
 */

#ifndef SCOPMUX_SYMBOL_MANAGEMENT_H
#define SCOPMUX_SYMBOL_MANAGEMENT_H

#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Implementation for extracting symbols from a parser context
 *
 * @param project The project context
 * @param parser The parser context containing AST nodes
 * @param symbol_table The global symbol table to populate
 * @return true if symbols were extracted successfully, false otherwise
 */
bool project_extract_symbols_impl(ProjectContext *project, ParserContext *parser,
                                  GlobalSymbolTable *symbol_table);

/**
 * @brief Implementation for resolving references across all files in a project
 *
 * @param project The project context
 * @return true if references were resolved successfully, false otherwise
 */
bool project_resolve_references_impl(ProjectContext *project);

/**
 * @brief Implementation for retrieving a symbol by its qualified name
 *
 * @param project The project context
 * @param qualified_name Fully qualified name of the symbol
 * @return Matching node or NULL if not found
 */
const ASTNode *project_get_symbol_impl(const ProjectContext *project, const char *qualified_name);

/**
 * @brief Implementation for retrieving all symbols of a specific type
 *
 * @param project The project context
 * @param type Node type to filter by
 * @param out_nodes Output array of nodes
 * @param max_nodes Maximum number of nodes to return
 * @return Number of nodes found
 */
size_t project_get_symbols_by_type_impl(const ProjectContext *project, ASTNodeType type,
                                        const ASTNode **out_nodes, size_t max_nodes);

/**
 * @brief Implementation for finding all references to a symbol across the project
 *
 * @param project The project context
 * @param node Symbol to find references to
 * @param out_references Output array of referencing nodes
 * @param max_references Maximum number of references to return
 * @return Number of references found
 */
size_t project_find_references_impl(const ProjectContext *project, const ASTNode *node,
                                    const ASTNode **out_references, size_t max_references);

#ifdef __cplusplus
}
#endif

#endif // SCOPMUX_SYMBOL_MANAGEMENT_H