/**
 * @file dependency_management.c
 * @brief Dependency tracking and include/import resolution for ProjectContext
 *
 * Handles extraction and processing of includes, imports, and other inter-file dependencies.
 * This module is responsible for discovering and tracking file relationships.
 */

#include "../src/parser/project_context/file_management.h" // for normalize_file_path
#include "../src/parser/project_context/project_utils.h"   // for project_set_error
#include "project_context_internal.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol_registration.h" // for register_file_symbols, project_context_extract_symbols_impl
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Static prototype for internal helper
static void process_node_for_includes(ProjectContext *project, ASTNode *node, const char *filepath,
                                      Language language);

/**
 * Extract includes/imports from a parsed file and add them to project
 *
 * Traverses the AST of a parsed file to find include directives, import statements,
 * and other dependency declarations. For each dependency found, it adds the
 * corresponding file to the project for parsing.
 *
 * @param project The ProjectContext
 * @param ctx The ParserContext containing the parsed AST
 * @param filepath The file path (used for resolving relative includes)
 */
void extract_and_process_includes(ProjectContext *project, ParserContext *ctx,
                                  const char *filepath) {
  if (!project || !ctx || !filepath || !project->config.follow_includes) {
    return;
  }

  // Increment include depth for this processing round
  project->current_include_depth++;

  // Start with the root nodes
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    process_node_for_includes(project, ctx->all_ast_nodes[i], filepath, ctx->language);
  }

  // Restore include depth when done with this file
  project->current_include_depth--;
}

/**
 * Helper function to recursively process an AST node for includes/imports
 *
 * @param project The ProjectContext
 * @param node The ASTNode to process
 * @param filepath The file path of the current file
 * @param language The language of the current file
 */
static void process_node_for_includes(ProjectContext *project, ASTNode *node, const char *filepath,
                                      Language language) {
  if (!project || !node) {
    return;
  }

  // Process this node for includes based on language and node type
  char *include_path = NULL;
  bool is_system_include = false;

  if (node->type == NODE_INCLUDE || node->type == NODE_IMPORT) {
    switch (language) {
    case LANG_C:
    case LANG_CPP:
      // Check for #include directive
      if (node->raw_content) {
        // Look for include with quotes first (local include)
        char *start = strchr(node->raw_content, '"');
        if (start) {
          start++;
          char *end = strchr(start, '"');
          if (end) {
            size_t len = end - start;
            include_path = malloc(len + 1);
            if (include_path) {
              strncpy(include_path, start, len);
              include_path[len] = '\0';
            }
          }
        } else {
          // Look for include with angle brackets (system include)
          start = strchr(node->raw_content, '<');
          if (start) {
            start++;
            char *end = strchr(start, '>');
            if (end) {
              size_t len = end - start;
              include_path = malloc(len + 1);
              if (include_path) {
                strncpy(include_path, start, len);
                include_path[len] = '\0';
                is_system_include = true;
              }
            }
          }
        }
      }
      break;

    case LANG_PYTHON:
      // Look for import statements
      if (node->type == NODE_IMPORT && node->name) {
        include_path = strdup(node->name);
      }
      break;

    case LANG_JAVASCRIPT:
    case LANG_TYPESCRIPT:
      // Look for import/require statements
      if (node->type == NODE_IMPORT && node->raw_content) {
        // Parse import statements like: import X from 'path'
        char *start = strstr(node->raw_content, "from");
        if (start) {
          start = strchr(start, '\'');
          if (!start) {
            start = strchr(node->raw_content, '"');
          }
          if (start) {
            start++;
            char *end = strchr(start, start[-1]); // matching quote
            if (end) {
              size_t len = end - start;
              include_path = malloc(len + 1);
              if (include_path) {
                strncpy(include_path, start, len);
                include_path[len] = '\0';
              }
            }
          }
        } else {
          // Try to handle require('path') pattern
          start = strstr(node->raw_content, "require");
          if (start) {
            start = strchr(start, '\'');
            if (!start)
              start = strchr(node->raw_content, '"');
            if (start) {
              start++;
              char *end = strchr(start, start[-1]); // matching quote
              if (end) {
                size_t len = end - start;
                include_path = malloc(len + 1);
                if (include_path) {
                  strncpy(include_path, start, len);
                  include_path[len] = '\0';
                }
              }
            }
          }
        }
      }
      break;

    default:
      // Unknown language, can't extract includes
      break;
    }

    // Process the include if found
    if (include_path) {
      if (is_system_include && !project->config.parse_headers) {
        // Skip system includes if not configured to parse them
        free(include_path);
      } else {
        // Determine the full path
        char full_path[1024];
        if (is_system_include) {
          // For system includes, try standard include paths
          // This is very simplified and should be expanded based on platform
          snprintf(full_path, sizeof(full_path), "/usr/include/%s", include_path);
        } else {
          // For local includes, resolve relative to current file
          char *dir_path = strdup(filepath);
          char *last_slash = strrchr(dir_path, '/');
          if (last_slash) {
            *last_slash = '\0';
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, include_path);
          } else {
            // No directory part, use project root
            snprintf(full_path, sizeof(full_path), "%s/%s", project->root_directory, include_path);
          }
          free(dir_path);
        }

        // Add the file to the project for parsing
        project_add_file_impl(project, full_path, language);
        free(include_path);
      }
    }
  }

  // Recursively process children
  for (size_t i = 0; i < node->num_children; i++) {
    process_node_for_includes(project, node->children[i], filepath, language);
  }
}

/**
 * Parse all files in the project (Implementation)
 *
 * Parses all discovered files that haven't been parsed yet.
 * For each file, it extracts includes and registers symbols.
 *
 * @param project The ProjectContext
 * @return true if successful, false otherwise
 */
bool project_parse_all_files_impl(ProjectContext *project) {
  if (!project) {
    return false;
  }

  log_info("Parsing %zu discovered files", project->num_discovered);

  // Keep track of how many files we've parsed so far in this iteration
  size_t initial_file_count = project->num_files;
  size_t files_parsed = 0;

  // Parse all discovered files that haven't been parsed yet
  for (size_t i = 0; i < project->num_discovered; i++) {
    const char *filepath = project->discovered_files[i];

    // Skip if already parsed
    if (is_file_parsed(project, filepath)) {
      continue;
    }

    // Check file max limit
    if (project->config.max_files > 0 && project->num_files >= project->config.max_files) {
      log_warning("Reached maximum file limit (%zu)", project->config.max_files);
      break;
    }

    log_info("Parsing file: %s", filepath);

    // Try to detect the language if not specified
    Language lang = LANG_UNKNOWN;

    // Simple language detection based on file extension
    const char *dot = strrchr(filepath, '.');
    if (dot) {
      dot++; // Skip the dot
      if (strcmp(dot, "c") == 0) {
        lang = LANG_C;
      } else if (strcmp(dot, "h") == 0) {
        lang = LANG_C; // Could be C or C++, assuming C for now
      } else if (strcmp(dot, "cpp") == 0 || strcmp(dot, "cc") == 0) {
        lang = LANG_CPP;
      } else if (strcmp(dot, "hpp") == 0 || strcmp(dot, "hh") == 0) {
        lang = LANG_CPP;
      } else if (strcmp(dot, "py") == 0) {
        lang = LANG_PYTHON;
      } else if (strcmp(dot, "js") == 0) {
        lang = LANG_JAVASCRIPT;
      } else if (strcmp(dot, "ts") == 0) {
        lang = LANG_TYPESCRIPT;
      }
    }

    if (lang == LANG_UNKNOWN) {
      log_warning("Unknown language for file: %s", filepath);
      continue;
    }

    // Allocate a new parser context
    ParserContext *ctx = parser_init();
    if (!ctx) {
      log_error("Failed to create parser context for file: %s", filepath);
      project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate parser context");
      continue;
    }

    // Parse the file
    if (!parser_parse_file(ctx, filepath, lang)) {
      log_error("Failed to parse file: %s", filepath);
      parser_free(ctx);
      continue;
    }

    // Add to parsed files array
    if (project->num_files >= project->files_capacity) {
      size_t new_capacity = project->files_capacity * 2;
      ParserContext **new_contexts =
          (ParserContext **)realloc(project->file_contexts, new_capacity * sizeof(ParserContext *));
      if (!new_contexts) {
        log_error("Failed to resize file contexts array");
        parser_free(ctx);
        project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to resize file contexts array");
        continue;
      }
      project->file_contexts = new_contexts;
      project->files_capacity = new_capacity;
    }

    project->file_contexts[project->num_files++] = ctx;
    files_parsed++;

    // Process includes in this file
    extract_and_process_includes(project, ctx, filepath);

    // Register symbols from this file in the global symbol table
    register_file_symbols(project, ctx, filepath);
  }

  log_info("Parsed %zu new files", files_parsed);

  // Check if we discovered new files to parse in this iteration
  if (project->num_files > initial_file_count) {
    // Recursively call to parse newly discovered files
    log_info("Parsing newly discovered files");
    return project_parse_all_files_impl(project);
  }

  return true;
}

/**
 * Add a dependency between two files (Implementation)
 *
 * This function establishes a dependency relationship between two files
 * in the project. It ensures both files are valid and tracked in the project.
 *
 * @param project The ProjectContext
 * @param source_file The source file that depends on the target
 * @param target_file The target file that is depended upon
 * @return true if successful, false otherwise
 */
bool project_add_dependency_impl(ProjectContext *project, const char *source_file,
                                 const char *target_file) {
  if (!project || !source_file || !target_file) {
    return false;
  }

  // Normalize both file paths
  char normalized_source[1024];
  char normalized_target[1024];

  if (!normalize_file_path(project->root_directory, source_file, normalized_source,
                           sizeof(normalized_source)) ||
      !normalize_file_path(project->root_directory, target_file, normalized_target,
                           sizeof(normalized_target))) {
    project_set_error(project, PROJECT_ERROR_INVALID_PATH, "Failed to normalize file paths");
    return false;
  }

  // Get the parser contexts for both files
  ParserContext *source_ctx = project_get_file_context_impl(project, normalized_source);
  ParserContext *target_ctx = project_get_file_context_impl(project, normalized_target);

  // If either file is not in the project, try to add them
  if (!source_ctx) {
    log_debug("Source file not in project, attempting to add: %s", normalized_source);

    // Try to detect language from extension
    Language lang = LANG_UNKNOWN;
    const char *dot = strrchr(normalized_source, '.');
    if (dot) {
      dot++; // Skip the dot
      if (strcmp(dot, "c") == 0)
        lang = LANG_C;
      else if (strcmp(dot, "h") == 0)
        lang = LANG_C;
      else if (strcmp(dot, "cpp") == 0 || strcmp(dot, "cc") == 0)
        lang = LANG_CPP;
      else if (strcmp(dot, "hpp") == 0 || strcmp(dot, "hh") == 0)
        lang = LANG_CPP;
      else if (strcmp(dot, "py") == 0)
        lang = LANG_PYTHON;
      else if (strcmp(dot, "js") == 0)
        lang = LANG_JAVASCRIPT;
      else if (strcmp(dot, "ts") == 0)
        lang = LANG_TYPESCRIPT;
    }

    if (lang == LANG_UNKNOWN) {
      log_error("Cannot determine language for source file: %s", normalized_source);
      project_set_error(project, PROJECT_ERROR_UNKNOWN_LANGUAGE,
                        "Unknown language for source file");
      return false;
    }

    if (!project_add_file_impl(project, normalized_source, lang)) {
      log_error("Failed to add source file to project: %s", normalized_source);
      return false;
    }

    source_ctx = project_get_file_context_impl(project, normalized_source);
    if (!source_ctx) {
      log_error("Failed to get parser context for source file after adding: %s", normalized_source);
      return false;
    }
  }

  if (!target_ctx) {
    log_debug("Target file not in project, attempting to add: %s", normalized_target);

    // Try to detect language from extension
    Language lang = LANG_UNKNOWN;
    const char *dot = strrchr(normalized_target, '.');
    if (dot) {
      dot++; // Skip the dot
      if (strcmp(dot, "c") == 0)
        lang = LANG_C;
      else if (strcmp(dot, "h") == 0)
        lang = LANG_C;
      else if (strcmp(dot, "cpp") == 0 || strcmp(dot, "cc") == 0)
        lang = LANG_CPP;
      else if (strcmp(dot, "hpp") == 0 || strcmp(dot, "hh") == 0)
        lang = LANG_CPP;
      else if (strcmp(dot, "py") == 0)
        lang = LANG_PYTHON;
      else if (strcmp(dot, "js") == 0)
        lang = LANG_JAVASCRIPT;
      else if (strcmp(dot, "ts") == 0)
        lang = LANG_TYPESCRIPT;
    }

    if (lang == LANG_UNKNOWN) {
      log_error("Cannot determine language for target file: %s", normalized_target);
      project_set_error(project, PROJECT_ERROR_UNKNOWN_LANGUAGE,
                        "Unknown language for target file");
      return false;
    }

    if (!project_add_file_impl(project, normalized_target, lang)) {
      log_error("Failed to add target file to project: %s", normalized_target);
      return false;
    }

    target_ctx = project_get_file_context_impl(project, normalized_target);
    if (!target_ctx) {
      log_error("Failed to get parser context for target file after adding: %s", normalized_target);
      return false;
    }
  }

  // Add dependency relationship to source file's context
  if (!parser_context_add_dependency(source_ctx, target_ctx)) {
    log_error("Failed to add dependency relationship between %s and %s", normalized_source,
              normalized_target);
    return false;
  }

  log_debug("Added dependency: %s -> %s", normalized_source, normalized_target);
  return true;
}

/**
 * Get dependencies for a file (Implementation)
 *
 * This function retrieves the list of files that the specified file depends on.
 * It returns the number of dependencies found and populates the out_dependencies
 * parameter with an array of file paths.
 *
 * @param project The ProjectContext
 * @param filepath The file to get dependencies for
 * @param out_dependencies Output parameter for the array of dependency file paths
 * @return The number of dependencies found
 */
size_t project_get_dependencies_impl(const ProjectContext *project, const char *filepath,
                                     char ***out_dependencies) {
  if (!project || !filepath) {
    if (out_dependencies) {
      *out_dependencies = NULL;
    }
    return 0;
  }

  // Normalize the file path
  char normalized_path[1024];
  if (!normalize_file_path(project->root_directory, filepath, normalized_path,
                           sizeof(normalized_path))) {
    // Cannot use project_set_error with const ProjectContext
    log_error("Failed to normalize file path");
    if (out_dependencies) {
      *out_dependencies = NULL;
    }
    return 0;
  }

  // Find the file in the parsed files
  ParserContext *ctx = project_get_file_context_impl(project, normalized_path);
  if (!ctx) {
    // File not found in project
    if (out_dependencies) {
      *out_dependencies = NULL;
    }
    return 0;
  }

  // Count dependencies
  size_t num_deps = ctx->num_dependencies;
  if (num_deps == 0 || !out_dependencies) {
    if (out_dependencies) {
      *out_dependencies = NULL;
    }
    return num_deps;
  }

  // Allocate array for dependency file paths
  char **deps = (char **)malloc(num_deps * sizeof(char *));
  if (!deps) {
    // Cannot use project_set_error with const ProjectContext
    log_error("Failed to allocate memory for dependencies");
    *out_dependencies = NULL;
    return 0;
  }

  // Populate the array with dependency file paths
  for (size_t i = 0; i < num_deps; i++) {
    if (ctx->dependencies[i]->filename) {
      deps[i] = strdup(ctx->dependencies[i]->filename);
      if (!deps[i]) {
        // Memory allocation failed, clean up
        for (size_t j = 0; j < i; j++) {
          free(deps[j]);
        }
        free(deps);
        *out_dependencies = NULL;
        return 0;
      }
    } else {
      deps[i] = strdup("(unnamed)");
    }
  }

  *out_dependencies = deps;
  return num_deps;
}
