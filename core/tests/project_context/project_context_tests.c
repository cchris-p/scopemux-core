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

#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol.h"
#include "scopemux/symbol_table.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Test fixture setup
static ProjectContext *project = NULL;
static ParserContext *parser = NULL;
static GlobalSymbolTable *symbols = NULL;

// Utility: Create files inside a per-test-process project directory.
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

static ASTNode *make_named_node(ASTNodeType type, const char *name, const char *qualified_name,
                                const char *file_path) {
  SourceRange range = {0};
  ASTNode *node = ast_node_create(type, strdup(name), AST_SOURCE_DEBUG_ALLOC,
                                  strdup(qualified_name), AST_SOURCE_DEBUG_ALLOC, range);

  cr_assert(node != NULL, "Failed to create AST node");
  cr_assert(ast_node_set_file_path(node, strdup(file_path), AST_SOURCE_DEBUG_ALLOC),
            "Failed to set AST node file path");
  return node;
}

void setup_project(void) {
  // Use an isolated temp directory so Criterion workers do not race on the same files.
  char template[] = "/tmp/scopemux-project-context-XXXXXX";
  char *temp_dir = mkdtemp(template);
  cr_assert(temp_dir != NULL, "Failed to create temporary test project directory");

  strncpy(test_project_abspath, temp_dir, sizeof(test_project_abspath) - 1);
  test_project_abspath[sizeof(test_project_abspath) - 1] = '\0';

  // Create dummy files for all test cases using absolute path
  create_dummy_file("file1.c", "int func1() { return 0; }\n");
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

void teardown_project(void) {
  // Remove dummy files after tests (from test_project directory)
  remove_dummy_file("file1.c");
  remove_dummy_file("file2.py");
  remove_dummy_file("main.c");
  remove_dummy_file("helper.c");
  remove_dummy_file("utils.c");
  remove_dummy_file("file2.c");

  // Remove test_project directory (after removing files)
  rmdir(test_project_abspath);

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

  // Parse all files after adding. This path also registers symbols into the
  // project's global symbol table via register_file_symbols().
  project_parse_all_files(project);

  // Verify that symbols from both files are available in the project-level
  // symbol table used by inter-file resolution.
  SymbolEntry *sym1 = symbol_table_lookup(project->symbol_table, "func1");
  SymbolEntry *sym2 = symbol_table_lookup(project->symbol_table, "func2");

  cr_assert(sym1 != NULL, "Symbol from file1 should be found");
  cr_assert(sym2 != NULL, "Symbol from file2 should be found");

  cr_assert_str_eq(sym1->file_path, file1_path, "Symbol 1 should retain file1 path");
  cr_assert_str_eq(sym2->file_path, file2_path, "Symbol 2 should retain file2 path");
}

Test(project_context_delegation, project_ir_snapshot, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  ASTNode *call_ref;
  ASTNode *include_node;
  char caller_path[512], callee_path[512];
  const ProjectIRSnapshot *snapshot;
  bool found_call_edge = false;
  bool found_dependency_edge = false;
  bool found_caller_symbol = false;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL, "Parser contexts should be created");

  join_test_project_path("main.c", caller_path, sizeof(caller_path));
  join_test_project_path("helper.c", callee_path, sizeof(callee_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_C;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_C;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  cr_assert(ast_node_set_signature(caller_fn, strdup("int caller()"), AST_SOURCE_DEBUG_ALLOC),
            "Failed to set caller signature");
  cr_assert(ast_node_set_signature(callee_fn, strdup("int helper()"), AST_SOURCE_DEBUG_ALLOC),
            "Failed to set callee signature");
  cr_assert(ast_node_set_docstring(caller_fn, strdup("Calls helper"), AST_SOURCE_DEBUG_ALLOC),
            "Failed to set caller docstring");

  call_ref = make_named_node(NODE_IDENTIFIER, "helper", "helper", caller_path);
  cr_assert(ast_node_add_child(caller_fn, call_ref), "Call reference should be attached");
  cr_assert(ast_node_add_reference(call_ref, callee_fn), "Call reference should resolve to helper");

  include_node = make_named_node(NODE_INCLUDE, "helper.c", "helper.c", caller_path);
  include_node->raw_content = strdup("#include \"helper.c\"");
  include_node->owned_fields |= FIELD_RAW_CONTENT;
  cr_assert(include_node->raw_content != NULL, "Include node raw content should be set");

  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(caller_ctx, include_node), "Include node should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Callee function should be tracked");
  cr_assert(parser_context_add_dependency(caller_ctx, callee_ctx),
            "Dependency should be created between parser contexts");

  project->file_contexts[0] = caller_ctx;
  project->file_contexts[1] = callee_ctx;
  project->num_files = 2;
  parser = NULL;

  cr_assert(symbol_table_register(project->symbol_table, "caller", caller_fn, caller_path, SCOPE_GLOBAL,
                                  LANG_C) != NULL,
            "Caller symbol should be registered");
  cr_assert(symbol_table_register(project->symbol_table, "helper", callee_fn, callee_path, SCOPE_GLOBAL,
                                  LANG_C) != NULL,
            "Helper symbol should be registered");

  cr_assert(project_context_rebuild_ir(project), "Project IR snapshot should rebuild");
  snapshot = project_context_get_ir(project);
  cr_assert(snapshot != NULL, "Project IR snapshot should be available");
  cr_assert_eq(snapshot->symbol_count, 2, "Expected two symbol IR entries");
  cr_assert_eq(snapshot->resolved_reference_count, 1,
               "Expected one resolved reference in symbol IR");
  cr_assert_eq(snapshot->call_graph_edge_count, 1, "Expected one call graph edge");
  cr_assert(snapshot->dependency_count >= 2,
            "Expected include and file relationship dependency edges");

  for (size_t i = 0; i < snapshot->symbol_count; i++) {
    const ProjectSymbolIR *symbol = &snapshot->symbols[i];
    if (symbol->qualified_name && strcmp(symbol->qualified_name, "caller") == 0) {
      found_caller_symbol = true;
      cr_assert_str_eq(symbol->signature, "int caller()", "Caller signature should be preserved");
      cr_assert_eq(symbol->resolved_reference_count, 1,
                   "Caller should own one resolved reference");
      cr_assert_eq(symbol->visibility, PROJECT_IR_VISIBILITY_PUBLIC,
                   "Caller should default to public visibility");
    }
  }

  for (size_t i = 0; i < snapshot->call_graph_edge_count; i++) {
    const ProjectCallGraphEdgeIR *edge = &snapshot->call_graph_edges[i];
    if (edge->caller_symbol && edge->callee_symbol && strcmp(edge->caller_symbol, "caller") == 0 &&
        strcmp(edge->callee_symbol, "helper") == 0) {
      found_call_edge = true;
      cr_assert_str_eq(edge->caller_file_path, caller_path,
                       "Call graph edge should retain caller file path");
      cr_assert_str_eq(edge->callee_file_path, callee_path,
                       "Call graph edge should retain callee file path");
    }
  }

  for (size_t i = 0; i < snapshot->dependency_count; i++) {
    const ProjectDependencyIR *edge = &snapshot->dependencies[i];
    if (edge->source_file_path && edge->target_file_path &&
        strcmp(edge->source_file_path, caller_path) == 0 && strcmp(edge->target_file_path, callee_path) == 0 &&
        edge->kind == PROJECT_DEPENDENCY_INCLUDE) {
      found_dependency_edge = true;
    }
  }

  cr_assert(found_caller_symbol, "Caller symbol IR entry should be present");
  cr_assert(found_call_edge, "Cross-file call graph edge should be present");
  cr_assert(found_dependency_edge, "Resolved include dependency edge should be present");
}
