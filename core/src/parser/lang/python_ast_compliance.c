/**
 * @file python_ast_compliance.c
 * @brief Python-specific schema compliance implementation
 *
 * This module implements Python-specific schema compliance and post-processing
 * for the AST builder.
 */

#include "scopemux/ast.h"
#include "scopemux/ast_compliance.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Python-specific schema compliance function
 *
 * This function applies Python-specific schema compliance rules to the AST nodes.
 *
 * @param node The AST node to process
 * @param ctx Parser context
 * @return int Status code (0 for success)
 */
int python_ensure_schema_compliance(ASTNode *node, ParserContext *ctx) {
  if (!node) {
    return -1;
  }

  // Python-specific schema compliance rules
  if (node->name) {
    // Handle Python-specific node types
    if (strcmp(node->name, "module") == 0) {
      // Convert module nodes to a standard type
      node->type = NODE_ROOT;
      free(node->name);
      node->name = strdup("ROOT");
      free(node->qualified_name);
      node->qualified_name = strdup("ROOT");
    } else if (strcmp(node->name, "function_definition") == 0) {
      // Ensure function_definition nodes are properly typed
      node->type = NODE_FUNCTION;
    } else if (strcmp(node->name, "class_definition") == 0) {
      // Ensure class_definition nodes are properly typed
      node->type = NODE_CLASS;
    }
    // Additional Python-specific rules can be added here
  }

  return 0;
}

/**
 * @brief Python-specific AST post-processing
 *
 * This function applies Python-specific post-processing to the entire AST.
 *
 * @param root_node The root AST node
 * @param ctx Parser context
 * @return ASTNode* The processed AST
 */
ASTNode *python_ast_post_process(ASTNode *root_node, ParserContext *ctx) {
  if (!root_node) {
    return NULL;
  }

  log_debug("Applying Python-specific AST post-processing");

  // Add Python-specific post-processing logic here
  // For example, handling imports, resolving qualified names, etc.

  return root_node;
}

/**
 * @brief Register Python language-specific callbacks
 *
 * This function registers the Python-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_python_ast_compliance(void) {
  register_schema_compliance_callback(LANG_PYTHON, python_ensure_schema_compliance);
  register_ast_post_process_callback(LANG_PYTHON, python_ast_post_process);
  log_debug("Registered Python AST compliance callbacks");
}
