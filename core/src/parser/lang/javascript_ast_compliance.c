/**
 * @file javascript_ast_compliance.c
 * @brief JavaScript-specific schema compliance implementation
 *
 * This module implements JavaScript-specific schema compliance and post-processing
 * for the AST builder.
 */

#include "scopemux/ast.h"
#include "scopemux/ast_compliance.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief JavaScript-specific schema compliance function
 *
 * This function applies JavaScript-specific schema compliance rules to AST nodes.
 *
 * @param node The AST node to process
 * @param ctx Parser context
 * @return int Status code (0 for success)
 */
int javascript_ensure_schema_compliance(ASTNode *node, ParserContext *ctx) {
  if (!node) {
    return -1;
  }

  // JavaScript-specific schema compliance rules
  if (node->name) {
    // Handle JavaScript-specific node types
    if (strcmp(node->name, "program") == 0) {
      // Convert program nodes to a standard type
      node->type = NODE_ROOT;
      free(node->name);
      node->name = strdup("ROOT");
      free(node->qualified_name);
      node->qualified_name = strdup("ROOT");
    } else if (strcmp(node->name, "function_declaration") == 0 ||
               strcmp(node->name, "function") == 0 || strcmp(node->name, "arrow_function") == 0) {
      // Ensure function nodes are properly typed
      node->type = NODE_FUNCTION;
    } else if (strcmp(node->name, "class_declaration") == 0 || strcmp(node->name, "class") == 0) {
      // Ensure class nodes are properly typed
      node->type = NODE_CLASS;
    } else if (strcmp(node->name, "method_definition") == 0) {
      // Ensure method nodes are properly typed
      node->type = NODE_METHOD;
    }
    // Additional JavaScript-specific rules can be added here
  }

  return 0;
}

/**
 * @brief JavaScript-specific AST post-processing
 *
 * This function applies JavaScript-specific post-processing to the entire AST.
 *
 * @param root_node The root AST node
 * @param ctx Parser context
 * @return ASTNode* The processed AST
 */
ASTNode *javascript_ast_post_process(ASTNode *root_node, ParserContext *ctx) {
  if (!root_node) {
    return NULL;
  }

  log_debug("Applying JavaScript-specific AST post-processing");

  // Add JavaScript-specific post-processing logic here
  // For example, handling imports/exports, resolving module dependencies, etc.

  return root_node;
}

/**
 * @brief Register JavaScript language-specific callbacks
 *
 * This function registers the JavaScript-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_javascript_ast_compliance(void) {
  register_schema_compliance_callback(LANG_JAVASCRIPT, javascript_ensure_schema_compliance);
  register_ast_post_process_callback(LANG_JAVASCRIPT, javascript_ast_post_process);
  log_debug("Registered JavaScript AST compliance callbacks");
}
