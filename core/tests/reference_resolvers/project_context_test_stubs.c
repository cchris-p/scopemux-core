/**
 * @file project_context_test_stubs.c
 * @brief Test stub implementations for project context functions
 *
 * Contains minimal implementations of project context functions
 * specifically for reference resolver tests. These stubs are
 * kept separate from main stubs to avoid linker conflicts.
 */

#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>

// A simple hash map to store file paths and languages for tests
typedef struct {
  char **paths;
  LanguageType *languages;
  size_t count;
  size_t capacity;
} FileRegistry;

// Global registry for test files, separate from ProjectContext internals
static FileRegistry g_file_registry = {NULL, NULL, 0, 0};

/**
 * Initialize the file registry for tests
 */
static void init_file_registry(void) {
  if (g_file_registry.paths == NULL) {
    g_file_registry.capacity = 8;
    g_file_registry.paths = calloc(g_file_registry.capacity, sizeof(char *));
    g_file_registry.languages = calloc(g_file_registry.capacity, sizeof(LanguageType));
    g_file_registry.count = 0;
  }
}

/**
 * Free the file registry
 */
static void free_file_registry(void) {
  if (g_file_registry.paths) {
    for (size_t i = 0; i < g_file_registry.count; i++) {
      free(g_file_registry.paths[i]);
    }
    free(g_file_registry.paths);
    free(g_file_registry.languages);
    g_file_registry.paths = NULL;
    g_file_registry.languages = NULL;
    g_file_registry.count = 0;
    g_file_registry.capacity = 0;
  }
}

/**
 * Create a project context (minimal stub implementation)
 * Note: Only needed for tests, doesn't need to be fully functional
 */
ProjectContext *project_context_create(const char *project_root) {
  // Initialize the file registry for tests
  init_file_registry();

  // Create and initialize the project context
  ProjectContext *ctx = calloc(1, sizeof(ProjectContext));
  cr_assert(ctx != NULL, "Failed to allocate project context");

  // Copy root directory if provided
  if (project_root) {
    ctx->root_directory = strdup(project_root);
  }

  return ctx;
}

/**
 * Free a project context
 */
void project_context_free(ProjectContext *ctx) {
  if (!ctx) {
    return;
  }

  // Free parser contexts if any (for actual impl, not used in tests)
  if (ctx->file_contexts) {
    for (size_t i = 0; i < ctx->num_files; i++) {
      // Don't free the file_contexts here as they're typically managed elsewhere
    }
    free(ctx->file_contexts);
  }

  // Free root directory and context
  free(ctx->root_directory);
  free(ctx);

  // Note: We don't free the file registry here to maintain file info between tests
  // It will be freed by atexit handler or explicit cleanup function
}

/**
 * Add a file to the project context
 */
bool project_context_add_file(ProjectContext *ctx, const char *file_path, LanguageType language) {
  if (!ctx || !file_path) {
    return false;
  }

  // Update internal file registry for testing
  if (g_file_registry.count >= g_file_registry.capacity) {
    // Expand capacity
    size_t new_capacity = g_file_registry.capacity * 2;
    char **new_paths = realloc(g_file_registry.paths, new_capacity * sizeof(char *));
    if (!new_paths) {
      return false;
    }
    g_file_registry.paths = new_paths;

    LanguageType *new_langs =
        realloc(g_file_registry.languages, new_capacity * sizeof(LanguageType));
    if (!new_langs) {
      return false;
    }
    g_file_registry.languages = new_langs;
    g_file_registry.capacity = new_capacity;
  }

  // Add to the file registry
  g_file_registry.paths[g_file_registry.count] = strdup(file_path);
  g_file_registry.languages[g_file_registry.count] = language;
  g_file_registry.count++;

  // Update the project context file count (actual impl would do more)
  ctx->num_files = g_file_registry.count;

  return true;
}

/**
 * Get the number of files in the project
 */
size_t project_context_get_file_count(const ProjectContext *ctx) {
  if (!ctx) {
    return 0;
  }

  // Just return the count from our registry
  return g_file_registry.count;
}

/**
 * Get a file path at a specific index
 */
const char *project_context_get_file_path(const ProjectContext *ctx, size_t index) {
  if (!ctx || index >= g_file_registry.count) {
    return NULL;
  }

  return g_file_registry.paths[index];
}

/**
 * Get a file language at a specific index
 */
LanguageType project_context_get_file_language(const ProjectContext *ctx, size_t index) {
  if (!ctx || index >= g_file_registry.count) {
    return LANG_UNKNOWN;
  }

  return g_file_registry.languages[index];
}

/**
 * Release resources on program exit
 */
__attribute__((destructor)) static void cleanup_test_resources(void) { free_file_registry(); }
