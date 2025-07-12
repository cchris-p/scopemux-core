#include "config/node_type_mapping_loader.h"
#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Centralized logging toggle for all test executables
int logging_enabled = 0;

/*
 * CRITICAL BUILD INTEGRATION NOTE:
 * This file MUST NOT be added directly to any test executable's source list.
 * It must only be linked via the test_utilities static library.
 * Direct inclusion will cause duplicate symbols and Criterion runtime errors.
 */

// Function prototypes for non-static functions
char *read_test_file(const char *language, const char *category, const char *file_name);
ASTNode *find_node_by_name(ASTNode *parent, const char *name, ASTNodeType type);
void assert_node_fields(ASTNode *node, const char *node_name);
int count_nodes_by_type(ASTNode *root, ASTNodeType type);
void dump_ast_structure(ASTNode *node, int level);
ASTNode *parse_cpp_ast(ParserContext *ctx, const char *source);

/* Helper function to read test files with robust error handling */
char *read_test_file(const char *language, const char *category, const char *file_name) {
  if (!category || !file_name) {
    fprintf(stderr, "ERROR: read_test_file called with NULL parameter(s)\n");
    cr_log_error("read_test_file called with NULL parameter(s)");
    return NULL;
  }

  fprintf(stderr, "DEBUG: read_test_file called for category=%s file_name=%s\n", category,
          file_name);

  char filepath[1024];
  FILE *f = NULL;

  // Only try canonical path: PROJECT_ROOT_DIR/core/examples/category/file_name
  const char *project_root = getenv("PROJECT_ROOT_DIR");
  if (project_root && strlen(project_root) > 0) {
    size_t path_len =
        snprintf(NULL, 0, "%s/core/examples/%s/%s", project_root, category, file_name);
    if (path_len < sizeof(filepath)) {
      snprintf(filepath, sizeof(filepath), "%s/core/examples/%s/%s", project_root, category,
               file_name);
      fprintf(stderr, "DEBUG: Trying canonical path using PROJECT_ROOT_DIR: %s\n", filepath);
      f = fopen(filepath, "rb");
      if (f) {
        cr_log_info("Successfully opened file using canonical PROJECT_ROOT_DIR: %s", filepath);
        goto file_found;
      }
    }
  }

  // Try canonical path relative to CWD
  size_t path_len = snprintf(NULL, 0, "core/examples/%s/%s", category, file_name);
  if (path_len < sizeof(filepath)) {
    snprintf(filepath, sizeof(filepath), "core/examples/%s/%s", category, file_name);
    fprintf(stderr, "DEBUG: Trying canonical path relative to CWD: %s\n", filepath);
    f = fopen(filepath, "rb");
    if (f) {
      cr_log_info("Successfully opened file using canonical CWD: %s", filepath);
      goto file_found;
    }
  }

  // If not found at canonical location, error
  fprintf(stderr, "ERROR: Failed to open test file at canonical location: core/examples/%s/%s\n",
          category, file_name);
  cr_log_error("Failed to open test file at canonical location: core/examples/%s/%s", category,
               file_name);
  return NULL;

file_found:
  if (!f) {
    fprintf(stderr, "ERROR: File handle is NULL despite reaching file_found label\n");
    cr_log_error("File handle is NULL despite reaching file_found label");
    return NULL;
  }

  // Get file size
  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "ERROR: Failed to seek to end of file: %s\n", strerror(errno));
    cr_log_error("Failed to seek to end of file: %s", strerror(errno));
    fclose(f);
    return NULL;
  }

  long length = ftell(f);
  if (length < 0) {
    fprintf(stderr, "ERROR: Failed to get file size: %s\n", strerror(errno));
    cr_log_error("Failed to get file size: %s", strerror(errno));
    fclose(f);
    return NULL;
  }
  fprintf(stderr, "DEBUG: File size is %ld bytes\n", length);

  if (fseek(f, 0, SEEK_SET) != 0) {
    fprintf(stderr, "ERROR: Failed to seek back to start of file: %s\n", strerror(errno));
    cr_log_error("Failed to seek back to start of file: %s", strerror(errno));
    fclose(f);
    return NULL;
  }

  // Allocate buffer with extra null terminator
  char *buffer = (char *)malloc(length + 1);
  if (!buffer) {
    fprintf(stderr, "ERROR: Failed to allocate memory for file contents (%ld bytes)\n", length + 1);
    cr_log_error("Failed to allocate memory for file contents (%ld bytes)", length + 1);
    fclose(f);
    return NULL;
  }

  // Read file contents
  size_t bytes_read = fread(buffer, 1, length, f);
  if (bytes_read != (size_t)length) {
    fprintf(stderr, "ERROR: Failed to read entire file (read %zu of %ld bytes): %s\n", bytes_read,
            length, ferror(f) ? strerror(errno) : "Unknown error");
    cr_log_error("Failed to read entire file (read %zu of %ld bytes)", bytes_read, length);
    free(buffer);
    fclose(f);
    return NULL;
  }

  // Null-terminate the buffer
  buffer[length] = '\0';
  fclose(f);

  fprintf(stderr, "DEBUG: Successfully read file contents\n");
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

/**
 * Parse C++ source code into an AST
 *
 * This function provides a standardized way to parse C++ code for tests.
 * It handles all the details of proper parser initialization and cleanup.
 *
 * @param ctx Parser context to use
 * @param source Source code to parse
 * @return Root AST node or NULL on failure
 */
ASTNode *parse_cpp_ast(ParserContext *ctx, const char *source) {
  if (!ctx || !source) {
    fprintf(stderr, "ERROR: Invalid arguments to parse_cpp_ast\n");
    return NULL;
  }

  // Set a very verbose logging level for debugging
  ctx->log_level = LOG_DEBUG;

  // Declare a filename for diagnostic messages
  const char *filename = "test_source.cpp";

  // Node type mapping is now hardcoded in the implementation.
  // No JSON file is needed or loaded for node type mapping.
  fprintf(stderr, "DEBUG: Node type mapping is hardcoded; no JSON config loaded.\n");

  // Output diagnostics
  fprintf(stderr, "DEBUG: Parsing C++ source with length %zu\n", strlen(source));

  // Parse the source code
  bool parse_result = parser_parse_string(ctx, source, strlen(source), filename, LANG_CPP);
  if (!parse_result) {
    fprintf(stderr, "ERROR: Failed to parse C++ source: %s\n",
            parser_get_last_error(ctx) ? parser_get_last_error(ctx) : "Unknown error");
    return NULL;
  }

  // Verify the AST root was created
  if (!ctx->ast_root) {
    fprintf(stderr, "ERROR: No AST root was created during parsing\n");

    // Add additional diagnostic information
    fprintf(stderr, "DIAGNOSTIC: TS parser is %s\n", ctx->ts_parser ? "initialized" : "NULL");
    fprintf(stderr, "DIAGNOSTIC: Language type is %d\n", ctx->language);

    // Try to parse again with more debug output
    fprintf(stderr, "RECOVERY: Attempting to re-parse with more debugging\n");

    // Force verbose diagnostics mode in the parser context
    ctx->log_level = LOG_DEBUG;

    // Re-parse with explicit query loading through parser context API
    bool reparse_result = parser_parse_string(ctx, source, strlen(source), filename, LANG_CPP);
    fprintf(stderr, "RECOVERY: Re-parse %s\n", reparse_result ? "succeeded" : "failed");

    if (reparse_result && ctx->ast_root) {
      fprintf(stderr, "RECOVERY: Successfully created AST root on second attempt\n");
    } else {
      fprintf(stderr, "RECOVERY: Failed to create AST root on second attempt\n");
    }

    return ctx->ast_root; // Return whatever we have, possibly still NULL
  }

  // Output AST statistics
  fprintf(stderr, "INFO: Successfully parsed C++ code into AST with %zu nodes\n",
          ctx->num_ast_nodes);

  // Return the AST root node
  return ctx->ast_root;
}
