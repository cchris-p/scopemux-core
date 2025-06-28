/**
 * @file project_context_stubs.c
 * @brief Stub implementations for project context API for testing
 *
 * This file provides minimal implementations of project context functions
 * to enable testing of reference resolvers without requiring the full
 * project context implementation.
 */

#include "scopemux/project_context.h"
#include <stdlib.h>
#include <string.h>

// Basic project context structure
typedef struct ProjectContextImpl {
  char **file_paths;
  LanguageType *languages;
  size_t num_files;
  size_t capacity;
} ProjectContextImpl;

// Create a new project context
ProjectContext *project_context_create(const char *root_directory) {
  ProjectContextImpl *ctx = malloc(sizeof(ProjectContextImpl));
  if (!ctx) {
    return NULL;
  }

  ctx->file_paths = NULL;
  ctx->languages = NULL;
  ctx->num_files = 0;
  ctx->capacity = 0;

  return (ProjectContext *)ctx;
}

// Free project context resources
void project_context_free(ProjectContext *ctx) {
  ProjectContextImpl *impl = (ProjectContextImpl *)ctx;
  if (!impl) {
    return;
  }

  // Free owned resources
  for (size_t i = 0; i < impl->num_files; i++) {
    free(impl->file_paths[i]);
  }

  free(impl->file_paths);
  free(impl->languages);
  free(impl);
}

// Add a file to project context
void project_context_add_file(ProjectContext *ctx, const char *file_path, LanguageType lang) {
  ProjectContextImpl *impl = (ProjectContextImpl *)ctx;
  if (!impl || !file_path) {
    return;
  }

  // Grow arrays if needed
  if (impl->num_files >= impl->capacity) {
    size_t new_capacity = impl->capacity == 0 ? 4 : impl->capacity * 2;
    char **new_paths = realloc(impl->file_paths, new_capacity * sizeof(char *));
    LanguageType *new_langs = realloc(impl->languages, new_capacity * sizeof(LanguageType));

    if (!new_paths || !new_langs) {
      if (new_paths)
        free(new_paths);
      if (new_langs)
        free(new_langs);
      return;
    }

    impl->file_paths = new_paths;
    impl->languages = new_langs;
    impl->capacity = new_capacity;
  }

  // Add the file path and language
  impl->file_paths[impl->num_files] = strdup(file_path);
  impl->languages[impl->num_files] = lang;
  impl->num_files++;
}

// Get the number of files in the project context
size_t project_context_get_file_count(const ProjectContext *ctx) {
  const ProjectContextImpl *impl = (const ProjectContextImpl *)ctx;
  return impl ? impl->num_files : 0;
}

// Get the file path at a specified index
const char *project_context_get_file_path(const ProjectContext *ctx, size_t index) {
  const ProjectContextImpl *impl = (const ProjectContextImpl *)ctx;
  if (!impl || index >= impl->num_files) {
    return NULL;
  }
  return impl->file_paths[index];
}

// Get the language type at a specified index
LanguageType project_context_get_language(const ProjectContext *ctx, size_t index) {
  const ProjectContextImpl *impl = (const ProjectContextImpl *)ctx;
  if (!impl || index >= impl->num_files) {
    return LANG_UNKNOWN;
  }
  return impl->languages[index];
}
