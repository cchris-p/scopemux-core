/**
 * @file test_interfile_relationships.c
 * @brief Test suite for inter-file relationship functionality
 *
 * These tests validate the basic functionality of the ProjectContext,
 * GlobalSymbolTable, and ReferenceResolver components working together
 * to resolve cross-file references.
 */

#include "scopemux/project_context.h"
#include "scopemux/symbol_table.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/parser.h"
#include "scopemux/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test utilities
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s, line %d\n", #condition, __LINE__); \
            return false; \
        } \
    } while (0)

// Forward declarations for test cases
static bool test_project_context_create_free();
static bool test_symbol_table_registration();
static bool test_reference_resolution_simple();
static bool test_multi_file_parsing();

// Test files
static const char *TEST_ROOT_DIR = "./test_projects";
static const char *TEST_PROJ_1 = "simple_project";
static const char *TEST_FILE_1 = "module1.c";
static const char *TEST_FILE_2 = "module2.c";

// Sample file contents for testing
static const char *TEST_MODULE1_CONTENT = 
    "// Test module 1\n"
    "\n"
    "int global_var = 42;\n"
    "\n"
    "int test_function(int param) {\n"
    "    return param * 2;\n"
    "}\n";

static const char *TEST_MODULE2_CONTENT = 
    "// Test module 2\n"
    "\n"
    "// This references module1.c\n"
    "extern int global_var;\n"
    "\n"
    "int use_function() {\n"
    "    return test_function(global_var);\n"
    "}\n";

/**
 * Helper to set up test files
 */
static bool setup_test_files() {
    // Create test directory
    char cmd[512];
    sprintf(cmd, "mkdir -p %s/%s", TEST_ROOT_DIR, TEST_PROJ_1);
    if (system(cmd) != 0) {
        fprintf(stderr, "Failed to create test directory\n");
        return false;
    }
    
    // Create test file 1
    char path1[512];
    sprintf(path1, "%s/%s/%s", TEST_ROOT_DIR, TEST_PROJ_1, TEST_FILE_1);
    FILE *f1 = fopen(path1, "w");
    if (!f1) {
        fprintf(stderr, "Failed to create test file 1\n");
        return false;
    }
    fputs(TEST_MODULE1_CONTENT, f1);
    fclose(f1);
    
    // Create test file 2
    char path2[512];
    sprintf(path2, "%s/%s/%s", TEST_ROOT_DIR, TEST_PROJ_1, TEST_FILE_2);
    FILE *f2 = fopen(path2, "w");
    if (!f2) {
        fprintf(stderr, "Failed to create test file 2\n");
        return false;
    }
    fputs(TEST_MODULE2_CONTENT, f2);
    fclose(f2);
    
    return true;
}

/**
 * Helper to clean up test files
 */
static void cleanup_test_files() {
    char cmd[512];
    sprintf(cmd, "rm -rf %s", TEST_ROOT_DIR);
    system(cmd);
}

/**
 * Main test runner
 */
int main(int argc, char *argv[]) {
    // Setup test environment
    if (!setup_test_files()) {
        fprintf(stderr, "Test setup failed\n");
        return 1;
    }
    
    // Set logging level
    log_set_level(LOG_INFO);
    
    printf("Running inter-file relationship tests...\n");
    
    // Run test cases
    bool all_passed = true;
    
    printf("Test: Project context create/free... ");
    if (test_project_context_create_free()) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
        all_passed = false;
    }
    
    printf("Test: Symbol table registration... ");
    if (test_symbol_table_registration()) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
        all_passed = false;
    }
    
    printf("Test: Simple reference resolution... ");
    if (test_reference_resolution_simple()) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
        all_passed = false;
    }
    
    printf("Test: Multi-file parsing... ");
    if (test_multi_file_parsing()) {
        printf("PASSED\n");
    } else {
        printf("FAILED\n");
        all_passed = false;
    }
    
    // Cleanup test environment
    cleanup_test_files();
    
    if (all_passed) {
        printf("All tests PASSED\n");
        return 0;
    } else {
        printf("Some tests FAILED\n");
        return 1;
    }
}

/**
 * Test case: Create and free project context
 */
static bool test_project_context_create_free() {
    char test_dir[512];
    sprintf(test_dir, "%s/%s", TEST_ROOT_DIR, TEST_PROJ_1);
    
    // Test creation
    ProjectContext *project = project_context_create(test_dir);
    TEST_ASSERT(project != NULL);
    TEST_ASSERT(project->root_directory != NULL);
    TEST_ASSERT(strcmp(project->root_directory, test_dir) == 0);
    TEST_ASSERT(project->file_contexts != NULL);
    TEST_ASSERT(project->discovered_files != NULL);
    TEST_ASSERT(project->symbol_table != NULL);
    
    // Test configuration
    ProjectConfig config;
    config.parse_headers = false;
    config.follow_includes = true;
    config.resolve_external_symbols = true;
    config.max_files = 100;
    config.max_include_depth = 5;
    config.log_level = LOG_DEBUG;
    
    project_context_set_config(project, &config);
    TEST_ASSERT(project->config.parse_headers == false);
    TEST_ASSERT(project->config.follow_includes == true);
    TEST_ASSERT(project->config.resolve_external_symbols == true);
    TEST_ASSERT(project->config.max_files == 100);
    TEST_ASSERT(project->config.max_include_depth == 5);
    TEST_ASSERT(project->config.log_level == LOG_DEBUG);
    
    // Free the project context
    project_context_free(project);
    
    return true;
}

/**
 * Test case: Symbol table registration
 */
static bool test_symbol_table_registration() {
    // Create symbol table
    GlobalSymbolTable *table = symbol_table_create(16);
    TEST_ASSERT(table != NULL);
    
    // Create a dummy AST node
    ASTNode node;
    memset(&node, 0, sizeof(ASTNode));
    node.type = NODE_FUNCTION;
    node.name = strdup("test_function");
    node.qualified_name = strdup("module.test_function");
    
    // Register the symbol
    SymbolEntry *entry = symbol_table_register(table, node.qualified_name, &node, 
                                            "/path/to/file.c", SCOPE_GLOBAL, LANG_C);
    TEST_ASSERT(entry != NULL);
    TEST_ASSERT(entry->node == &node);
    TEST_ASSERT(strcmp(entry->qualified_name, "module.test_function") == 0);
    TEST_ASSERT(strcmp(entry->simple_name, "test_function") == 0);
    
    // Look up the symbol by qualified name
    SymbolEntry *found = symbol_table_lookup(table, "module.test_function");
    TEST_ASSERT(found != NULL);
    TEST_ASSERT(found->node == &node);
    
    // Look up the symbol by simple name with scope
    found = symbol_table_scope_lookup(table, "test_function", "module", LANG_C);
    TEST_ASSERT(found != NULL);
    TEST_ASSERT(found->node == &node);
    
    // Add scope prefix and try lookup
    TEST_ASSERT(symbol_table_add_scope(table, "module"));
    found = symbol_table_scope_lookup(table, "test_function", NULL, LANG_C);
    TEST_ASSERT(found != NULL);
    TEST_ASSERT(found->node == &node);
    
    // Try non-existent symbol
    found = symbol_table_lookup(table, "nonexistent");
    TEST_ASSERT(found == NULL);
    
    // Check stats
    size_t capacity, size, collisions;
    symbol_table_get_stats(table, &capacity, &size, &collisions);
    TEST_ASSERT(capacity == 16);
    TEST_ASSERT(size == 1);
    TEST_ASSERT(collisions == 0);
    
    // Clean up
    free(node.name);
    free(node.qualified_name);
    symbol_table_free(table);
    
    return true;
}

/**
 * Test case: Simple reference resolution
 */
static bool test_reference_resolution_simple() {
    // Create symbol table
    GlobalSymbolTable *table = symbol_table_create(16);
    TEST_ASSERT(table != NULL);
    
    // Create nodes
    ASTNode node_def;
    memset(&node_def, 0, sizeof(ASTNode));
    node_def.type = NODE_FUNCTION;
    node_def.name = strdup("test_function");
    node_def.qualified_name = strdup("module.test_function");
    node_def.language_type = LANG_C;
    
    ASTNode node_ref;
    memset(&node_ref, 0, sizeof(ASTNode));
    node_ref.type = NODE_FUNCTION_CALL;
    node_ref.name = strdup("test_function");
    node_ref.language_type = LANG_C;
    
    // Register the definition
    SymbolEntry *entry = symbol_table_register(table, node_def.qualified_name, &node_def, 
                                            "/path/to/file.c", SCOPE_GLOBAL, LANG_C);
    TEST_ASSERT(entry != NULL);
    
    // Create resolver
    ReferenceResolver *resolver = reference_resolver_create(table);
    TEST_ASSERT(resolver != NULL);
    
    // Initialize built-in resolvers
    TEST_ASSERT(reference_resolver_init_builtin(resolver));
    
    // Try to resolve the reference
    ResolutionResult result = reference_resolver_resolve_node(
        resolver, &node_ref, REFERENCE_CALL, "module.test_function");
    TEST_ASSERT(result == RESOLUTION_SUCCESS);
    TEST_ASSERT(node_ref.num_references == 1);
    TEST_ASSERT(node_ref.references[0] == &node_def);
    
    // Clean up
    free(node_def.name);
    free(node_def.qualified_name);
    free(node_ref.name);
    free(node_ref.references);
    reference_resolver_free(resolver);
    symbol_table_free(table);
    
    return true;
}

/**
 * Test case: Multi-file parsing
 */
static bool test_multi_file_parsing() {
    char test_dir[512];
    sprintf(test_dir, "%s/%s", TEST_ROOT_DIR, TEST_PROJ_1);
    
    char module1_path[512];
    sprintf(module1_path, "%s/%s", test_dir, TEST_FILE_1);
    
    char module2_path[512];
    sprintf(module2_path, "%s/%s", test_dir, TEST_FILE_2);
    
    // Create project context
    ProjectContext *project = project_context_create(test_dir);
    TEST_ASSERT(project != NULL);
    
    // Add files to the project
    TEST_ASSERT(project_add_file(project, module1_path, LANG_C));
    TEST_ASSERT(project_add_file(project, module2_path, LANG_C));
    TEST_ASSERT(project->num_discovered == 2);
    
    // At this point, we would parse the files and resolve references
    // but that requires a working parser implementation to be in place
    
    // Just verify that our project structure looks correct for now
    TEST_ASSERT(project->num_files == 0);  // Not parsed yet
    TEST_ASSERT(project->num_discovered == 2);
    
    // Clean up
    project_context_free(project);
    
    return true;
}
