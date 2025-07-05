/**
 * @file typescript_ast_compliance.c
 * @brief TypeScript-specific schema compliance implementation
 *
 * This module implements TypeScript-specific schema compliance and post-processing
 * for the AST builder.
 */

#include "scopemux/ast.h"
#include "scopemux/ast_compliance.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief TypeScript-specific schema compliance function
 *
 * This function applies TypeScript-specific schema compliance rules to AST nodes.
 *
 * @param node The AST node to process
 * @param ctx Parser context
 * @return int Status code (0 for success)
 */
int typescript_ensure_schema_compliance(ASTNode *node, ParserContext *ctx) {
  if (!node) {
    return -1;
  }

  // TypeScript-specific schema compliance rules
  if (node->name) {
    // Handle TypeScript-specific node types
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
    } else if (strcmp(node->name, "interface_declaration") == 0) {
      // Handle TypeScript interfaces
      node->type = NODE_INTERFACE;
    } else if (strcmp(node->name, "type_alias_declaration") == 0) {
      // Handle TypeScript type aliases
      /* NODE_TYPE_ALIAS does not exist in the current ASTNodeType enum. If alias nodes are needed,
       * define a new type in ast.h. For now, this assignment is removed. */
    }
    // Additional TypeScript-specific rules can be added here
  }

  return 0;
}

/**
 * @brief TypeScript-specific AST post-processing
 *
 * This function applies TypeScript-specific post-processing to the entire AST.
 *
 * @param root_node The root AST node
 * @param ctx Parser context
 * @return ASTNode* The processed AST
 */
ASTNode *typescript_ast_post_process(ASTNode *root_node, ParserContext *ctx) {
  if (!root_node) {
    return NULL;
  }

  log_debug("Applying TypeScript-specific AST post-processing");

  // Add TypeScript-specific post-processing logic here
  // For example, handling type annotations, interfaces, generics, etc.

  return root_node;
}

/**
 * @brief Register TypeScript language-specific callbacks
 *
 * This function registers the TypeScript-specific schema compliance and
 * post-processing callbacks with the AST compliance system.
 */
void register_typescript_ast_compliance(void) {
  register_schema_compliance_callback(LANG_TYPESCRIPT, typescript_ensure_schema_compliance);
  register_ast_post_process_callback(LANG_TYPESCRIPT, typescript_ast_post_process);
  log_debug("Registered TypeScript AST compliance callbacks");
}
