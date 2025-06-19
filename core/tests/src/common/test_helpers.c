#include "../../include/test_helpers.h"
#include <errno.h>
#include <unistd.h>

/* Helper function to read test files */
char *read_test_file(const char *language, const char *category, const char *file_name) {
  char filepath[512];
  char project_root_path[512] = "";
  
  // First try using PROJECT_ROOT_DIR environment variable
  const char *project_root = getenv("PROJECT_ROOT_DIR");
  if (project_root) {
    snprintf(project_root_path, sizeof(project_root_path), "%s/core/tests/examples/%s/%s/%s", 
             project_root, language, category, file_name);
    FILE *f = fopen(project_root_path, "rb");
    if (f) {
      cr_log_info("Successfully opened file using PROJECT_ROOT_DIR: %s", project_root_path);
      goto file_found;
    }
  }
  
  // Get current working directory for logging and path calculation
  char cwd[512];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    cr_log_error("Failed to get current working directory");
    return NULL;
  }
  cr_log_info("Current working directory: %s", cwd);
  
  // Try multiple possible paths based on where the test might be running from
  const char *possible_paths[] = {
    "../../../core/tests/examples/%s/%s/%s",           // Original path
    "../../core/tests/examples/%s/%s/%s",            // One level up
    "../core/tests/examples/%s/%s/%s",               // Two levels up
    "./core/tests/examples/%s/%s/%s",                // From project root
    "/home/matrillo/apps/scopemux/core/tests/examples/%s/%s/%s"  // Absolute path
  };
  
  FILE *f = NULL;
  for (size_t i = 0; i < sizeof(possible_paths) / sizeof(possible_paths[0]); i++) {
    snprintf(filepath, sizeof(filepath), possible_paths[i], language, category, file_name);
    f = fopen(filepath, "rb");
    if (f) {
      cr_log_info("Successfully opened file: %s", filepath);
      goto file_found;
    }
  }
  
  // Try to construct paths by navigating from the build directory to the source directory
  // This is a common approach for finding test data in CMake projects
  if (strstr(cwd, "/build/")) {
    // If we're in a build subdirectory, try to navigate to source
    char *build_pos = strstr(cwd, "/build/");
    *build_pos = '\0'; // Terminate string at /build to get project root
    
    // Construct path from project root to examples
    snprintf(filepath, sizeof(filepath), "%s/core/tests/examples/%s/%s/%s", 
             cwd, language, category, file_name);
    f = fopen(filepath, "rb");
    if (f) {
      cr_log_info("Successfully opened file using build directory logic: %s", filepath);
      goto file_found;
    }
  }
  
  // If all paths failed
  cr_log_error("Failed to open test file: %s/%s/%s (from working dir: %s)", 
               language, category, file_name, cwd);
  return NULL;
  
file_found:

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
