/**
 * @file symbol_collection.h
 * @brief Symbol collection functionality for ProjectContext
 *
 * Declares functions for adding symbols to a collection during project analysis.
 */

#ifndef SCOPEMUX_SYMBOL_COLLECTION_H
#define SCOPEMUX_SYMBOL_COLLECTION_H

#include "ast.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a symbol to a symbol collection
 *
 * This function adds a symbol with the given name, type, and associated AST node
 * to the provided symbol collection.
 *
 * @param symbols The symbol collection to add to
 * @param name The name of the symbol
 * @param symbol_type The type of the symbol (SYMBOL_FUNCTION, SYMBOL_TYPE, etc.)
 * @param node The AST node associated with this symbol
 */
void symbol_collection_add(void *symbols, const char *name, int symbol_type, ASTNode *node);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_SYMBOL_COLLECTION_H */
