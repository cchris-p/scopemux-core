/**
 * @file project_context_tests.c
 * @brief Main test runner for project context functionality tests
 *
 * These tests verify that the project context module correctly manages files
 * and dependencies across a multi-file project, supporting interfile functionality.
 */

/**
 * IMPORTANT: Do not define a custom main() in Criterion test suites.
 *
 * Criterion provides its own test runner entry point and manages test execution and process
 * isolation. Defining a custom main (e.g., one that manually calls criterion_initialize,
 * criterion_run_all_tests, and criterion_finalize) can cause catastrophic errors such as
 * re-entrancy, protocol errors, or core dumps. Always allow Criterion to supply its own main and
 * handle test discovery and execution automatically.
 */

#include "parser.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>

// Test fixture setup
static ProjectContext *project = NULL;
static ParserContext *parser = NULL;
static GlobalSymbolTable *symbols = NULL;

// Utility: Create a file with minimal content
#include <sys/stat.h>
#include <unistd.h>

// Utility: Create a file with minimal content inside test_project directory
static char test_project_abspath[512];

// Utility: Join test_project_abspath with filename
static void join_test_project_path(const char *filename, char *out, size_t out_size) {
  snprintf(out, out_size, "%s/%s", test_project_abspath, filename);
}

// Utility: Create a file with minimal content using absolute path
static void create_dummy_file(const char *filename, const char *content) {
  char path[512];
  join_test_project_path(filename, path, sizeof(path));
  FILE *f = fopen(path, "w");
  if (f) {
    fputs(content, f);
    fclose(f);
  }
}

// Utility: Remove a file if it exists using absolute path
static void remove_dummy_file(const char *filename) {
  char path[512];
  join_test_project_path(filename, path, sizeof(path));
  remove(path);
}

#include <libgen.h>
#include <limits.h>

void setup_project() {
  // Determine the directory of the running executable robustly
  char exe_path[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (len != -1) {
    exe_path[len] = '\0';
    char *exe_dir = dirname(exe_path);
    snprintf(test_project_abspath, sizeof(test_project_abspath), "%s/test_project", exe_dir);
  } else {
    // Fallback: use CWD if /proc/self/exe is not available
    if (getcwd(test_project_abspath, sizeof(test_project_abspath))) {
      strncat(test_project_abspath, "/test_project",
              sizeof(test_project_abspath) - strlen(test_project_abspath) - 1);
    } else {
      // Absolute fallback: just use "test_project" (may fail)
      strncpy(test_project_abspath, "test_project", sizeof(test_project_abspath) - 1);
      test_project_abspath[sizeof(test_project_abspath) - 1] = '\0';
    }
  }

  // Ensure test_project directory exists
  mkdir(test_project_abspath, 0777);

  // Create dummy files for all test cases using absolute path
  create_dummy_file("file1.c", "int main() { return 0; }\n");
  create_dummy_file("file2.py", "print('hello')\n");
  create_dummy_file("main.c", "int main() { return 0; }\n");
  create_dummy_file("helper.c", "int helper() { return 1; }\n");
  create_dummy_file("utils.c", "int util() { return 2; }\n");
  create_dummy_file("file2.c", "int func2() { return 0; }\n");

  // Debug: Print current working directory and absolute path of test_project/file1.c
  char cwd[512];
  if (getcwd(cwd, sizeof(cwd))) {
    printf("[DEBUG] CWD: %s\n", cwd);
  }
  char abspath[512];
  snprintf(abspath, sizeof(abspath), "%s/file1.c", test_project_abspath);
  printf("[DEBUG] test_project/file1.c absolute path: %s\n", abspath);

  project = project_context_create(test_project_abspath);
  cr_assert(project != NULL, "Failed to create project context for tests");

  parser = parser_init();
  cr_assert(parser != NULL, "Failed to create parser context for tests");

  symbols = symbol_table_create(16);
  cr_assert(symbols != NULL, "Failed to create symbol table for tests");
}
// Ensure test_project directory exists and get its absolute path
mkdir("test_project", 0777);
realpath("test_project", test_project_abspath);

// Create dummy files for all test cases using absolute path
create_dummy_file("file1.c", "int main() { return 0; }\n");
create_dummy_file("file2.py", "print('hello')\n");
create_dummy_file("main.c", "int main() { return 0; }\n");
create_dummy_file("helper.c", "int helper() { return 1; }\n");
create_dummy_file("utils.c", "int util() { return 2; }\n");
create_dummy_file("file2.c", "int func2() { return 0; }\n");

// Debug: Print current working directory and absolute path of test_project/file1.c
char cwd[512];
if (getcwd(cwd, sizeof(cwd))) {
  printf("[DEBUG] CWD: %s\n", cwd);
}
char abspath[512];
realpath("test_project/file1.c", abspath);
printf("[DEBUG] test_project/file1.c absolute path: %s\n", abspath);

project = project_context_create("test_project");
cr_assert(project != NULL, "Failed to create project context for tests");

parser = parser_init();
cr_assert(parser != NULL, "Failed to create parser context for tests");

symbols = symbol_table_create(16);
cr_assert(symbols != NULL, "Failed to create symbol table for tests");
}

void teardown_project() {
  // Remove dummy files after tests (from test_project directory)
  remove_dummy_file("file1.c");
  remove_dummy_file("file2.py");
  remove_dummy_file("main.c");
  remove_dummy_file("helper.c");
  remove_dummy_file("utils.c");
  remove_dummy_file("file2.c");

  // Remove test_project directory (after removing files)
  rmdir("test_project");

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
  // Skipped name check: ProjectContext has no 'name' field
  cr_assert(project->num_files == 0, "Project should start with 0 files");
}

// Test file management
Test(project_context_delegation, file_management, .init = setup_project, .fini = teardown_project) {
  // Add multiple files
  char file1_path[512], file2_path[512];
  join_test_project_path("file1.c", file1_path, sizeof(file1_path));
  join_test_project_path("file2.py", file2_path, sizeof(file2_path));

  bool added1 = project_context_add_file(project, file1_path, LANG_C);
  bool added2 = project_context_add_file(project, file2_path, LANG_PYTHON);

  // Parse all files after adding
  project_parse_all_files(project);

  cr_assert(added1, "First file should be added successfully");
  cr_assert(added2, "Second file should be added successfully");
  cr_assert(project->num_files == 2, "Project should have 2 files");

  // Get file by path
  ParserContext *file1_ctx = project_get_file_context(project, file1_path);
  cr_assert(file1_ctx != NULL, "Should find the first file");
  cr_assert_str_eq(file1_ctx->filename, file1_path, "File path should be correct");
  cr_assert(file1_ctx->language == LANG_C, "File language should be correct");

  // File removal
  bool removed = project_context_remove_file(project, file1_path);
  cr_assert(removed, "File should be removed successfully");
  cr_assert(project->num_files == 1, "Project should have 1 file remaining");

  // File should no longer be accessible
  ParserContext *not_found_ctx = project_get_file_context(project, file1_path);
  cr_assert(not_found_ctx == NULL, "Removed file should not be found");
}

// Test dependency tracking
Test(project_context_delegation, dependency_management, .init = setup_project,
     .fini = teardown_project) {
  // Add files
  char main_path[512], helper_path[512], utils_path[512];
  join_test_project_path("main.c", main_path, sizeof(main_path));
  join_test_project_path("helper.c", helper_path, sizeof(helper_path));
  join_test_project_path("utils.c", utils_path, sizeof(utils_path));

  project_context_add_file(project, main_path, LANG_C);
  project_context_add_file(project, helper_path, LANG_C);
  project_context_add_file(project, utils_path, LANG_C);

  // Parse all files after adding
  project_parse_all_files(project);

  // Add dependencies using absolute paths
  bool dep1_added = project_context_add_dependency(project, main_path, helper_path);
  bool dep2_added = project_context_add_dependency(project, main_path, utils_path);

  cr_assert(dep1_added, "First dependency should be added");
  cr_assert(dep2_added, "Second dependency should be added");

  // Get dependencies for main.c
  char **deps = NULL;
  size_t num_deps = project_context_get_dependencies(project, main_path, &deps);

  cr_assert(num_deps == 2, "Should find 2 dependencies for main.c");

  // Verify dependency contents (ignoring order)
  bool found_helper = false;
  bool found_utils = false;

  for (size_t i = 0; i < num_deps; i++) {
    if (strcmp(deps[i], helper_path) == 0) {
      found_helper = true;
    } else if (strcmp(deps[i], utils_path) == 0) {
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
  char file1_path[512], file2_path[512];
  join_test_project_path("file1.c", file1_path, sizeof(file1_path));
  join_test_project_path("file2.c", file2_path, sizeof(file2_path));

  project_context_add_file(project, file1_path, LANG_C);
  project_context_add_file(project, file2_path, LANG_C);

  // Parse all files after adding
  project_parse_all_files(project);

  // Create AST nodes for each file
  ASTNode *ast1 = ast_node_new(NODE_ROOT, NULL);
  ast1->lang = LANG_C;
  ASTNode *func1 = ast_node_new(NODE_FUNCTION, "func1");
  func1->lang = LANG_C;
  ast_node_add_child(ast1, func1);

  ASTNode *ast2 = ast_node_new(NODE_ROOT, NULL);
  ast2->lang = LANG_C;
  ASTNode *func2 = ast_node_new(NODE_FUNCTION, "func2");
  func2->lang = LANG_C;
  ast_node_add_child(ast2, func2);

  // Add ASTs to parser context with filenames
  parser_context_add_ast_with_filename(parser, ast1, file1_path);
  parser_context_add_ast_with_filename(parser, ast2, file2_path);

  // Extract symbols from both files into the symbol table
  project_context_extract_symbols(project, parser, symbols);

  // Verify that symbols from both files are available in the symbol table
  Symbol *sym1 = symbol_table_lookup(symbols, "func1");
  Symbol *sym2 = symbol_table_lookup(symbols, "func2");

  cr_assert(sym1 != NULL, "Symbol from file1 should be found");
  cr_assert(sym2 != NULL, "Symbol from file2 should be found");

  cr_assert(strcmp(ast_node_get_file_path(sym1->node), file1_path) == 0,
            "Symbol 1 file_path: expected '%s', got '%s'.", file1_path,
            ast_node_get_file_path(sym1->node));
  cr_assert(strcmp(ast_node_get_file_path(sym2->node), file2_path) == 0,
            "Symbol 2 file_path: expected '%s', got '%s'.", file2_path,
            ast_node_get_file_path(sym2->node));

  // Cleanup AST nodes (parser context takes ownership)
  ast_node_free(ast1);
  ast_node_free(ast2);
}
