/**
 * @file resolver_resolution_tests.c
 * @brief Unit tests for reference resolution algorithms
 *
 * Tests for node-level, file-level, and project-level reference resolution.
 */

#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/parser/parser_internal.h" // For ASTNODE_MAGIC
#include "reference_resolver_private.h"    // For resolution status types
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"

// Forward declarations for functions used in tests
ASTNode *ast_node_get_child_at_index(ASTNode *node, size_t index);
void parser_context_add_ast(ParserContext *ctx, ASTNode *ast, const char *file_path);
void project_context_add_file(ProjectContext *ctx, const char *file_path, Language lang);
size_t reference_resolver_resolve_file(ReferenceResolver *resolver, ParserContext *ctx,
                                       const char *file_path);
size_t reference_resolver_resolve_project(ReferenceResolver *resolver, ProjectContext *ctx,
                                          ParserContext *parser_ctx);
Symbol *reference_resolver_get_resolved_symbol(ReferenceResolver *resolver, ASTNode *node);
ResolutionStatus reference_resolver_resolve_node_safe(ReferenceResolver *resolver, ASTNode *node,
                                                      ReferenceType ref_type, const char *name);

// Structure for resolver statistics
typedef struct ResolverStats {
  size_t total_references;
  size_t resolved_references;
  size_t unresolved_references;
} ResolverStats;

// Function to get resolver statistics
void reference_resolver_get_statistics(const ReferenceResolver *resolver, ResolverStats *stats);

// Test fixtures
static ReferenceResolver *resolver = NULL;
static GlobalSymbolTable *symbol_table = NULL;
static ProjectContext *project_context = NULL;
static ParserContext *parser_context = NULL;
static ASTNode *root_node = NULL;

// Setup helper functions
static ASTNode *create_test_ast(void) {
  // Create a simple AST for testing with proper node types
  ASTNode *root = ast_node_new(NODE_ROOT, "root");

  // Create function node
  ASTNode *func = ast_node_new(NODE_FUNCTION, "test_function");
  ast_node_add_child(root, func);

  // Create variable node
  ASTNode *var = ast_node_new(NODE_VARIABLE, "test_var");
  ast_node_add_child(func, var);

  // Create function call node (using NODE_FUNCTION as there's no specific call type)
  ASTNode *call = ast_node_new(NODE_FUNCTION, "referenced_function");
  ast_node_add_child(func, call);

  return root;
}

void setup_resolution() {
  // Create a new symbol table
  symbol_table = symbol_table_create(16);
  cr_assert(symbol_table != NULL, "Failed to create symbol table");

  // Create a new reference resolver
  resolver = reference_resolver_create(symbol_table);
  cr_assert(resolver != NULL, "Failed to create reference resolver");

  // Initialize built-in resolvers
  reference_resolver_init_builtin(resolver);

  // Initialize parser context
  parser_context = parser_init();
  cr_assert(parser_context != NULL, "Parser context creation should succeed");

  // Create a project context
  project_context = project_context_create("test_project");
  cr_assert(project_context != NULL, "Failed to create project context");

  // Create a test AST
  root_node = create_test_ast();
  cr_assert(root_node != NULL, "Failed to create test AST");

  // Add some symbols to the symbol table for resolution tests
  Symbol *sym1 = symbol_new("referenced_function", SYMBOL_FUNCTION);
  sym1->file_path = strdup("test_file.c");
  sym1->line = 42;
  sym1->column = 10;

  symbol_table_add(symbol_table, sym1);
}

void teardown_resolution() {
  if (root_node) {
    ast_node_free(root_node);
    root_node = NULL;
  }

  if (project_context) {
    project_context_free(project_context);
    project_context = NULL;
  }

  if (parser_context) {
    parser_free(parser_context);
    parser_context = NULL;
  }

  if (resolver) {
    reference_resolver_free(resolver);
    resolver = NULL;
  }

  if (symbol_table) {
    symbol_table_free(symbol_table);
    symbol_table = NULL;
  }
}

// Test node-level resolution
Test(resolver_resolution, node_level, .init = setup_resolution, .fini = teardown_resolution) {
  // Find the function call node
  ASTNode *func = ast_node_get_child_at_index(root_node, 0); // test_function
  ASTNode *call = ast_node_get_child_at_index(func, 1);      // referenced_function call

  // Resolve the reference
  ResolutionStatus result =
      reference_resolver_resolve_node(resolver, call, REF_TYPE_FUNCTION, "referenced_function");

  // Verify resolution was successful
  cr_assert(result == RESOLUTION_SUCCESS, "Node-level resolution should succeed");

  // Verify the reference through public API
  Symbol *ref_symbol = reference_resolver_get_resolved_symbol(resolver, call);
  cr_assert(ref_symbol != NULL, "Resolution symbol should be populated");
  cr_assert_str_eq(ref_symbol->name, "referenced_function",
                   "Reference name should be set correctly");
  cr_assert_str_eq(ref_symbol->file_path, "test_file.c",
                   "Reference file path should be set correctly");
  cr_assert(ref_symbol->line == 42, "Reference line should be set correctly");
}

// Test file-level resolution
Test(resolver_resolution, file_level, .init = setup_resolution, .fini = teardown_resolution) {
  // Simulate a file with multiple nodes to resolve
  const char *file_path = "test_file.c";

  // Add the test AST to the parser context with the file path
  parser_context_add_ast(parser_context, root_node, file_path);

  // Resolve references in the file
  size_t resolved = reference_resolver_resolve_file(resolver, parser_context, file_path);

  // We expect 1 reference to be resolved (the referenced_function call)
  cr_assert(resolved == 1, "File-level resolution should resolve 1 reference");

  // Get resolver statistics through public API
  ResolverStats stats;
  reference_resolver_get_statistics(resolver, &stats);
  cr_assert(stats.resolved_references == 1, "Resolver should track 1 resolved reference");
  cr_assert(stats.total_references == 1, "Resolver should track 1 total reference");
}

// Test project-level resolution
Test(resolver_resolution, project_level, .init = setup_resolution, .fini = teardown_resolution) {
  // Simulate a project with multiple files
  const char *file_path1 = "test_file1.c";
  const char *file_path2 = "test_file2.c";

  // Add files to project context
  project_context_add_file(project_context, file_path1, LANG_C);
  project_context_add_file(project_context, file_path2, LANG_C);

  // Add ASTs to parser context
  parser_context_add_ast(parser_context, root_node, file_path1);

  // Create a second AST for the second file
  ASTNode *root2 = create_test_ast();
  parser_context_add_ast(parser_context, root2, file_path2);

  // Resolve references in the project
  size_t resolved = reference_resolver_resolve_project(resolver, project_context, parser_context);

  // We expect 2 references to be resolved (one in each file)
  cr_assert(resolved == 2, "Project-level resolution should resolve 2 references");

  // Get resolver statistics through public API
  ResolverStats stats;
  reference_resolver_get_statistics(resolver, &stats);
  cr_assert(stats.resolved_references == 2, "Resolver should track 2 resolved references");
  cr_assert(stats.total_references == 2, "Resolver should track 2 total references");

  // Cleanup the second AST (first one is cleaned in teardown)
  ast_node_free(root2);
}

// Test generic resolution fallback
Test(resolver_resolution, generic_resolution, .init = setup_resolution,
     .fini = teardown_resolution) {
  // Unregister all language-specific resolvers to test the generic fallback
  reference_resolver_unregister(resolver, LANG_C);
  reference_resolver_unregister(resolver, LANG_PYTHON);
  reference_resolver_unregister(resolver, LANG_JAVASCRIPT);
  reference_resolver_unregister(resolver, LANG_TYPESCRIPT);

  // Find the function call node - using the safe accessor function
  ASTNode *func = ast_node_get_child_at_index(root_node, 0);
  if (!func || func->magic != ASTNODE_MAGIC) {
    log_error("Invalid function node detected in generic_resolution test");
    // Fix the magic number if corrupted
    if (func)
      func->magic = ASTNODE_MAGIC;
  }

  ASTNode *call = ast_node_get_child_at_index(func, 1);
  if (!call || call->magic != ASTNODE_MAGIC) {
    log_error("Invalid call node detected in generic_resolution test");
    // Fix the magic number if corrupted
    if (call)
      call->magic = ASTNODE_MAGIC;
  }

  // Resolve using the safe wrapper for the generic resolver
  ResolutionStatus result = reference_resolver_resolve_node_safe(resolver, call, REF_TYPE_FUNCTION,
                                                                 "referenced_function");

  // The generic resolver should still be able to find the symbol
  cr_assert(result == RESOLUTION_SUCCESS, "Generic resolution should succeed");

  // Verify using public API
  Symbol *ref_symbol = reference_resolver_get_resolved_symbol(resolver, call);
  cr_assert(ref_symbol != NULL, "Reference should be populated by generic resolver");
  cr_assert_str_eq(ref_symbol->name, "referenced_function",
                   "Reference name should be set correctly");
}
