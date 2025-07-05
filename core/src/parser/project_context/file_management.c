/**
 * @file file_management.c
 * @brief File management functionality for ProjectContext
 *
 * Handles file discovery, tracking, and path normalization.
 */

#include "project_context_internal.h"
#include "project_utils.h" // for project_set_error
#include "scopemux/logging.h"
#include "scopemux/project_context.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Normalize a file path relative to the project root
 *
 * Converts a potentially relative path to an absolute path within the project.
 * Handles path normalization including removing ".." and "." components.
 *
 * @param project_root The project root directory
 * @param filepath The file path to normalize
 * @param out_path Buffer to store the normalized path
 * @param out_size Size of the output buffer
 * @return true if successful, false otherwise
 */
bool normalize_file_path(const char *project_root, const char *filepath, char *out_path,
                         size_t out_size) {
  if (!project_root || !filepath || !out_path || out_size == 0) {
    return false;
  }

  // Check if the path is already absolute
  if (filepath[0] == '/') {
    // Just copy it directly
    strncpy(out_path, filepath, out_size);
    out_path[out_size - 1] = '\0';
    return true;
  }

  // Create absolute path by combining project root and relative path
  snprintf(out_path, out_size, "%s/%s", project_root, filepath);

  // TODO: Implement path normalization to handle ".." and "." components
  // This is a simplified implementation that doesn't handle complex cases

  return true;
}

/**
 * Add a file to the discovered files list
 *
 * @param project The ProjectContext
 * @param filepath The file path to add
 * @return true if successful, false otherwise
 */
bool add_discovered_file(ProjectContext *project, const char *filepath) {
  if (!project || !filepath) {
    return false;
  }

  // Check if the file is already discovered
  for (size_t i = 0; i < project->num_discovered; i++) {
    if (strcmp(project->discovered_files[i], filepath) == 0) {
      return true; // Already discovered
    }
  }

  // Check if we need to resize the discovered files array
  if (project->num_discovered >= project->discovered_capacity) {
    size_t new_capacity = project->discovered_capacity * 2;
    char **new_files = (char **)realloc(project->discovered_files, new_capacity * sizeof(char *));
    if (!new_files) {
      project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to resize discovered files array");
      return false;
    }
    project->discovered_files = new_files;
    project->discovered_capacity = new_capacity;
  }

  // Add the file to the list
  project->discovered_files[project->num_discovered] = strdup(filepath);
  if (!project->discovered_files[project->num_discovered]) {
    project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate memory for file path");
    return false;
  }
  project->num_discovered++;

  return true;
}

/**
 * Check if a file is already parsed
 *
 * @param project The ProjectContext
 * @param filepath The file path to check
 * @return true if the file is already parsed, false otherwise
 */
bool is_file_parsed(const ProjectContext *project, const char *filepath) {
  if (!project || !filepath) {
    return false;
  }

  for (size_t i = 0; i < project->num_files; i++) {
    if (project->file_contexts[i] && project->file_contexts[i]->filename &&
        strcmp(project->file_contexts[i]->filename, filepath) == 0) {
      return true;
    }
  }

  return false;
}

/**
 * Add a file to the project for parsing (Implementation)
 *
 * @param project The ProjectContext
 * @param filepath The file path to add
 * @param language The language of the file
 * @return true if successful, false otherwise
 */
bool project_add_file_impl(ProjectContext *project, const char *filepath, Language language) {
  if (!project || !filepath) {
    return false;
  }

  // Check for max_files limit
  if (project->config.max_files > 0 && project->num_files >= project->config.max_files) {
    project_set_error(project, PROJECT_ERROR_TOO_MANY_FILES, "Maximum number of files reached");
    return false;
  }

  // Check for max_include_depth limit
  if (project->current_include_depth > project->config.max_include_depth) {
    project_set_error(project, PROJECT_ERROR_INCLUDE_DEPTH, "Maximum include depth reached");
    return false;
  }

  // Normalize the file path
  char normalized_path[1024];
  if (!normalize_file_path(project->root_directory, filepath, normalized_path,
                           sizeof(normalized_path))) {
    project_set_error(project, PROJECT_ERROR_INVALID_PATH, "Failed to normalize file path");
    return false;
  }

  // Check if the file is already in the discovered list
  if (!add_discovered_file(project, normalized_path)) {
    // Error already set in add_discovered_file
    return false;
  }

  // Don't re-parse files that are already parsed
  if (is_file_parsed(project, normalized_path)) {
    return true;
  }

  // The file is added to the discovered list but not parsed yet
  // It will be parsed later when project_parse_all_files is called

  return true;
}

/**
 * Add all files in a directory to the project (Implementation)
 *
 * Recursively discovers files in the specified directory that match the given extensions.
 *
 * @param project The ProjectContext
 * @param dirpath The directory path
 * @param extensions Array of file extensions to include (NULL terminated)
 * @param recursive Whether to scan subdirectories recursively
 * @return The number of files added
 */
size_t project_add_directory_impl(ProjectContext *project, const char *dirpath,
                                  const char **extensions, bool recursive) {
  if (!project || !dirpath) {
    return 0;
  }

  // Normalize the directory path
  char normalized_dir[1024];
  if (!normalize_file_path(project->root_directory, dirpath, normalized_dir,
                           sizeof(normalized_dir))) {
    project_set_error(project, PROJECT_ERROR_INVALID_PATH, "Failed to normalize directory path");
    return 0;
  }

  // Open the directory
  DIR *dir = opendir(normalized_dir);
  if (!dir) {
    project_set_error(project, PROJECT_ERROR_IO, "Failed to open directory");
    return 0;
  }

  size_t files_added = 0;
  struct dirent *entry;

  // Iterate through directory entries
  while ((entry = readdir(dir)) != NULL) {
    // Skip "." and ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Prepare buffer for full path
    char full_path[1024];

    // Check if path would be too long
    size_t dir_len = strlen(normalized_dir);
    size_t name_len = strlen(entry->d_name);
    if (dir_len + name_len + 2 > sizeof(full_path)) { // +2 for '/' and null terminator
      log_warning("Path too long, skipping: %s/%s", normalized_dir, entry->d_name);
      continue;
    }

    // Construct the full path safely
    int written = snprintf(full_path, sizeof(full_path), "%s/%s", normalized_dir, entry->d_name);
    if (written < 0 || (size_t)written >= sizeof(full_path)) {
      log_warning("Path truncated, skipping: %s/%s", normalized_dir, entry->d_name);
      continue;
    }

    // Get file status
    struct stat file_stat;
    if (stat(full_path, &file_stat) < 0) {
      continue; // Skip if can't stat
    }

    if (S_ISDIR(file_stat.st_mode)) {
      // It's a directory, recurse if enabled
      if (recursive) {
        files_added += project_add_directory(project, full_path, extensions, true);
      }
    } else if (S_ISREG(file_stat.st_mode)) {
      // It's a regular file, check extension
      bool add_file = false;

      if (!extensions) {
        // No extensions specified, add all files
        add_file = true;
      } else {
        // Check if the file has one of the specified extensions
        char *dot = strrchr(entry->d_name, '.');
        if (dot) {
          dot++; // Skip the dot
          const char **ext = extensions;
          while (*ext) {
            if (strcasecmp(dot, *ext) == 0) {
              add_file = true;
              break;
            }
            ext++;
          }
        }
      }

      if (add_file) {
        // Infer the language from the file extension
        Language lang = LANG_UNKNOWN;
        char *dot = strrchr(entry->d_name, '.');
        if (dot) {
          dot++; // Skip the dot
          if (strcasecmp(dot, "c") == 0) {
            lang = LANG_C;
          } else if (strcasecmp(dot, "h") == 0) {
            lang = LANG_C;
          } else if (strcasecmp(dot, "cpp") == 0 || strcasecmp(dot, "cc") == 0) {
            lang = LANG_CPP;
          } else if (strcasecmp(dot, "hpp") == 0 || strcasecmp(dot, "hh") == 0) {
            lang = LANG_CPP;
          } else if (strcasecmp(dot, "py") == 0) {
            lang = LANG_PYTHON;
          } else if (strcasecmp(dot, "js") == 0) {
            lang = LANG_JAVASCRIPT;
          } else if (strcasecmp(dot, "ts") == 0) {
            lang = LANG_TYPESCRIPT;
          }
        }

        if (project_add_file(project, full_path, lang)) {
          files_added++;
        }
      }
    }
  }

  closedir(dir);
  return files_added;
}

/**
 * Get a file context by filename (Implementation)
 *
 * @param project The ProjectContext
 * @param filepath The file path
 * @return The ParserContext for the file, or NULL if not found
 */
ParserContext *project_get_file_context_impl(const ProjectContext *project, const char *filepath) {
  if (!project || !filepath) {
    return NULL;
  }

  // Normalize the file path
  char normalized_path[1024];
  if (!normalize_file_path(project->root_directory, filepath, normalized_path,
                           sizeof(normalized_path))) {
    return NULL;
  }

  // Find the file in the parsed files
  for (size_t i = 0; i < project->num_files; i++) {
    if (project->file_contexts[i] && project->file_contexts[i]->filename &&
        strcmp(project->file_contexts[i]->filename, normalized_path) == 0) {
      return project->file_contexts[i];
    }
  }

  return NULL;
}

/**
 * Remove a file from the project (Implementation)
 *
 * @param project The ProjectContext
 * @param filepath The file path to remove
 * @return true if successful, false otherwise
 */
bool project_remove_file_impl(ProjectContext *project, const char *filepath) {
  if (!project || !filepath) {
    return false;
  }

  // Normalize the file path
  char normalized_path[1024];
  if (!normalize_file_path(project->root_directory, filepath, normalized_path,
                           sizeof(normalized_path))) {
    project_set_error(project, PROJECT_ERROR_INVALID_PATH, "Failed to normalize file path");
    return false;
  }

  // Find the file in the parsed files
  int found_index = -1;
  for (size_t i = 0; i < project->num_files; i++) {
    if (project->file_contexts[i] && project->file_contexts[i]->filename &&
        strcmp(project->file_contexts[i]->filename, normalized_path) == 0) {
      found_index = (int)i;
      break;
    }
  }

  if (found_index < 0) {
    // File not found in project
    return false;
  }

  // Free the parser context
  parser_context_free(project->file_contexts[found_index]);
  project->file_contexts[found_index] = NULL;

  // Compact the array by shifting elements
  for (size_t i = found_index; i < project->num_files - 1; i++) {
    project->file_contexts[i] = project->file_contexts[i + 1];
  }
  project->num_files--;

  // Also remove from discovered files if present
  for (size_t i = 0; i < project->num_discovered; i++) {
    if (project->discovered_files[i] &&
        strcmp(project->discovered_files[i], normalized_path) == 0) {
      free(project->discovered_files[i]);

      // Shift remaining elements
      for (size_t j = i; j < project->num_discovered - 1; j++) {
        project->discovered_files[j] = project->discovered_files[j + 1];
      }
      project->num_discovered--;
      break;
    }
  }

  log_debug("Removed file from project: %s", normalized_path);
  return true;
}
