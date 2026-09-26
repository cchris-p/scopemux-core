/**
 * @file incremental_index.c
 * @brief Incremental file update for ProjectContext (WI-018)
 *
 * Provides content-hash-based incremental updates so a file that has not
 * changed is not re-parsed, and a changed or removed file invalidates the
 * derived IR / InfoBlock / search caches while leaving durable plan nodes
 * untouched. This is the in-memory core the filesystem watcher drives; the OS
 * event source is a separate follow-up slice.
 */

#include "dependency_management.h"
#include "file_management.h"
#include "project_context_internal.h"
#include "project_utils.h"
#include "scopemux/language.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol_registration.h"
#include "scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief FNV-1a 64-bit hash of a byte buffer.
 *
 * Deterministic and dependency-free; used only to detect content changes, not
 * for security. (A faster xxh3 can replace this behind the same API later.)
 */
uint64_t project_context_hash_content(const void *data, size_t length) {
  const unsigned char *bytes = (const unsigned char *)data;
  uint64_t hash = 1469598103934665603ULL; /* FNV offset basis */

  if (!data) {
    return 0;
  }

  for (size_t i = 0; i < length; i++) {
    hash ^= (uint64_t)bytes[i];
    hash *= 1099511628211ULL; /* FNV prime */
  }

  return hash;
}

/**
 * @brief Index of a parsed file context by normalized path, or -1.
 */
static int incremental_find_parsed_index(const ProjectContext *project,
                                         const char *normalized_path) {
  if (!project || !normalized_path) {
    return -1;
  }

  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    if (ctx && ctx->filename && strcmp(ctx->filename, normalized_path) == 0) {
      return (int)i;
    }
  }

  return -1;
}

/**
 * @brief Hash recorded for a parsed context, computed lazily from source if unset.
 */
static uint64_t incremental_stored_hash(ParserContext *ctx) {
  if (!ctx) {
    return 0;
  }

  if (ctx->content_hash == 0 && ctx->source_code) {
    ctx->content_hash =
        project_context_hash_content(ctx->source_code, ctx->source_code_length);
  }

  return ctx->content_hash;
}

bool project_file_is_unchanged(ProjectContext *project, const char *filepath, const char *content,
                               size_t content_length) {
  char normalized[1024];
  int index;
  ParserContext *ctx;
  uint64_t stored;
  uint64_t incoming;

  if (!project || !filepath || (!content && content_length > 0)) {
    return false;
  }

  if (!normalize_file_path(project->root_directory, filepath, normalized, sizeof(normalized))) {
    return false;
  }

  index = incremental_find_parsed_index(project, normalized);
  if (index < 0) {
    return false;
  }

  ctx = project->file_contexts[index];
  stored = incremental_stored_hash(ctx);
  incoming = project_context_hash_content(content, content_length);

  return stored == incoming;
}

bool project_update_file_from_string(ProjectContext *project, const char *filepath,
                                     const char *content, size_t content_length, Language language,
                                     bool *out_changed) {
  char normalized[1024];
  int index;
  ParserContext *old_ctx;
  ParserContext *ctx;
  uint64_t new_hash;

  if (out_changed) {
    *out_changed = false;
  }

  if (!project || !filepath || (!content && content_length > 0)) {
    return false;
  }

  if (!normalize_file_path(project->root_directory, filepath, normalized, sizeof(normalized))) {
    project_set_error(project, PROJECT_ERROR_INVALID_PATH, "Failed to normalize file path");
    return false;
  }

  new_hash = project_context_hash_content(content, content_length);
  index = incremental_find_parsed_index(project, normalized);
  old_ctx = (index >= 0) ? project->file_contexts[index] : NULL;

  // No-op gate: identical content means nothing to recompute.
  if (old_ctx && incremental_stored_hash(old_ctx) == new_hash) {
    return true;
  }

  if (language == LANG_UNKNOWN) {
    language = language_detect_from_extension(normalized);
  }
  if (language == LANG_UNKNOWN) {
    project_set_error(project, PROJECT_ERROR_UNKNOWN_LANGUAGE, "Unknown language for file");
    return false;
  }

  // Changed content: drop the old symbols, detach the old context from the
  // array, and reparse into a replacement. Dependents are repointed at the
  // replacement before the old context is freed so no dangling edges remain.
  if (old_ctx) {
    if (project->symbol_table) {
      symbol_table_remove_by_file(project->symbol_table, normalized);
    }
    project->file_contexts[index] = NULL;
  }

  ctx = parser_init();
  if (!ctx) {
    if (old_ctx) {
      project_context_scrub_dependency_target(project, old_ctx);
      parser_context_free(old_ctx);
    }
    project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate parser context");
    return false;
  }

  if (!parser_parse_string(ctx, content, content_length, normalized, language)) {
    parser_free(ctx);
    if (old_ctx) {
      project_context_scrub_dependency_target(project, old_ctx);
      parser_context_free(old_ctx);
    }
    project_set_error(project, PROJECT_ERROR_IO, "Failed to parse file");
    return false;
  }
  ctx->content_hash = new_hash;

  if (old_ctx) {
    project_context_repoint_dependency_target(project, old_ctx, ctx);
    parser_context_free(old_ctx);
  }

  if (index >= 0) {
    project->file_contexts[index] = ctx;
  } else {
    if (project->num_files >= project->files_capacity) {
      size_t new_capacity = project->files_capacity > 0 ? project->files_capacity * 2 : 8;
      ParserContext **new_contexts = (ParserContext **)realloc(
          project->file_contexts, new_capacity * sizeof(ParserContext *));
      if (!new_contexts) {
        parser_context_free(ctx);
        project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to resize file contexts array");
        return false;
      }
      project->file_contexts = new_contexts;
      project->files_capacity = new_capacity;
    }
    project->file_contexts[project->num_files++] = ctx;
    add_discovered_file(project, normalized);
  }

  extract_and_process_includes(project, ctx, normalized);
  register_file_symbols(project, ctx, normalized);

  // Derived state is now stale; durable plan nodes are untouched.
  project_context_clear_ir(project);

  if (out_changed) {
    *out_changed = true;
  }
  return true;
}

bool project_update_file(ProjectContext *project, const char *filepath, Language language,
                         bool *out_changed) {
  FILE *file;
  char *buffer;
  long size;
  size_t read_count;
  bool result;

  if (out_changed) {
    *out_changed = false;
  }

  if (!project || !filepath) {
    return false;
  }

  file = fopen(filepath, "rb");
  if (!file) {
    project_set_error(project, PROJECT_ERROR_IO, "Failed to open file for update");
    return false;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    project_set_error(project, PROJECT_ERROR_IO, "Failed to seek file");
    return false;
  }
  size = ftell(file);
  if (size < 0) {
    fclose(file);
    project_set_error(project, PROJECT_ERROR_IO, "Failed to size file");
    return false;
  }
  rewind(file);

  buffer = (char *)malloc((size_t)size + 1);
  if (!buffer) {
    fclose(file);
    project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate file buffer");
    return false;
  }

  read_count = fread(buffer, 1, (size_t)size, file);
  fclose(file);
  buffer[read_count] = '\0';

  result = project_update_file_from_string(project, filepath, buffer, read_count, language,
                                           out_changed);
  free(buffer);
  return result;
}
