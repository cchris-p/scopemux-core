#include "../../include/test_helpers.h"

/* Helper function to read test files */
char *read_test_file(const char *language, const char *category, const char *file_name) {
  char filepath[512];
  snprintf(filepath, sizeof(filepath), "../examples/%s/%s/%s", language, category, file_name);

  FILE *f = fopen(filepath, "rb");
  if (!f) {
    cr_log_error("Failed to open test file: %s", filepath);
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buffer = (char *)malloc(length + 1);
  if (buffer) {
    fread(buffer, 1, length, f);
    buffer[length] = '\0';
  }
  fclose(f);
  return buffer;
}

/* Helper function to check if an AST node has a specific child */
ASTNode *find_node_by_name(ASTNode *parent, const char *name, ASTNodeType type) {
  if (!parent || !name)
    return NULL;

  for (size_t i = 0; i < parent->num_children; i++) {
    ASTNode *child = parent->children[i];
    if (child && child->name && child->type == type && strcmp(child->name, name) == 0) {
      return child;
    }
  }
  return NULL;
}

/* Helper function to check if node has expected fields populated */
void assert_node_fields(ASTNode *node, const char *node_name) {
  cr_assert_not_null(node, "Node '%s' should not be NULL", node_name);
  cr_assert_not_null(node->name, "Node '%s' should have a name", node_name);
  cr_assert_str_eq(node->name, node_name, "Node name should be '%s'", node_name);

  // Source range should be valid
  cr_assert_gt(node->range.end.line, 0, "Node '%s' should have valid end line", node_name);

  // Check if qualified_name is populated
  cr_assert_not_null(node->qualified_name, "Node '%s' should have a qualified_name", node_name);
}

/* Helper function to count nodes of a specific type in the AST */
int count_nodes_by_type(ASTNode *root, ASTNodeType type) {
  if (!root)
    return 0;

  int count = (root->type == type) ? 1 : 0;
  for (size_t i = 0; i < root->num_children; i++) {
    count += count_nodes_by_type(root->children[i], type);
  }
  return count;
}

/* Helper function to dump AST structure for debugging */
void dump_ast_structure(ASTNode *node, int level) {
  if (!node)
    return;

  // Print indentation
  for (int i = 0; i < level; i++) {
    printf("  ");
  }

  // Print node information
  printf("%s (%d) [%zu children]\n", node->name ? node->name : "(unnamed)", node->type,
         node->num_children);

  // Recursively print children
  for (size_t i = 0; i < node->num_children; i++) {
    dump_ast_structure(node->children[i], level + 1);
  }
}
