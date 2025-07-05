/**
 * @file project_symbol_extraction.c
 * @brief Symbol extraction functionality for ProjectContext
 *
 * Handles extraction of symbols from parsed files into the global symbol table.
 */

#include "project_context_internal.h"
#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol.h"
#include "scopemux/symbol_collection.h"
#include "scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Forward declarations for missing headers ---
// TODO: Move these to appropriate headers when available.
static void extract_symbols_from_ast(ASTNode *node, void *symbols);
// symbol_collection_add is now implemented in symbol_collection.c
void symbol_table_register_from_ast(GlobalSymbolTable *table, ASTNode *root, const char *filepath);

/**
 * Register symbols from a parsed file into the global symbol table
 *
 * This function traverses the AST of a parsed file and registers all
 * symbol definitions in the global symbol table. It handles qualified
 * names, scopes, and other language-specific symbol registration details.
 *
 * @param project The ProjectContext
 * @param ctx The ParserContext containing the parsed AST
 * @param filepath The file path (used for symbol origin tracking)
 */
void register_file_symbols(ProjectContext *project, ParserContext *ctx, const char *filepath) {
  if (!project || !ctx || !filepath || !project->symbol_table) {
    return;
  }

  log_debug("Registering symbols from file: %s", filepath);

  // Process each AST node in the file
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    ASTNode *root = ctx->all_ast_nodes[i];
    if (!root) {
      continue;
    }

    // Register symbols from this AST
    symbol_table_register_from_ast(project->symbol_table, root, filepath);
  }

  log_debug("Completed symbol registration for file: %s", filepath);
}

/**
 * Extract symbols from a parser context and store them in a symbol collection
 *
 * This function extracts symbols from a parser context and stores them in
 * a symbol collection for further processing or analysis.
 *
 * @param project The ProjectContext
 * @param ctx The ParserContext to extract symbols from
 * @param symbols The symbol collection to store extracted symbols in
 * @return true if successful, false otherwise
 */
bool project_context_extract_symbols_impl(ProjectContext *project, ParserContext *ctx,
                                          void *symbols) {
  if (!project || !ctx || !symbols) {
    return false;
  }

  log_debug("Extracting symbols from parser context: %s",
            ctx->filename ? ctx->filename : "(unnamed)");

  // Process each AST node in the file
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    ASTNode *root = ctx->all_ast_nodes[i];
    if (!root) {
      continue;
    }

    // Extract symbols from this AST
    // This is a simplified implementation that just counts nodes by type
    extract_symbols_from_ast(root, symbols);
  }

  return true;
}

/**
 * Helper function to recursively extract symbols from an AST node
 *
 * @param node The AST node to extract symbols from
 * @param symbols The symbol collection to store extracted symbols in
 */
static void extract_symbols_from_ast(ASTNode *node, void *symbols) {
  if (!node || !symbols) {
    return;
  }

  // Process this node based on its type
  switch (node->type) {
  case NODE_FUNCTION:
  case NODE_METHOD:
    // Add function/method symbol
    if (node->name) {
      symbol_collection_add(symbols, node->name, SYMBOL_FUNCTION, node);
    }
    break;

  case NODE_CLASS:
  case NODE_STRUCT:
  case NODE_INTERFACE:
    // Add class/struct/interface symbol
    if (node->name) {
      symbol_collection_add(symbols, node->name, SYMBOL_TYPE, node);
    }
    break;

  case NODE_VARIABLE:
    // Add variable symbol
    if (node->name) {
      symbol_collection_add(symbols, node->name, SYMBOL_VARIABLE, node);
    }
    break;

  case NODE_ENUM:
    // Add enum symbol
    if (node->name) {
      symbol_collection_add(symbols, node->name, SYMBOL_ENUM, node);
    }
    break;

  case NODE_NAMESPACE:
    // Add namespace symbol
    if (node->name) {
      symbol_collection_add(symbols, node->name, SYMBOL_NAMESPACE, node);
    }
    break;

  default:
    // Other node types are not symbols
    break;
  }

  // Recursively process children
  for (size_t i = 0; i < node->num_children; i++) {
    extract_symbols_from_ast(node->children[i], symbols);
  }
}
