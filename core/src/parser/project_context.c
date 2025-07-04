/**
 * @file project_context.c
 * @brief Implementation of multi-file parsing and relationship management
 *
 * This is the main entry point for the ProjectContext functionality.
 * It delegates to specialized modules in the project_context/ directory
 * for different aspects of project management:
 *
 * - project_utils.c: Core lifecycle management functions
 * - file_management.c: File tracking and discovery functions
 * - symbol_management.c: Symbol management and reference resolution
 * - dependency_management.c: Dependency tracking and include/import resolution
 */

#include "scopemux/project_context.h"
#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Internal static helper prototypes
static bool normalize_file_path(const char *project_root, const char *filepath, char *out_path,
                                size_t out_size);
static bool add_discovered_file(ProjectContext *project, const char *filepath);
static bool is_file_parsed(const ProjectContext *project, const char *filepath);
static void register_file_symbols(ProjectContext *project, ParserContext *ctx,
                                  const char *filepath);
static void extract_and_process_includes(ProjectContext *project, ParserContext *ctx,
                                         const char *filepath);

/**
 * Remove a file from the project
 * Delegates to implementation in file_management.c
 */
bool project_remove_file(ProjectContext *project, const char *filepath) {
  extern bool project_remove_file_impl(ProjectContext * project, const char *filepath);
  return project_remove_file_impl(project, filepath);
}

/**
 * Public API: Remove a file from the project
 * Delegates to implementation
 */
bool project_context_remove_file(ProjectContext *project, const char *filepath) {
  return project_remove_file(project, filepath);
}

/**
 * Add a dependency between two files
 * Delegates to implementation in dependency_management.c
 */
bool project_add_dependency(ProjectContext *project, const char *source_file,
                            const char *target_file) {
  extern bool project_add_dependency_impl(ProjectContext * project, const char *source_file,
                                          const char *target_file);
  return project_add_dependency_impl(project, source_file, target_file);
}

/**
 * Public API: Add a dependency between two files
 * Delegates to implementation
 */
bool project_context_add_dependency(ProjectContext *project, const char *source_file,
                                    const char *target_file) {
  return project_add_dependency(project, source_file, target_file);
}

/**
 * Get dependencies for a file
 * Delegates to implementation in dependency_management.c
 */
size_t project_get_dependencies(const ProjectContext *project, const char *filepath,
                                char ***out_dependencies) {
  extern size_t project_get_dependencies_impl(const ProjectContext *project, const char *filepath,
                                              char ***out_dependencies);
  return project_get_dependencies_impl(project, filepath, out_dependencies);
}

/**
 * Public API: Get dependencies for a file
 * Delegates to implementation
 */
size_t project_context_get_dependencies(const ProjectContext *project, const char *filepath,
                                        char ***out_dependencies) {
  return project_get_dependencies(project, filepath, out_dependencies);
}

/**
 * Extract symbols from parsed files
 * Delegates to implementation in symbol_management.c
 */
bool project_extract_symbols(ProjectContext *project, ParserContext *parser,
                             GlobalSymbolTable *symbol_table) {
  extern bool project_extract_symbols_impl(ProjectContext * project, ParserContext * parser,
                                           GlobalSymbolTable * symbol_table);
  return project_extract_symbols_impl(project, parser, symbol_table);
}

/**
 * Public API: Extract symbols from parsed files
 * Delegates to implementation
 */
bool project_context_extract_symbols(ProjectContext *project, ParserContext *parser,
                                     GlobalSymbolTable *symbol_table) {
  return project_extract_symbols(project, parser, symbol_table);
}

// External functions defined in the specialized modules
// From project_core.c
extern ProjectContext *project_context_create(const char *root_directory);
extern void project_context_free(ProjectContext *project);
extern void project_context_set_config(ProjectContext *project, const ProjectConfig *config);
extern void project_set_error(ProjectContext *project, int code, const char *message);
extern const char *project_get_error(const ProjectContext *project, int *out_code);
extern void project_get_stats(const ProjectContext *project, size_t *out_total_files,
                              size_t *out_total_symbols, size_t *out_total_references,
                              size_t *out_unresolved);

// From file_management.c
// static/internal helpers: normalize_file_path, add_discovered_file, is_file_parsed are defined
// below as static and not used outside this file.
extern bool project_add_file_impl(ProjectContext *project, const char *filepath, Language language);
extern size_t project_add_directory_impl(ProjectContext *project, const char *dirpath,
                                         const char **extensions, bool recursive);
extern ParserContext *project_get_file_context_impl(const ProjectContext *project,
                                                    const char *filepath);
extern bool project_remove_file_impl(ProjectContext *project, const char *filepath);
extern bool project_add_dependency_impl(ProjectContext *project, const char *source_file,
                                        const char *target_file);
extern size_t project_get_dependencies_impl(const ProjectContext *project, const char *filepath,
                                            char ***out_dependencies);

// From symbol_management.c
// static/internal helper: register_file_symbols is defined below as static and not used outside
// this file.
extern const ASTNode *project_get_symbol_impl(const ProjectContext *project,
                                              const char *qualified_name);
extern size_t project_get_symbols_by_type_impl(const ProjectContext *project, ASTNodeType type,
                                               const ASTNode **out_nodes, size_t max_nodes);
extern size_t project_find_references_impl(const ProjectContext *project, const ASTNode *node,
                                           const ASTNode **out_references, size_t max_references);
extern bool project_resolve_references_impl(ProjectContext *project);
extern bool project_extract_symbols_impl(ProjectContext *project, ParserContext *parser,
                                         GlobalSymbolTable *symbol_table);

// From dependency_management.c
// static/internal helper: extract_and_process_includes is defined below as static and not used
// outside this file.
extern bool project_parse_all_files_impl(ProjectContext *project);

/**
 * Create a new project context
 * Delegates to implementation in project_utils.c
 */
ProjectContext *project_context_create(const char *root_directory) {
  extern ProjectContext *project_context_create_impl(const char *root_directory);
  return project_context_create_impl(root_directory);
}

/**
 * Free all resources associated with a project context
 * Delegates to implementation in project_utils.c
 */
void project_context_free(ProjectContext *project) {
  extern void project_context_free_impl(ProjectContext * project);
  project_context_free_impl(project);
}

/**
 * Set project configuration options
 * Delegates to implementation in project_utils.c
 */
void project_context_set_config(ProjectContext *project, const ProjectConfig *config) {
  extern void project_context_set_config_impl(ProjectContext * project,
                                              const ProjectConfig *config);
  project_context_set_config_impl(project, config);
}

/**
 * Add a file to the project for parsing
 * Delegates to implementation in file_management.c
 */
bool project_add_file(ProjectContext *project, const char *filepath, Language language) {
  extern bool project_add_file_impl(ProjectContext * project, const char *filepath,
                                    Language language);
  return project_add_file_impl(project, filepath, language);
}

/**
 * Public API: Add a file to the project for parsing
 * Delegates to implementation
 */
bool project_context_add_file(ProjectContext *project, const char *filepath, Language language) {
  return project_add_file(project, filepath, language);
}

/**
 * Public API: Add a dependency between two files
 * Delegates to implementation
 *
 * @param project The ProjectContext
 * @param source_file The source file that depends on the target
 * @param target_file The target file that is depended upon
 * @return true if successful, false otherwise
 */
bool project_context_add_dependency(ProjectContext *project, const char *source_file,
                                    const char *target_file) {
  extern bool project_add_dependency_impl(ProjectContext * project, const char *source_file,
                                          const char *target_file);
  return project_add_dependency_impl(project, source_file, target_file);
}

/**
 * Public API: Extract symbols from a parser context
 * Extracts symbols from a parser context and stores them in a symbol collection
 *
 * @param project The ProjectContext
 * @param ctx The ParserContext to extract symbols from
 * @param symbols The symbol collection to store extracted symbols in
 * @return true if successful, false otherwise
 */
bool project_context_extract_symbols(ProjectContext *project, ParserContext *ctx, void *symbols) {
  extern bool project_context_extract_symbols_impl(ProjectContext * project, ParserContext * ctx,
                                                   void *symbols);
  return project_context_extract_symbols_impl(project, ctx, symbols);
}

/**
 * Add all files in a directory to the project
 * Delegates to implementation in file_management.c
 */
size_t project_add_directory(ProjectContext *project, const char *dirpath, const char **extensions,
                             bool recursive) {
  extern size_t project_add_directory_impl(ProjectContext * project, const char *dirpath,
                                           const char **extensions, bool recursive);
  return project_add_directory_impl(project, dirpath, extensions, recursive);
}

/**
 * Public API: Add all files in a directory to the project
 * Delegates to implementation
 */
size_t project_context_add_directory(ProjectContext *project, const char *dirpath,
                                     const char **extensions, bool recursive) {
  return project_add_directory(project, dirpath, extensions, recursive);
}

/**
 * Parse all files in the project
 * Delegates to implementation in dependency_management.c
 */
bool project_parse_all_files(ProjectContext *project) {
  extern bool project_parse_all_files_impl(ProjectContext * project);
  return project_parse_all_files_impl(project);
}

/**
 * Public API: Parse all files in the project
 * Delegates to implementation
 */
bool project_context_parse_all_files(ProjectContext *project) {
  return project_parse_all_files(project);
}

/**
 * Resolve references across all files
 * Delegates to implementation in symbol_management.c
 */
bool project_resolve_references(ProjectContext *project) {
  extern bool project_resolve_references_impl(ProjectContext * project);
  return project_resolve_references_impl(project);
}

/**
 * Public API: Resolve references across all files
 * Delegates to implementation
 */
bool project_context_resolve_references(ProjectContext *project) {
  return project_resolve_references(project);
}

/**
 * Get a file context by filename
 * Delegates to implementation in file_management.c
 */
ParserContext *project_get_file_context(const ProjectContext *project, const char *filepath) {
  extern ParserContext *project_get_file_context_impl(const ProjectContext *project,
                                                      const char *filepath);
  return project_get_file_context_impl(project, filepath);
}

/**
 * Public API: Get a file context by filename
 * Delegates to implementation
 */
ParserContext *project_context_get_file_context(const ProjectContext *project,
                                                const char *filepath) {
  return project_get_file_context(project, filepath);
}

/**
 * Get a symbol by its qualified name
 * Delegates to implementation in symbol_management.c
 */
const ASTNode *project_get_symbol(const ProjectContext *project, const char *qualified_name) {
  extern const ASTNode *project_get_symbol_impl(const ProjectContext *project,
                                                const char *qualified_name);
  return project_get_symbol_impl(project, qualified_name);
}

/**
 * Public API: Get a symbol by its qualified name
 * Delegates to implementation
 */
const ASTNode *project_context_get_symbol(const ProjectContext *project,
                                          const char *qualified_name) {
  return project_get_symbol(project, qualified_name);
}

/**
 * Get all symbols of a specific type
 * Delegates to implementation in symbol_management.c
 */
size_t project_get_symbols_by_type(const ProjectContext *project, ASTNodeType type,
                                   const ASTNode **out_nodes, size_t max_nodes) {
  extern size_t project_get_symbols_by_type_impl(const ProjectContext *project, ASTNodeType type,
                                                 const ASTNode **out_nodes, size_t max_nodes);
  return project_get_symbols_by_type_impl(project, type, out_nodes, max_nodes);
}

/**
 * Public API: Get all symbols of a specific type
 * Delegates to implementation
 */
size_t project_context_get_symbols_by_type(const ProjectContext *project, ASTNodeType type,
                                           const ASTNode **out_nodes, size_t max_nodes) {
  return project_get_symbols_by_type(project, type, out_nodes, max_nodes);
}

/**
 * Find all references to a symbol across the project
 * Delegates to implementation in symbol_management.c
 */
size_t project_find_references(const ProjectContext *project, const ASTNode *node,
                               const ASTNode **out_references, size_t max_references) {
  extern size_t project_find_references_impl(const ProjectContext *project, const ASTNode *node,
                                             const ASTNode **out_references, size_t max_references);
  return project_find_references_impl(project, node, out_references, max_references);
}

/**
 * Public API: Find all references to a symbol across the project
 * Delegates to implementation
 */
size_t project_context_find_references(const ProjectContext *project, const ASTNode *node,
                                       const ASTNode **out_references, size_t max_references) {
  return project_find_references(project, node, out_references, max_references);
}

/**
 * Get project statistics
 * Delegates to implementation in project_utils.c
 */
void project_get_stats(const ProjectContext *project, size_t *out_total_files,
                       size_t *out_total_symbols, size_t *out_total_references,
                       size_t *out_unresolved) {
  extern void project_get_stats_impl(const ProjectContext *project, size_t *out_total_files,
                                     size_t *out_total_symbols, size_t *out_total_references,
                                     size_t *out_unresolved);
  project_get_stats_impl(project, out_total_files, out_total_symbols, out_total_references,
                         out_unresolved);
}

/**
 * Set an error message in the project context
 * Delegates to implementation in project_utils.c
 */
void project_set_error(ProjectContext *project, int code, const char *message) {
  extern void project_set_error_impl(ProjectContext * project, int code, const char *message);
  project_set_error_impl(project, code, message);
}

/**
 * Get the last error message
 * Delegates to implementation in project_utils.c
 */
const char *project_get_error(const ProjectContext *project, int *out_code) {
  extern const char *project_get_error_impl(const ProjectContext *project, int *out_code);
  return project_get_error_impl(project, out_code);
}

//-----------------------------------------------------------------------------
// Internal helper functions
//-----------------------------------------------------------------------------

/**
 * Normalize a file path relative to the project root
 */
static bool normalize_file_path(const char *project_root, const char *filepath, char *out_path,
                                size_t out_size) {
  // Simple implementation - just prepend root if path is relative
  // TODO: Implement proper path normalization with ".." and "." handling

  if (!project_root || !filepath || !out_path || out_size == 0) {
    return false;
  }

  // Check if path is already absolute
  if (filepath[0] == '/') {
    strncpy(out_path, filepath, out_size - 1);
    out_path[out_size - 1] = '\0';
    return true;
  }

  // Prepend project root
  size_t root_len = strlen(project_root);
  if (root_len + 1 + strlen(filepath) >= out_size) {
    // Path too long
    return false;
  }

  strcpy(out_path, project_root);

  // Add separator if needed
  if (project_root[root_len - 1] != '/') {
    out_path[root_len] = '/';
    strcpy(out_path + root_len + 1, filepath);
  } else {
    strcpy(out_path + root_len, filepath);
  }

  return true;
}

/**
 * Add a file to the discovered files list
 */
static bool add_discovered_file(ProjectContext *project, const char *filepath) {
  if (!project || !filepath) {
    return false;
  }

  // Check if already in the discovered list
  for (size_t i = 0; i < project->num_discovered; i++) {
    if (strcmp(project->discovered_files[i], filepath) == 0) {
      return true; // Already discovered
    }
  }

  // Resize if needed
  if (project->num_discovered >= project->discovered_capacity) {
    size_t new_capacity = project->discovered_capacity * 2;
    char **new_files = (char **)realloc(project->discovered_files, new_capacity * sizeof(char *));
    if (!new_files) {
      return false;
    }
    project->discovered_files = new_files;
    project->discovered_capacity = new_capacity;
  }

  // Add to the list
  project->discovered_files[project->num_discovered] = strdup(filepath);
  if (!project->discovered_files[project->num_discovered]) {
    return false;
  }
  project->num_discovered++;

  return true;
}

/**
 * Check if a file is already parsed
 */
static bool is_file_parsed(const ProjectContext *project, const char *filepath) {
  if (!project || !filepath) {
    return false;
  }

  // Check against all parsed files
  for (size_t i = 0; i < project->num_files; i++) {
    // Compare with the filepath stored in the context
    // Note: This assumes the filepath is stored in the context
    // which may need to be added in the future
    if (strcmp(project->file_contexts[i]->filename, filepath) == 0) {
      return true;
    }
  }

  return false;
}

/**
 * Register symbols from a parsed file into the global symbol table
 */
static void register_file_symbols(ProjectContext *project, ParserContext *ctx,
                                  const char *filepath) {
  if (!project || !ctx || !filepath || !project->symbol_table) {
    return;
  }

  // Skip if there are no nodes
  if (!ctx->all_ast_nodes || ctx->num_ast_nodes == 0) {
    return;
  }

  // Language detection
  Language language = LANG_UNKNOWN;
  const char *extension = strrchr(filepath, '.');
  if (extension) {
    extension++; // Skip the dot
    if (strcasecmp(extension, "c") == 0) {
      language = LANG_C;
    } else if (strcasecmp(extension, "h") == 0) {
      language = LANG_C;
    } else if (strcasecmp(extension, "cpp") == 0 || strcasecmp(extension, "cc") == 0 ||
               strcasecmp(extension, "hpp") == 0) {
      language = LANG_CPP;
    } else if (strcasecmp(extension, "py") == 0) {
      language = LANG_PYTHON;
    } else if (strcasecmp(extension, "js") == 0) {
      language = LANG_JAVASCRIPT;
    } else if (strcasecmp(extension, "ts") == 0) {
      language = LANG_TYPESCRIPT;
    }
  }

  // Register all nodes with qualified names
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    ASTNode *node = ctx->all_ast_nodes[i];
    if (!node || !node->qualified_name || strlen(node->qualified_name) == 0) {
      continue;
    }

    // Determine symbol scope
    SymbolScope scope = SCOPE_LOCAL;
    switch (node->type) {
    case NODE_FUNCTION:
    case NODE_CLASS:
    case NODE_MODULE:
    case NODE_STRUCT:
    case NODE_ENUM:
    case NODE_INTERFACE:
      scope = SCOPE_GLOBAL;
      break;

    case NODE_METHOD:
    case NODE_PROPERTY:
      scope = SCOPE_CLASS;
      break;

    default:
      scope = SCOPE_LOCAL;
      break;
    }

    // Register the symbol
    SymbolEntry *entry = symbol_table_register(project->symbol_table, node->qualified_name, node,
                                               filepath, scope, language);

    if (entry) {
      // Track the total number of registered symbols
      project->total_symbols++;

      // Register the scope for name resolution
      if (scope == SCOPE_GLOBAL && node->name) {
        // For global symbols, register their parent scope
        const char *parent_scope = NULL;
        if (node->parent && node->parent->qualified_name) {
          parent_scope = node->parent->qualified_name;
          symbol_table_add_scope(project->symbol_table, parent_scope);
        }
      }
    }
  }
}

/**
 * Extract includes/imports from a parsed file and add them to project
 */
static void extract_and_process_includes(ProjectContext *project, ParserContext *ctx,
                                         const char *filepath) {
  if (!project || !ctx || !filepath) {
    return;
  }

  // Skip if no nodes or reached max include depth
  if (!ctx->all_ast_nodes || ctx->num_ast_nodes == 0 ||
      project->current_include_depth >= project->config.max_include_depth) {
    return;
  }

  // Detect file language
  Language language = LANG_UNKNOWN;
  const char *extension = strrchr(filepath, '.');
  if (extension) {
    extension++; // Skip the dot
    if (strcasecmp(extension, "c") == 0 || strcasecmp(extension, "h") == 0) {
      language = LANG_C;
    } else if (strcasecmp(extension, "cpp") == 0 || strcasecmp(extension, "cc") == 0 ||
               strcasecmp(extension, "hpp") == 0) {
      language = LANG_CPP;
    } else if (strcasecmp(extension, "py") == 0) {
      language = LANG_PYTHON;
    } else if (strcasecmp(extension, "js") == 0) {
      language = LANG_JAVASCRIPT;
    } else if (strcasecmp(extension, "ts") == 0) {
      language = LANG_TYPESCRIPT;
    }
  }

  // Find include/import nodes based on language
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    ASTNode *node = ctx->all_ast_nodes[i];
    if (!node) {
      continue;
    }

    char *include_path = NULL;
    bool is_system_include = false;

    // Language-specific include/import extraction
    switch (language) {
    case LANG_C:
    case LANG_CPP:
      // Look for nodes with type NODE_INCLUDE or check raw_content for #include
      if (node->type == NODE_INCLUDE && node->raw_content) {
        // Parse out the include path
        char *start = strchr(node->raw_content, '"');
        if (start) {
          // Local include with ""
          start++;
          char *end = strchr(start, '"');
          if (end) {
            size_t len = end - start;
            include_path = malloc(len + 1);
            if (include_path) {
              strncpy(include_path, start, len);
              include_path[len] = '\0';
              is_system_include = false;
            }
          }
        } else {
          // System include with <>
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
      // Removed NODE_MODULE_REF as it is not defined
      if ((node->type == NODE_IMPORT) && node->name) {
        include_path = strdup(node->name);
      }
      break;

    case LANG_JAVASCRIPT:
    case LANG_TYPESCRIPT:
      // Look for import/require statements
      // Removed NODE_MODULE_REF as it is not defined
      if ((node->type == NODE_IMPORT) && node->raw_content) {
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
        continue;
      }

      // Determine the full path
      char full_path[1024];
      if (is_system_include) {
        // For system includes, try standard include paths
        // This is very simplified and should be expanded
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

      // Increase include depth
      project->current_include_depth++;

      // Add the file to the project for parsing
      project_add_file(project, full_path, language);

      // Restore include depth
      project->current_include_depth--;

      free(include_path);
    }
  }
}
