/**
 * @file tree_sitter_integration.c
 * @brief Implementation of the Tree-sitter integration for ScopeMux
 *
 * This file serves as a facade for the Tree-sitter integration, delegating to
 * specialized modules while maintaining the public interface.
 *
 * The AST generation process follows these key steps:
 * 1. Create a root NODE_ROOT node representing the file/module
 * 2. Process Tree-sitter queries in a hierarchical order
 * 3. Map language-specific Tree-sitter nodes to standard AST node types
 * 4. Generate qualified names for AST nodes based on their hierarchy
 * 5. Apply post-processing and language-specific adaptations
 *
 * The standard node types provide a common structure across all supported
 * languages while preserving language-specific details in node attributes.
 * This enables consistent analysis and transformation tools to work across
 * multiple languages.
 */

// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L


#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include "scopemux/logging.h"
#include "scopemux/ast.h"
#include "scopemux/parser.h"
#include "scopemux/ts_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Implements the public interface for Tree-sitter integration.
 *
 * This file now serves as a thin facade that delegates to specialized modules
 * for different aspects of Tree-sitter integration:
 *
 * - ts_init.c: Handles parser initialization and language setup
 * - ts_ast_builder.c: Handles AST generation from Tree-sitter trees
 * - ts_cst_builder.c: Handles CST generation from Tree-sitter trees
 * - ts_query_processor.c: Handles Tree-sitter query execution
 *
 * This architecture follows the Single Responsibility Principle by separating
 * different concerns into focused modules, while maintaining backward compatibility
 * with the existing public interface.
 */

// Forward declarations for Tree-sitter language functions
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

/**
 * @brief Builds a queries directory path for the given language
 *
 * This function is implemented in ts_init.c
 */
extern char *build_queries_dir_impl(LanguageType language);

/**
 * @brief Initialize the Tree-sitter parser for a specific language.
 *
 * @param ctx Parser context
 * @param language Language type
 * @return true Parser initialized successfully
 * @return false Parser failed to initialize
 */
bool ts_init_parser(ParserContext *ctx, LanguageType language) {
  if (!ctx) {
    log_error("NULL context passed to ts_init_parser");
    return false;
  }

  // Implementation delegated to ts_init.c
  // This facade maintains the public interface
  extern bool ts_init_parser_impl(ParserContext * ctx, LanguageType language);
  log_debug("ts_init_parser facade called, delegating to ts_init_parser_impl for language %d",
            language);
  return ts_init_parser_impl(ctx, language);
}

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Abstract Syntax Tree.
 *
 * This facade implementation delegates to the specialized AST builder module.
 * It provides the public interface while the actual implementation is in
 * ts_ast_builder.c.
 *
 * @param root_node The root node of the Tree-sitter syntax tree.
 * @param ctx The parser context, which contains the source code and other info.
 * @return ASTNode* The root of the generated AST, or NULL on failure.
 */
ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx) {
    if (ctx) {
      parser_set_error(ctx, -1, "Invalid arguments to ts_tree_to_ast");
    }
    return NULL;
  }

  // Implementation delegated to ts_ast_builder.c
  // This facade maintains the public interface
  extern ASTNode *ts_tree_to_ast_impl(TSNode root_node, ParserContext * ctx);
  return ts_tree_to_ast_impl(root_node, ctx);
}

/**
 * @brief Converts a raw Tree-sitter tree into a ScopeMux Concrete Syntax Tree.
 *
 * This facade implementation delegates to the specialized CST builder module.
 * It provides the public interface while the actual implementation is in
 * ts_cst_builder.c.
 *
 * @param root_node The root node of the Tree-sitter syntax tree.
 * @param ctx The parser context, which contains the source code and other info.
 * @return CSTNode* The root of the generated CST, or NULL on failure.
 */
CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx || !ctx->source_code) {
    parser_set_error(ctx, -1, "Invalid context for CST generation");
    return NULL;
  }

  // Implementation delegated to ts_cst_builder.c
  // This facade maintains the public interface
  extern CSTNode *ts_tree_to_cst_impl(TSNode root_node, ParserContext * ctx);
  return ts_tree_to_cst_impl(root_node, ctx);
}
