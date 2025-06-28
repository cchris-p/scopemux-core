/**
 * @file project_context_tests.c
 * @brief Main test runner for project context functionality tests
 *
 * These tests verify that the project context module correctly manages files
 * and dependencies across a multi-file project, supporting interfile functionality.
 */

#include "scopemux/parser_context.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>

// Test fixture setup
static ProjectContext *project = NULL;
static ParserContext *parser = NULL;
static GlobalSymbolTable *symbols = NULL;

void setup_project() {
  project = project_context_create("test_project");
  cr_assert(project != NULL, "Failed to create project context for tests");

  parser = parser_context_create();
  cr_assert(parser != NULL, "Failed to create parser context for tests");

  symbols = symbol_table_create(16);
  cr_assert(symbols != NULL, "Failed to create symbol table for tests");
}

void teardown_project() {
  if (symbols) {
    symbol_table_free(symbols);
    symbols = NULL;
  }

  if (parser) {
    parser_context_free(parser);
    parser = NULL;
  }

  if (project) {
    project_context_free(project);
    project = NULL;
  }
}

// Test creation and basic properties
Test(project_context_delegation, create_delegate, .init = setup_project, .fini = teardown_project) {
  cr_assert(project != NULL, "Project context should be non-NULL");
  cr_assert_str_eq(project->name, "test_project", "Project name should be set correctly");
  cr_assert(project->num_files == 0, "Project should start with 0 files");
}

// Test file management
Test(project_context_delegation, file_management, .init = setup_project, .fini = teardown_project) {
  // Add multiple files
  bool added1 = project_context_add_file(project, "file1.c", LANG_C);
  bool added2 = project_context_add_file(project, "file2.py", LANG_PYTHON);

  cr_assert(added1, "First file should be added successfully");
  cr_assert(added2, "Second file should be added successfully");
  cr_assert(project->num_files == 2, "Project should have 2 files");

  // Get file by path
  ProjectFile *file1 = project_context_get_file(project, "file1.c");
  cr_assert(file1 != NULL, "Should find the first file");
  cr_assert_str_eq(file1->path, "file1.c", "File path should be correct");
  cr_assert(file1->language == LANG_C, "File language should be correct");

  // File removal
  bool removed = project_context_remove_file(project, "file1.c");
  cr_assert(removed, "File should be removed successfully");
  cr_assert(project->num_files == 1, "Project should have 1 file remaining");

  // File should no longer be accessible
  ProjectFile *not_found = project_context_get_file(project, "file1.c");
  cr_assert(not_found == NULL, "Removed file should not be found");
}

// Test dependency tracking
Test(project_context_delegation, dependency_management, .init = setup_project,
     .fini = teardown_project) {
  // Add files
  project_context_add_file(project, "main.c", LANG_C);
  project_context_add_file(project, "helper.c", LANG_C);
  project_context_add_file(project, "utils.c", LANG_C);

  // Add dependencies
  bool dep1_added = project_context_add_dependency(project, "main.c", "helper.c");
  bool dep2_added = project_context_add_dependency(project, "main.c", "utils.c");

  cr_assert(dep1_added, "First dependency should be added");
  cr_assert(dep2_added, "Second dependency should be added");

  // Get dependencies for main.c
  char **deps = NULL;
  size_t num_deps = project_context_get_dependencies(project, "main.c", &deps);

  cr_assert(num_deps == 2, "Should find 2 dependencies for main.c");

  // Verify dependency contents (ignoring order)
  bool found_helper = false;
  bool found_utils = false;

  for (size_t i = 0; i < num_deps; i++) {
    if (strcmp(deps[i], "helper.c") == 0) {
      found_helper = true;
    } else if (strcmp(deps[i], "utils.c") == 0) {
      found_utils = true;
    }
  }

  cr_assert(found_helper, "Should find helper.c in dependencies");
  cr_assert(found_utils, "Should find utils.c in dependencies");

  // Free the dependency array
  for (size_t i = 0; i < num_deps; i++) {
    free(deps[i]);
  }
  free(deps);
}

// Test interfile symbol context
Test(project_context_delegation, interfile_symbols, .init = setup_project,
     .fini = teardown_project) {
  // Add files
  project_context_add_file(project, "file1.c", LANG_C);
  project_context_add_file(project, "file2.c", LANG_C);

  // Create AST nodes for each file
  ASTNode *ast1 = ast_node_new(NODE_TYPE_ROOT);
  ast1->language = LANG_C;
  ASTNode *func1 = ast_node_new(NODE_TYPE_FUNCTION);
  func1->language = LANG_C;
  ast_node_set_name(func1, "func1");
  ast_node_add_child(ast1, func1);

  ASTNode *ast2 = ast_node_new(NODE_TYPE_ROOT);
  ast2->language = LANG_C;
  ASTNode *func2 = ast_node_new(NODE_TYPE_FUNCTION);
  func2->language = LANG_C;
  ast_node_set_name(func2, "func2");
  ast_node_add_child(ast2, func2);

  // Add ASTs to parser context
  parser_context_add_ast(parser, ast1, "file1.c");
  parser_context_add_ast(parser, ast2, "file2.c");

  // Extract symbols from both files into the symbol table
  project_context_extract_symbols(project, parser, symbols);

  // Verify that symbols from both files are available in the symbol table
  Symbol *sym1 = symbol_table_lookup(symbols, "func1");
  Symbol *sym2 = symbol_table_lookup(symbols, "func2");

  cr_assert(sym1 != NULL, "Symbol from file1 should be found");
  cr_assert(sym2 != NULL, "Symbol from file2 should be found");

  cr_assert_str_eq(sym1->file_path, "file1.c", "Symbol 1 file path should be correct");
  cr_assert_str_eq(sym2->file_path, "file2.c", "Symbol 2 file path should be correct");

  // Cleanup AST nodes (parser context takes ownership)
  ast_node_free(ast1);
  ast_node_free(ast2);
}

// Test main entry point
int main(int argc, char *argv[]) {
  // Initialize Criterion test framework
  struct criterion_test_set *tests = criterion_initialize();

  // Run tests
  int result = criterion_run_all_tests(tests);

  // Clean up
  criterion_finalize(tests);

  return result;
}
