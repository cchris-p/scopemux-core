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

// WI-018: incremental update skips unchanged content and re-parses changed content.
Test(project_context_delegation, incremental_update_noop_and_change, .init = setup_project,
     .fini = teardown_project) {
  const char *v1 = "int alpha;\n";
  const char *v2 = "int beta;\n";
  char path[512];
  bool changed = false;
  ParserContext *first_ctx;
  ParserContext *second_ctx;

  join_test_project_path("inc_a.c", path, sizeof(path));

  cr_assert(project_update_file_from_string(project, path, v1, strlen(v1), LANG_C, &changed),
            "Initial incremental parse should succeed");
  cr_assert(changed, "First update should report a change");
  cr_assert_eq(project->num_files, 1, "Project should hold one file");

  cr_assert(project_file_is_unchanged(project, path, v1, strlen(v1)),
            "Identical content should be detected as unchanged");
  cr_assert_not(project_file_is_unchanged(project, path, v2, strlen(v2)),
                "Different content should be detected as changed");

  first_ctx = project_get_file_context(project, path);
  cr_assert_not_null(first_ctx, "File context should be retrievable");

  // No-op: same content must not replace the context or invalidate derived state.
  cr_assert(project_context_rebuild_ir(project), "IR should rebuild before the no-op");
  cr_assert(project->ir_ready, "IR should be ready before the no-op");

  changed = true;
  cr_assert(project_update_file_from_string(project, path, v1, strlen(v1), LANG_C, &changed),
            "No-op update should succeed");
  cr_assert_not(changed, "Unchanged content must not report a change");
  cr_assert_eq(project->num_files, 1, "No-op update must not add a file");
  cr_assert(project_get_file_context(project, path) == first_ctx,
            "No-op update must not replace the parser context");
  cr_assert(project->ir_ready, "No-op update must not invalidate derived IR");

  // Change: new content re-parses in place and invalidates derived state.
  changed = false;
  cr_assert(project_update_file_from_string(project, path, v2, strlen(v2), LANG_C, &changed),
            "Changed update should succeed");
  cr_assert(changed, "Changed content should report a change");
  cr_assert_eq(project->num_files, 1, "Changed update must replace in place");
  cr_assert_not(project->ir_ready, "Changed update must invalidate derived IR");

  second_ctx = project_get_file_context(project, path);
  cr_assert_not_null(second_ctx, "Updated file context should be retrievable");
  cr_assert(second_ctx != first_ctx, "Changed update should replace the parser context");
  cr_assert_not_null(second_ctx->source_code, "Updated context should hold source");
  cr_assert(strstr(second_ctx->source_code, "beta") != NULL, "Updated source should be parsed");
}

// WI-018: incremental re-index reconciles durable state instead of wiping it.
Test(project_context_delegation, incremental_update_preserves_durable_plan_nodes,
     .init = setup_project, .fini = teardown_project) {
  const char *v1 = "int alpha;\n";
  const char *v2 = "int beta;\n";
  char path[512];
  bool changed = false;

  join_test_project_path("inc_b.c", path, sizeof(path));

  cr_assert(project_update_file_from_string(project, path, v1, strlen(v1), LANG_C, &changed),
            "Initial incremental parse should succeed");
  cr_assert_not_null(project_context_plan_node_create(project, "TASK-I", "add-store",
                                                      PROJECT_PLAN_NODE_NEW_SYMBOL),
                     "Plan node should be created");
  cr_assert_eq(project_context_get_plan_node_count(project), 1, "Plan store should hold one node");

  cr_assert(project_update_file_from_string(project, path, v2, strlen(v2), LANG_C, &changed),
            "Changed update should succeed");
  cr_assert_eq(project_context_get_plan_node_count(project), 1,
               "Incremental re-index must not wipe durable plan nodes");

  cr_assert(project_context_remove_file(project, path), "File removal should succeed");
  cr_assert_eq(project_context_get_plan_node_count(project), 1,
               "Removal must not wipe durable plan nodes");
  cr_assert_not(project->ir_ready, "Removal must invalidate derived IR");
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

Test(project_context_delegation, project_ir_snapshot_rust_calls, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  ASTNode *call_ref;
  char caller_path[512], callee_path[512];
  const ProjectIRSnapshot *snapshot;
  bool found_call_edge = false;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL, "Parser contexts should be created");

  join_test_project_path("main.rs", caller_path, sizeof(caller_path));
  join_test_project_path("helper.rs", callee_path, sizeof(callee_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_RUST;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_RUST;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  caller_fn->range.start.line = 0;
  caller_fn->range.end.line = 5;

  // A Rust call site is extracted as a NODE_IDENTIFIER child. No reference is
  // added by hand: project_resolve_references must resolve it via the Rust
  // resolver so the call graph edge is produced end-to-end.
  call_ref = make_named_node(NODE_IDENTIFIER, "helper", "helper", caller_path);
  call_ref->range.start.line = 2;
  call_ref->range.end.line = 2;
  cr_assert(ast_node_add_child(caller_fn, call_ref), "Call site should attach to caller");

  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Callee function should be tracked");

  project->file_contexts[0] = caller_ctx;
  project->file_contexts[1] = callee_ctx;
  project->num_files = 2;
  parser = NULL;

  cr_assert(symbol_table_register(project->symbol_table, "caller", caller_fn, caller_path,
                                  SCOPE_GLOBAL, LANG_RUST) != NULL,
            "Caller symbol should be registered");
  cr_assert(symbol_table_register(project->symbol_table, "helper", callee_fn, callee_path,
                                  SCOPE_GLOBAL, LANG_RUST) != NULL,
            "Helper symbol should be registered");

  cr_assert(project_resolve_references(project), "Rust references should resolve");
  snapshot = project_context_get_ir(project);
  cr_assert(snapshot != NULL, "Project IR snapshot should be available");
  cr_assert_eq(snapshot->call_graph_edge_count, 1, "Expected one Rust call graph edge");

  for (size_t i = 0; i < snapshot->call_graph_edge_count; i++) {
    const ProjectCallGraphEdgeIR *edge = &snapshot->call_graph_edges[i];
    if (edge->caller_symbol && edge->callee_symbol && strcmp(edge->caller_symbol, "caller") == 0 &&
        strcmp(edge->callee_symbol, "helper") == 0) {
      found_call_edge = true;
    }
  }
  cr_assert(found_call_edge, "Rust caller -> helper call graph edge should be present");
}

Test(project_context_delegation, rust_method_call_resolves_by_type, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *point_origin;
  ASTNode *vector_origin;
  ASTNode *call_ref;
  char file_path[512];

  cr_assert(ctx != NULL, "Parser context should be created");
  join_test_project_path("scoping.rs", file_path, sizeof(file_path));
  ctx->filename = strdup(file_path);
  ctx->language = LANG_RUST;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", file_path);
  caller_fn->range.start.line = 0;
  caller_fn->range.end.line = 8;

  // Two methods share the simple name `origin` but live under different impls.
  point_origin = make_named_node(NODE_METHOD, "origin", "point.rs.Point.origin", file_path);
  vector_origin = make_named_node(NODE_METHOD, "origin", "vector.rs.Vector.origin", file_path);

  // A `Vector::origin` call site must resolve to the Vector method, not Point's.
  call_ref = make_named_node(NODE_IDENTIFIER, "Vector::origin", "Vector::origin", file_path);
  call_ref->range.start.line = 3;
  call_ref->range.end.line = 3;
  cr_assert(ast_node_add_child(caller_fn, call_ref), "Call site should attach to caller");

  cr_assert(parser_add_ast_node(ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(ctx, point_origin), "Point method should be tracked");
  cr_assert(parser_add_ast_node(ctx, vector_origin), "Vector method should be tracked");

  project->file_contexts[0] = ctx;
  project->num_files = 1;
  parser = NULL;

  cr_assert(symbol_table_register(project->symbol_table, "point.rs.Point.origin", point_origin,
                                  file_path, SCOPE_FILE, LANG_RUST) != NULL,
            "Point method symbol should be registered");
  cr_assert(symbol_table_register(project->symbol_table, "vector.rs.Vector.origin", vector_origin,
                                  file_path, SCOPE_FILE, LANG_RUST) != NULL,
            "Vector method symbol should be registered");

  cr_assert(project_resolve_references(project), "Rust references should resolve");

  bool references_vector = false;
  bool references_point = false;
  for (size_t i = 0; i < call_ref->num_references; i++) {
    if (call_ref->references[i] == vector_origin) {
      references_vector = true;
    }
    if (call_ref->references[i] == point_origin) {
      references_point = true;
    }
  }
  cr_assert(references_vector, "Vector::origin should resolve to the Vector method");
  cr_assert_not(references_point, "Vector::origin should not resolve to Point::origin");
}

Test(project_context_delegation, info_block_registry_and_tiered_context, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ParserContext *python_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  ASTNode *call_ref;
  ASTNode *include_node;
  ASTNode *python_class;
  char caller_path[512], callee_path[512], python_path[512];
  const ProjectInfoBlockRegistry *registry;
  ProjectTieredContextRequest request = {0};
  ProjectTieredContextResult result = {0};
  const char *focus_ids[2];
  const char *summary_ids[1];
  bool saw_tier0 = false;
  bool saw_tier1 = false;
  bool saw_tier2 = false;
  bool saw_tier3 = false;
  bool saw_tier4 = false;
  bool saw_helper = false;
  bool saw_python_file_summary = false;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL && python_ctx != NULL,
            "Parser contexts should be created");

  join_test_project_path("main.c", caller_path, sizeof(caller_path));
  join_test_project_path("helper.c", callee_path, sizeof(callee_path));
  join_test_project_path("file2.py", python_path, sizeof(python_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_C;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_C;
  python_ctx->filename = strdup(python_path);
  python_ctx->language = LANG_PYTHON;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  python_class = make_named_node(NODE_CLASS, "Worker", "Worker", python_path);
  call_ref = make_named_node(NODE_IDENTIFIER, "helper", "helper", caller_path);
  include_node = make_named_node(NODE_INCLUDE, "helper.c", "helper.c", caller_path);
  include_node->raw_content = strdup("#include \"helper.c\"");
  include_node->owned_fields |= FIELD_RAW_CONTENT;

  cr_assert(ast_node_add_child(caller_fn, call_ref), "Call reference should be attached");
  cr_assert(ast_node_add_reference(call_ref, callee_fn), "Call reference should resolve to helper");
  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(caller_ctx, include_node), "Include node should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Callee function should be tracked");
  cr_assert(parser_add_ast_node(python_ctx, python_class), "Python class should be tracked");
  cr_assert(parser_context_add_dependency(caller_ctx, callee_ctx),
            "Dependency should be created between parser contexts");

  project->file_contexts[0] = caller_ctx;
  project->file_contexts[1] = callee_ctx;
  project->file_contexts[2] = python_ctx;
  project->num_files = 3;
  parser = NULL;

  cr_assert(symbol_table_register(project->symbol_table, "caller", caller_fn, caller_path, SCOPE_GLOBAL,
                                  LANG_C) != NULL,
            "Caller symbol should be registered");
  cr_assert(symbol_table_register(project->symbol_table, "helper", callee_fn, callee_path, SCOPE_GLOBAL,
                                  LANG_C) != NULL,
            "Helper symbol should be registered");
  cr_assert(symbol_table_register(project->symbol_table, "Worker", python_class, python_path,
                                  SCOPE_GLOBAL, LANG_PYTHON) != NULL,
            "Worker symbol should be registered");

  cr_assert(project_context_rebuild_ir(project), "Project IR snapshot should rebuild");
  registry = project_context_get_info_block_registry(project);
  cr_assert_not_null(registry, "InfoBlock registry should rebuild");
  cr_assert(project_context_find_info_block(project, "sym:caller") != NULL,
            "Caller symbol block should be addressable by ID");
  cr_assert(project_context_find_info_block(project, "file:") == NULL,
            "Partial IDs should not match registry entries");

  // WI-033: parsed blocks carry origin, lifecycle, provenance, and confidence.
  for (size_t i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    cr_assert_eq(block->origin, PROJECT_INFO_BLOCK_ORIGIN_PARSED,
                 "parsed blocks should be marked parsed (id=%s)", block->id ? block->id : "?");
    cr_assert_eq(block->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_NONE,
                 "parsed blocks should have no lifecycle (id=%s)", block->id ? block->id : "?");
    cr_assert_float_eq(block->confidence, 1.0f, 0.0001f,
                       "parsed blocks should be exact (id=%s)", block->id ? block->id : "?");
    cr_assert_not_null(block->provenance, "parsed blocks should carry provenance (id=%s)",
                       block->id ? block->id : "?");
  }

  // WI-033: origin filters restrict selection.
  request.origin_mask = 1u << PROJECT_INFO_BLOCK_ORIGIN_PLANNED;
  {
    ProjectTieredContextResult planned_only = {0};
    project_context_build_tiered_context(project, &request, &planned_only);
    cr_assert_eq(planned_only.selection_count, 0,
                 "filtering for planned blocks should return none while only parsed blocks exist");
    project_tiered_context_result_free(&planned_only);
  }
  request.origin_mask = 1u << PROJECT_INFO_BLOCK_ORIGIN_PARSED;

  for (size_t i = 0; i < registry->block_count; i++) {
    switch (registry->blocks[i].tier) {
    case PROJECT_CONTEXT_TIER_0:
      saw_tier0 = true;
      break;
    case PROJECT_CONTEXT_TIER_1:
      saw_tier1 = true;
      break;
    case PROJECT_CONTEXT_TIER_2:
      saw_tier2 = true;
      break;
    case PROJECT_CONTEXT_TIER_3:
      saw_tier3 = true;
      break;
    case PROJECT_CONTEXT_TIER_4:
      saw_tier4 = true;
      break;
    }
  }

  cr_assert(saw_tier0 && saw_tier1 && saw_tier2 && saw_tier3 && saw_tier4,
            "Registry should emit canonical Tier 0-4 blocks");

  focus_ids[0] = "sym:caller";
  focus_ids[1] = "sym:Worker";
  summary_ids[0] = python_path;
  request.focus_block_ids = focus_ids;
  request.focus_block_count = 2;
  request.summary_only_block_ids = (const char **)(const void *)summary_ids;
  request.summary_only_block_count = 0;
  request.anchor_symbol = NULL;
  request.anchor_file_path = NULL;
  request.min_tier = PROJECT_CONTEXT_TIER_1;
  request.max_tier = PROJECT_CONTEXT_TIER_3;
  request.include_related = true;
  request.include_dependencies = true;
  request.max_blocks = 12;
  request.max_tokens = 0;

  summary_ids[0] = "file:";
  {
    char *python_file_id = malloc(strlen("file:") + strlen(python_path) + 1);
    cr_assert_not_null(python_file_id, "Python file summary ID should allocate");
    sprintf(python_file_id, "file:%s", python_path);
    request.summary_only_block_ids = (const char **)&summary_ids[0];
    request.summary_only_block_count = 1;
    summary_ids[0] = python_file_id;

    cr_assert(project_context_build_tiered_context(project, &request, &result),
              "Tiered context request should succeed");

    for (size_t i = 0; i < result.selection_count; i++) {
      const ProjectTieredContextSelection *selection = &result.selections[i];
      if (selection->block->qualified_name && strcmp(selection->block->qualified_name, "helper") == 0) {
        saw_helper = true;
      }
      if (selection->block->kind == PROJECT_INFO_BLOCK_FILE && selection->block->file_path &&
          strcmp(selection->block->file_path, python_path) == 0 &&
          selection->disposition == PROJECT_CONTEXT_BLOCK_SUMMARIZED) {
        saw_python_file_summary = true;
      }
    }

    free(python_file_id);
  }

  cr_assert(result.selection_count >= 4,
            "Tiered context should include focused and related blocks across files/languages");
  cr_assert(saw_helper, "Related helper symbol should be pulled into tiered context");
  cr_assert(saw_python_file_summary,
            "Summary-only file blocks should be marked summarized in results");

  project_tiered_context_result_free(&result);
}

Test(project_context_delegation, searchable_index_and_prompt_assembly, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  ASTNode *call_ref;
  ASTNode *include_node;
  char caller_path[512], callee_path[512];
  ProjectSearchRequest search_request = {0};
  ProjectSearchResult search_result = {0};
  ProjectPromptAssemblyRequest prompt_request = {0};
  ProjectPromptAssemblyResult prompt_result = {0};
  bool saw_helper_hit = false;
  bool saw_summary = false;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL, "Parser contexts should be created");

  join_test_project_path("main.c", caller_path, sizeof(caller_path));
  join_test_project_path("helper.c", callee_path, sizeof(callee_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_C;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_C;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  cr_assert(ast_node_set_signature(caller_fn, strdup("int caller(void)"), AST_SOURCE_DEBUG_ALLOC),
            "Failed to set caller signature");
  cr_assert(ast_node_set_signature(callee_fn, strdup("int helper(void)"), AST_SOURCE_DEBUG_ALLOC),
            "Failed to set helper signature");
  cr_assert(ast_node_set_docstring(caller_fn, strdup("Entry point that delegates to helper"),
                                   AST_SOURCE_DEBUG_ALLOC),
            "Failed to set caller docstring");
  cr_assert(ast_node_set_docstring(callee_fn, strdup("Returns computed helper value"),
                                   AST_SOURCE_DEBUG_ALLOC),
            "Failed to set helper docstring");
  caller_fn->raw_content = strdup("int caller(void) {\n    int total = helper();\n    return total;\n}");
  caller_fn->owned_fields |= FIELD_RAW_CONTENT;
  callee_fn->raw_content = strdup("int helper(void) {\n    return 42;\n}");
  callee_fn->owned_fields |= FIELD_RAW_CONTENT;

  call_ref = make_named_node(NODE_IDENTIFIER, "helper", "helper", caller_path);
  include_node = make_named_node(NODE_INCLUDE, "helper.c", "helper.c", caller_path);
  include_node->raw_content = strdup("#include \"helper.c\"");
  include_node->owned_fields |= FIELD_RAW_CONTENT;

  cr_assert(ast_node_add_child(caller_fn, call_ref), "Call reference should be attached");
  cr_assert(ast_node_add_reference(call_ref, callee_fn), "Call reference should resolve to helper");
  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(caller_ctx, include_node), "Include node should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Helper function should be tracked");
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

  search_request.query_text = "helper";
  search_request.anchor_symbol = "caller";
  search_request.min_tier = PROJECT_CONTEXT_TIER_0;
  search_request.max_tier = PROJECT_CONTEXT_TIER_3;
  search_request.include_related = true;
  search_request.include_dependencies = true;
  search_request.max_hits = 6;

  cr_assert(project_context_search_info_blocks(project, &search_request, &search_result),
            "Indexed search should succeed");
  cr_assert(search_result.hit_count > 0, "Search should produce ranked hits");

  for (size_t i = 0; i < search_result.hit_count; i++) {
    if (search_result.hits[i].block->qualified_name &&
        strcmp(search_result.hits[i].block->qualified_name, "helper") == 0) {
      saw_helper_hit = true;
      cr_assert(search_result.hits[i].name_match || search_result.hits[i].text_match,
                "Helper hit should match the query text");
    }
  }

  cr_assert(saw_helper_hit, "Search results should include helper symbol hit");

  prompt_request.context_request.anchor_symbol = "caller";
  prompt_request.context_request.min_tier = PROJECT_CONTEXT_TIER_1;
  prompt_request.context_request.max_tier = PROJECT_CONTEXT_TIER_3;
  prompt_request.context_request.include_related = true;
  prompt_request.context_request.include_dependencies = true;
  prompt_request.context_request.max_blocks = 8;
  prompt_request.user_query = "What does caller depend on?";
  prompt_request.system_preamble = "Answer using the provided context only.";
  prompt_request.response_format = "Return a short bullet list.";
  prompt_request.include_block_metadata = true;
  prompt_request.max_prompt_tokens = 55;

  cr_assert(project_context_assemble_prompt(project, &prompt_request, &prompt_result),
            "Prompt assembly should succeed");
  cr_assert_not_null(prompt_result.prompt_text, "Prompt assembly should render prompt text");
  cr_assert(strstr(prompt_result.prompt_text, "User query: What does caller depend on?") != NULL,
            "Prompt text should include the user query");
  cr_assert(strstr(prompt_result.prompt_text, "sym:caller") != NULL,
            "Prompt text should include the focused caller block");
  cr_assert(strstr(prompt_result.prompt_text, "sym:helper") != NULL,
            "Prompt text should include the related helper block");

  for (size_t i = 0; i < prompt_result.context_result.selection_count; i++) {
    if (prompt_result.context_result.selections[i].disposition == PROJECT_CONTEXT_BLOCK_SUMMARIZED) {
      saw_summary = true;
      break;
    }
  }

  cr_assert(saw_summary || prompt_result.omitted_block_count > 0,
            "Prompt assembly should apply token-aware compression or omission");

  project_prompt_assembly_result_free(&prompt_result);
  project_search_result_free(&search_result);
}

Test(project_context_delegation, search_index_grows_for_long_block_text, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *ctx = parser_init();
  ASTNode *fn;
  char path[512];
  char long_content[512];
  ProjectSearchRequest search_request = {0};
  ProjectSearchResult search_result = {0};

  cr_assert(ctx != NULL, "Parser context should be created");

  /* A long path plus long block content exceeds the old fixed search-text
   * buffer; the index builder must grow instead of failing. */
  join_test_project_path(
      "an_extremely_long_file_name_used_to_exceed_the_old_search_buffer.c", path, sizeof(path));

  ctx->filename = strdup(path);
  ctx->language = LANG_C;

  fn = make_named_node(NODE_FUNCTION, "long_function", "long_function", path);
  memset(long_content, 'x', sizeof(long_content) - 1);
  long_content[sizeof(long_content) - 1] = '\0';
  fn->raw_content = strdup(long_content);
  fn->owned_fields |= FIELD_RAW_CONTENT;

  cr_assert(parser_add_ast_node(ctx, fn), "Function node should be tracked");

  project->file_contexts[0] = ctx;
  project->num_files = 1;
  parser = NULL;

  cr_assert(symbol_table_register(project->symbol_table, "long_function", fn, path, SCOPE_GLOBAL,
                                  LANG_C) != NULL,
            "Function symbol should be registered");
  cr_assert(project_context_rebuild_ir(project), "Project IR snapshot should rebuild");

  search_request.query_text = "long_function";
  search_request.min_tier = PROJECT_CONTEXT_TIER_0;
  search_request.max_tier = PROJECT_CONTEXT_TIER_3;
  search_request.max_hits = 4;

  cr_assert(project_context_search_info_blocks(project, &search_request, &search_result),
            "Search index must grow for long paths and block content");
  cr_assert(search_result.hit_count > 0, "Long-content block should still be searchable");

  project_search_result_free(&search_result);
}

Test(project_context_delegation, plan_node_projection_and_reconciliation, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  char caller_path[512], callee_path[512];
  char caller_file_id[600];
  const ProjectInfoBlockRegistry *registry;
  ProjectPlanNode *implemented_node;
  ProjectPlanNode *planned_node;
  ProjectPlanNode *stale_node;
  ProjectPlanNode *conflict_node;
  ProjectPlanNodeReconciliationResult reconcile = {0};
  ProjectTieredContextRequest planned_request = {0};
  ProjectTieredContextResult planned_result = {0};
  ProjectSearchRequest search_request = {0};
  ProjectSearchResult search_result = {0};
  size_t planned_blocks = 0;
  bool saw_plan_metadata = false;
  bool saw_plan_hit = false;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL, "Parser contexts should be created");

  join_test_project_path("main.c", caller_path, sizeof(caller_path));
  join_test_project_path("helper.c", callee_path, sizeof(callee_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_C;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_C;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Helper function should be tracked");

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

  snprintf(caller_file_id, sizeof(caller_file_id), "file:%s", caller_path);

  // WI-032: create projected plan nodes. A projected symbol that already exists
  // in parsed state reconciles to implemented; anchors drive stale/conflict.
  implemented_node = project_context_plan_node_create(project, "TASK-9", "expose-api",
                                                      PROJECT_PLAN_NODE_MODIFY_SYMBOL);
  cr_assert_not_null(implemented_node, "Plan node create should succeed");
  cr_assert(project_context_plan_node_set_title(project, implemented_node, "Expose helper API"),
            "Title setter should succeed");
  cr_assert(project_context_plan_node_set_projected_symbol(project, implemented_node, "helper"),
            "Projected symbol setter should succeed");

  planned_node = project_context_plan_node_create(project, "TASK-9", "add-parser",
                                                  PROJECT_PLAN_NODE_NEW_SYMBOL);
  cr_assert_not_null(planned_node, "Second plan node create should succeed");
  cr_assert(project_context_plan_node_set_desired_shape(project, planned_node,
                                                        "int parse(const char *input)"),
            "Desired shape setter should succeed");
  cr_assert(project_context_plan_node_set_rationale(project, planned_node,
                                                    "required by completion criteria"),
            "Rationale setter should succeed");
  cr_assert(project_context_plan_node_set_provenance(project, planned_node, "TASK-9:implementing"),
            "Provenance setter should succeed");
  cr_assert(project_context_plan_node_set_confidence(project, planned_node, 0.6f),
            "Confidence setter should succeed");
  cr_assert(project_context_plan_node_add_anchor(project, planned_node, caller_file_id),
            "Anchor add should succeed");
  cr_assert(project_context_plan_node_add_anchor(project, planned_node, caller_file_id),
            "Duplicate anchor add should be a no-op success");
  cr_assert(planned_node->anchor_count == 1, "Duplicate anchors should not be stored twice");

  stale_node = project_context_plan_node_create(project, "TASK-9", "remove-legacy",
                                                PROJECT_PLAN_NODE_REMOVE);
  cr_assert_not_null(stale_node, "Third plan node create should succeed");
  cr_assert(project_context_plan_node_add_anchor(project, stale_node, "sym:ghost"),
            "Stale node anchor should be added");

  conflict_node = project_context_plan_node_create(project, "TASK-9", "consolidate-utils",
                                                   PROJECT_PLAN_NODE_CONSOLIDATE);
  cr_assert_not_null(conflict_node, "Fourth plan node create should succeed");
  cr_assert(project_context_plan_node_add_anchor(project, conflict_node, caller_file_id),
            "Conflict node first anchor should be added");
  cr_assert(project_context_plan_node_add_anchor(project, conflict_node, "sym:ghost2"),
            "Conflict node second anchor should be added");

  // Duplicate ids must be rejected.
  cr_assert_null(project_context_plan_node_create(project, "TASK-9", "add-parser",
                                                  PROJECT_PLAN_NODE_NEW_SYMBOL),
                 "Duplicate plan node id should be rejected");
  cr_assert(project_context_get_plan_node_count(project) == 4, "Plan store should hold four nodes");
  cr_assert_not_null(project_context_find_plan_node(project, "plan:TASK-9:add-parser"),
                     "Plan node should be addressable by stable id");

  registry = project_context_get_info_block_registry(project);
  cr_assert_not_null(registry, "InfoBlock registry should rebuild with plan nodes");
  for (size_t i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->origin != PROJECT_INFO_BLOCK_ORIGIN_PLANNED) {
      continue;
    }
    planned_blocks++;
    cr_assert_eq(block->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_PLANNED,
                 "new plan blocks should start planned (id=%s)", block->id ? block->id : "?");
    if (block->id && strcmp(block->id, "plan:TASK-9:add-parser") == 0) {
      cr_assert_str_eq(block->desired_shape, "int parse(const char *input)",
                       "plan block should carry desired shape");
      cr_assert_str_eq(block->rationale, "required by completion criteria",
                       "plan block should carry rationale");
      cr_assert_str_eq(block->anchor_list, caller_file_id,
                       "plan block should carry joined anchors");
      cr_assert_eq(block->plan_kind, PROJECT_PLAN_NODE_NEW_SYMBOL,
                   "plan block should carry plan kind");
      saw_plan_metadata = true;
    }
  }
  cr_assert_eq(planned_blocks, 4, "Registry should project all four plan nodes (saw %zu)",
               planned_blocks);
  cr_assert(saw_plan_metadata, "Planned block should expose plan metadata");

  // WI-032: origin filtering covers planned nodes in tiered context and search.
  planned_request.origin_mask = 1u << PROJECT_INFO_BLOCK_ORIGIN_PLANNED;
  planned_request.min_tier = PROJECT_CONTEXT_TIER_0;
  planned_request.max_tier = PROJECT_CONTEXT_TIER_4;
  planned_request.max_blocks = 16;
  cr_assert(project_context_build_tiered_context(project, &planned_request, &planned_result),
            "Tiered context should succeed for planned-only requests");
  cr_assert_eq(planned_result.selection_count, 4,
               "planned-only tiered context should select the four plan nodes");
  for (size_t i = 0; i < planned_result.selection_count; i++) {
    cr_assert_eq(planned_result.selections[i].block->origin, PROJECT_INFO_BLOCK_ORIGIN_PLANNED,
                 "planned-only selection must contain only planned blocks");
  }
  project_tiered_context_result_free(&planned_result);

  search_request.query_text = "parse";
  search_request.min_tier = PROJECT_CONTEXT_TIER_0;
  search_request.max_tier = PROJECT_CONTEXT_TIER_4;
  search_request.max_hits = 10;
  search_request.origin_mask = 1u << PROJECT_INFO_BLOCK_ORIGIN_PLANNED;
  cr_assert(project_context_search_info_blocks(project, &search_request, &search_result),
            "Indexed search should succeed over planned blocks");
  for (size_t i = 0; i < search_result.hit_count; i++) {
    cr_assert_eq(search_result.hits[i].block->origin, PROJECT_INFO_BLOCK_ORIGIN_PLANNED,
                 "planned-only search must return only planned blocks");
    if (search_result.hits[i].block->id &&
        strcmp(search_result.hits[i].block->id, "plan:TASK-9:add-parser") == 0) {
      saw_plan_hit = true;
    }
  }
  cr_assert(saw_plan_hit, "Search should find the projected parser plan node by desired shape");
  project_search_result_free(&search_result);

  // WI-032: reconcile against parsed state.
  cr_assert(project_context_reconcile_plan_nodes(project, &reconcile),
            "Plan-node reconciliation should succeed");
  cr_assert(reconcile.implemented_count >= 1, "An existing projected symbol should reconcile implemented");
  cr_assert(reconcile.stale_count >= 1, "A fully vanished anchor set should reconcile stale");
  cr_assert(reconcile.conflict_count >= 1, "A partial anchor divergence should reconcile conflict");
  cr_assert_eq(implemented_node->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_IMPLEMENTED,
               "projected symbol present in parsed state should mark the node implemented");
  cr_assert_eq(planned_node->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_PLANNED,
               "valid anchored plan with no projected symbol should stay planned");
  cr_assert_eq(stale_node->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_STALE,
               "vanished anchors should mark the node stale");
  cr_assert_eq(conflict_node->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_CONFLICT,
               "partial anchor divergence should mark the node conflict");

  // Reconciliation does not delete nodes and the updated lifecycle is re-projected.
  cr_assert(project_context_get_plan_node_count(project) == 4,
            "reconciliation must never delete plan nodes");
  registry = project_context_get_info_block_registry(project);
  cr_assert_not_null(registry, "Registry should re-project after reconciliation");
  {
    const ProjectInfoBlock *implemented_block =
        project_context_find_info_block(project, "plan:TASK-9:expose-api");
    const ProjectInfoBlock *stale_block =
        project_context_find_info_block(project, "plan:TASK-9:remove-legacy");
    cr_assert_not_null(implemented_block, "implemented plan block should be findable");
    cr_assert_eq(implemented_block->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_IMPLEMENTED,
                 "re-projected implemented block should carry the new lifecycle");
    cr_assert_not_null(stale_block, "stale plan block should be findable");
    cr_assert_eq(stale_block->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_STALE,
                 "re-projected stale block should carry the new lifecycle");
  }

  project_plan_node_reconciliation_result_free(&reconcile);
}

Test(project_context_delegation, delta_and_map_query_api, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  char caller_path[512], callee_path[512];
  char caller_file_id[600];
  ProjectPlanNode *add_node;
  ProjectPlanNode *change_node;
  ProjectPlanNode *remove_node;
  ProjectPlanNode *reuse_node;
  ProjectDeltaResult delta = {0};
  ProjectMapQueryResult query = {0};
  const char *changed_files[1];
  bool saw_shape = false;
  bool saw_provenance = false;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL, "Parser contexts should be created");

  join_test_project_path("main.c", caller_path, sizeof(caller_path));
  join_test_project_path("helper.c", callee_path, sizeof(callee_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_C;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_C;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Helper function should be tracked");

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

  snprintf(caller_file_id, sizeof(caller_file_id), "file:%s", caller_path);

  add_node = project_context_plan_node_create(project, "TASK-D", "add-parser",
                                              PROJECT_PLAN_NODE_NEW_SYMBOL);
  cr_assert_not_null(add_node, "add node should be created");
  cr_assert(project_context_plan_node_set_desired_shape(project, add_node, "int parse(void)"),
            "shape setter should succeed");
  cr_assert(project_context_plan_node_set_provenance(project, add_node, "TASK-D:implementing"),
            "provenance setter should succeed");
  cr_assert(project_context_plan_node_add_anchor(project, add_node, caller_file_id),
            "add node anchor should be set");

  change_node = project_context_plan_node_create(project, "TASK-D", "log-helper",
                                                 PROJECT_PLAN_NODE_OBSERVABILITY_POINT);
  cr_assert_not_null(change_node, "change node should be created");
  cr_assert(project_context_plan_node_add_anchor(project, change_node, "sym:helper"),
            "change node anchor should be set");

  remove_node = project_context_plan_node_create(project, "TASK-D", "remove-legacy",
                                                 PROJECT_PLAN_NODE_REMOVE);
  cr_assert_not_null(remove_node, "remove node should be created");
  cr_assert(project_context_plan_node_add_anchor(project, remove_node, caller_file_id),
            "remove node anchor should be set");

  reuse_node = project_context_plan_node_create(project, "TASK-D", "reuse-helper",
                                                PROJECT_PLAN_NODE_MODIFY_SYMBOL);
  cr_assert_not_null(reuse_node, "reuse node should be created");
  cr_assert(project_context_plan_node_set_projected_symbol(project, reuse_node, "helper"),
            "reuse node projected symbol should be set");

  // WI-036: delta reports add/change/remove/reuse with trust and cost metadata.
  cr_assert(project_context_compute_delta(project, "TASK-D", "implementing", &delta),
            "delta computation should succeed");
  cr_assert_eq(delta.entry_count, 4, "delta should report all four plan nodes");
  cr_assert_eq(delta.add_count, 1, "delta should report one add");
  cr_assert_eq(delta.change_count, 1, "delta should report one change");
  cr_assert_eq(delta.remove_count, 1, "delta should report one remove");
  cr_assert_eq(delta.reuse_count, 1, "delta should report one reuse");
  cr_assert(delta.estimated_tokens > 0, "delta should estimate token cost");
  for (size_t i = 0; i < delta.entry_count; i++) {
    cr_assert_not_null(delta.entries[i].block, "delta entries should reference a block");
    cr_assert_not_null(delta.entries[i].provenance, "delta entries should carry provenance");
    if (delta.entries[i].projected_shape &&
        strcmp(delta.entries[i].projected_shape, "int parse(void)") == 0) {
      saw_shape = true;
    }
    if (delta.entries[i].provenance &&
        strcmp(delta.entries[i].provenance, "TASK-D:implementing") == 0) {
      saw_provenance = true;
    }
  }
  cr_assert(saw_shape, "delta should carry projected shape");
  cr_assert(saw_provenance, "delta should carry plan provenance");
  {
    ProjectDeltaResult other = {0};
    cr_assert(project_context_compute_delta(project, "TASK-OTHER", "implementing", &other),
              "delta computation for other task should succeed");
    cr_assert_eq(other.entry_count, 0, "delta should filter by task id");
    project_delta_result_free(&other);
  }
  project_delta_result_free(&delta);

  // query(node)
  cr_assert(project_context_query_node(project, "sym:helper", &query), "node query should succeed");
  cr_assert(query.item_count >= 1, "node query should resolve the helper block");
  cr_assert(query.estimated_tokens > 0, "query results should estimate tokens");
  cr_assert_not_null(query.items[0].provenance, "query results should carry provenance");
  cr_assert(query.items[0].confidence > 0.0f, "query results should carry confidence");
  project_map_query_result_free(&query);

  // query(resolve)
  cr_assert(project_context_query_resolve(project, "TASK-D", "implementing", &query),
            "resolve query should succeed");
  cr_assert(query.item_count >= 4, "resolve should return plan nodes and anchors");
  project_map_query_result_free(&query);

  // query(expand)
  cr_assert(project_context_query_expand(project, "sym:caller", PROJECT_CONTEXT_TIER_3, &query),
            "expand query should succeed");
  cr_assert(query.item_count >= 2, "expand should include the seed and related blocks");
  project_map_query_result_free(&query);

  // query(neighbors)
  cr_assert(project_context_query_neighbors(project, "sym:caller", 1, &query),
            "neighbors query should succeed");
  cr_assert(query.item_count >= 2, "neighbors should include the seed and at least one neighbor");
  project_map_query_result_free(&query);

  // query(observability)
  cr_assert(project_context_query_observability(project, "helper", &query),
            "observability query should succeed");
  cr_assert_eq(query.item_count, 1, "observability should find the helper observability plan");
  cr_assert_str_eq(query.items[0].block->id, "plan:TASK-D:log-helper",
                   "observability should return the planned observability node");
  project_map_query_result_free(&query);

  // query(change_impact)
  changed_files[0] = caller_path;
  cr_assert(project_context_query_change_impact(project, changed_files, 1, &query),
            "change impact query should succeed");
  cr_assert(query.item_count >= 2, "change impact should include the file, its symbols, and plans");
  project_map_query_result_free(&query);

  // query(duplicates)
  cr_assert(project_context_query_duplicates(project, NULL, &query),
            "duplicates query should succeed");
  project_map_query_result_free(&query);
}

Test(project_context_delegation, durable_plan_store_roundtrip, .init = setup_project,
     .fini = teardown_project) {
  ParserContext *caller_ctx = parser_init();
  ParserContext *callee_ctx = parser_init();
  ASTNode *caller_fn;
  ASTNode *callee_fn;
  char caller_path[512], callee_path[512];
  char caller_file_id[600];
  char store_path[600];
  char *json;
  ProjectPlanNode *add_node;
  ProjectPlanNode *obs_node;
  ProjectPlanNode *loaded;

  cr_assert(caller_ctx != NULL && callee_ctx != NULL, "Parser contexts should be created");

  join_test_project_path("main.c", caller_path, sizeof(caller_path));
  join_test_project_path("helper.c", callee_path, sizeof(callee_path));
  join_test_project_path("plan-store.json", store_path, sizeof(store_path));

  caller_ctx->filename = strdup(caller_path);
  caller_ctx->language = LANG_C;
  callee_ctx->filename = strdup(callee_path);
  callee_ctx->language = LANG_C;

  caller_fn = make_named_node(NODE_FUNCTION, "caller", "caller", caller_path);
  callee_fn = make_named_node(NODE_FUNCTION, "helper", "helper", callee_path);
  cr_assert(parser_add_ast_node(caller_ctx, caller_fn), "Caller function should be tracked");
  cr_assert(parser_add_ast_node(callee_ctx, callee_fn), "Helper function should be tracked");

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

  snprintf(caller_file_id, sizeof(caller_file_id), "file:%s", caller_path);

  add_node = project_context_plan_node_create(project, "TASK-S", "add-store",
                                              PROJECT_PLAN_NODE_NEW_SYMBOL);
  cr_assert_not_null(add_node, "add node should be created");
  cr_assert(project_context_plan_node_set_title(project, add_node, "Add durable store"),
            "title setter should succeed");
  cr_assert(project_context_plan_node_set_desired_shape(project, add_node, "int store_open(void)"),
            "shape setter should succeed");
  cr_assert(project_context_plan_node_set_rationale(project, add_node, "persist plan nodes"),
            "rationale setter should succeed");
  cr_assert(project_context_plan_node_set_provenance(project, add_node, "TASK-S:implementing"),
            "provenance setter should succeed");
  cr_assert(project_context_plan_node_set_confidence(project, add_node, 0.75f),
            "confidence setter should succeed");
  cr_assert(project_context_plan_node_add_anchor(project, add_node, caller_file_id),
            "anchor should be added");

  obs_node = project_context_plan_node_create(project, "TASK-S", "log-store",
                                              PROJECT_PLAN_NODE_OBSERVABILITY_POINT);
  cr_assert_not_null(obs_node, "observability node should be created");
  cr_assert(project_context_plan_node_add_anchor(project, obs_node, "sym:helper"),
            "observability anchor should be added");
  cr_assert(project_context_plan_node_set_lifecycle(project, obs_node,
                                                    PROJECT_INFO_BLOCK_LIFECYCLE_IN_PROGRESS),
            "lifecycle setter should succeed");

  // WI-031: serialize the durable store.
  json = project_context_plan_nodes_to_json(project);
  cr_assert_not_null(json, "plan store should serialize to JSON");
  cr_assert(strstr(json, "\"plan_nodes\"") != NULL, "JSON should contain the plan_nodes array");
  cr_assert(strstr(json, "plan:TASK-S:add-store") != NULL, "JSON should contain the plan id");
  cr_assert(strstr(json, caller_file_id) != NULL, "JSON should contain the anchor id");

  // Replacing load restores the full store.
  project_context_clear_plan_nodes(project);
  cr_assert_eq(project_context_get_plan_node_count(project), 0, "clear should empty the store");
  cr_assert(project_context_plan_nodes_from_json(project, json, false),
            "from_json should restore the store");
  free(json);
  cr_assert_eq(project_context_get_plan_node_count(project), 2, "store should round-trip two nodes");

  loaded = project_context_find_plan_node(project, "plan:TASK-S:add-store");
  cr_assert_not_null(loaded, "round-tripped add node should exist");
  cr_assert_str_eq(loaded->title, "Add durable store", "title should round-trip");
  cr_assert_str_eq(loaded->desired_shape, "int store_open(void)", "desired shape should round-trip");
  cr_assert_str_eq(loaded->rationale, "persist plan nodes", "rationale should round-trip");
  cr_assert_str_eq(loaded->provenance, "TASK-S:implementing", "provenance should round-trip");
  cr_assert_float_eq(loaded->confidence, 0.75f, 0.0001f, "confidence should round-trip");
  cr_assert_eq(loaded->anchor_count, 1, "anchor should round-trip");
  cr_assert_str_eq(loaded->anchor_ids[0], caller_file_id, "anchor id should round-trip");
  cr_assert_eq(loaded->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_PLANNED,
               "default lifecycle should round-trip");

  loaded = project_context_find_plan_node(project, "plan:TASK-S:log-store");
  cr_assert_not_null(loaded, "round-tripped observability node should exist");
  cr_assert_eq(loaded->lifecycle, PROJECT_INFO_BLOCK_LIFECYCLE_IN_PROGRESS,
               "in-progress lifecycle should round-trip");

  // Re-index is derived-only and must not wipe the durable store.
  cr_assert(project_context_rebuild_ir(project), "re-index should succeed");
  cr_assert_eq(project_context_get_plan_node_count(project), 2,
               "re-index must not lose durable plan nodes");

  // File save/load round-trip.
  cr_assert(project_context_plan_nodes_save(project, store_path), "save should succeed");
  project_context_clear_plan_nodes(project);
  cr_assert(project_context_plan_nodes_load(project, store_path, false), "load should succeed");
  cr_assert_eq(project_context_get_plan_node_count(project), 2, "file load should restore nodes");

  // Merging the same payload is idempotent.
  cr_assert(project_context_plan_nodes_load(project, store_path, true), "merge load should succeed");
  cr_assert_eq(project_context_get_plan_node_count(project), 2, "merge should not duplicate nodes");

  remove(store_path);
}
