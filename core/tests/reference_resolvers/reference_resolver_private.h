#ifndef SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H
#define SCOPEMUX_REFERENCE_RESOLVER_PRIVATE_H

#include "scopemux/ast.h"
#include "scopemux/language.h"

static inline ASTNode *make_test_node(ASTNodeType type, const char *name, Language lang) {
  ASTNode *node = ast_node_new(type, (char *)name, AST_SOURCE_STATIC);
  if (node) {
    node->lang = lang;
  }
  return node;
}

static inline ASTNode *ast_node_child_at(ASTNode *node, size_t index) {
  if (!node || index >= node->num_children) {
    return NULL;
  }
  return node->children[index];
}

#endif
