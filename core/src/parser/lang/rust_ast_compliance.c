/**
 * @file rust_ast_compliance.c
 * @brief Rust-specific schema compliance implementation
 *
 * This module implements Rust-specific schema compliance and post-processing
 * for the AST builder.
 */

#include "scopemux/ast.h"
#include "scopemux/ast_compliance.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Rust-specific schema compliance function
 *
 * Maps Tree-sitter Rust node names to the canonical AST node types so that
 * downstream IR, InfoBlock, and reference handling treat Rust like the other
 * supported languages.
 *
 * @param node The AST node to process
 * @param ctx Parser context
 * @return int Status code (0 for success)
 */
int rust_ensure_schema_compliance(ASTNode *node, ParserContext *ctx) {
  if (!node) {
    return -1;
  }

  (void)ctx;

  if (node->name) {
    if (strcmp(node->name, "source_file") == 0) {
      node->type = NODE_ROOT;
      free(node->name);
      node->name = strdup("ROOT");
      free(node->qualified_name);
      node->qualified_name = strdup("ROOT");
    } else if (strcmp(node->name, "function_item") == 0) {
      node->type = NODE_FUNCTION;
    } else if (strcmp(node->name, "struct_item") == 0) {
      node->type = NODE_STRUCT;
    } else if (strcmp(node->name, "enum_item") == 0) {
      node->type = NODE_ENUM;
    } else if (strcmp(node->name, "union_item") == 0) {
      node->type = NODE_UNION;
    } else if (strcmp(node->name, "trait_item") == 0) {
      node->type = NODE_INTERFACE;
    } else if (strcmp(node->name, "impl_item") == 0) {
      node->type = NODE_CLASS;
    } else if (strcmp(node->name, "type_item") == 0) {
      node->type = NODE_TYPEDEF;
    } else if (strcmp(node->name, "const_item") == 0 || strcmp(node->name, "static_item") == 0) {
      node->type = NODE_VARIABLE;
    } else if (strcmp(node->name, "mod_item") == 0) {
      node->type = NODE_MODULE;
    } else if (strcmp(node->name, "macro_definition") == 0) {
      node->type = NODE_MACRO;
    } else if (strcmp(node->name, "use_declaration") == 0) {
      node->type = NODE_INCLUDE;
    }
  }

  return 0;
}

/**
 * @brief Rust-specific AST post-processing
 *
 * @param root_node The root AST node
 * @param ctx Parser context
 * @return ASTNode* The processed AST
 */
ASTNode *rust_ast_post_process(ASTNode *root_node, ParserContext *ctx) {
  if (!root_node) {
    return NULL;
  }

  (void)ctx;
  log_debug("Applying Rust-specific AST post-processing");

  return root_node;
}

/**
 * @brief Register Rust language-specific callbacks
 */
void register_rust_ast_compliance(void) {
  register_schema_compliance_callback(LANG_RUST, rust_ensure_schema_compliance);
  register_ast_post_process_callback(LANG_RUST, rust_ast_post_process);
  log_debug("Registered Rust AST compliance callbacks");
}
