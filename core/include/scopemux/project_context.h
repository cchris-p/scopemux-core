/**
 * @file project_context.h
 * @brief Multi-file parsing and relationship management for ScopeMux
 *
 * This module provides infrastructure for managing and analyzing multiple files
 * as a cohesive project, enabling inter-file relationship tracking, resolution
 * of cross-file references, and project-wide symbol management.
 *
 * The ProjectContext implementation is modularized into specialized components:
 * - Core lifecycle and state management
 * - File discovery and management
 * - Symbol registration and reference resolution
 * - Dependency tracking and include/import resolution
 *
 * Each component is implemented in separate source files within the
 * project_context/ directory for improved maintainability and extensibility.
 */

#ifndef SCOPEMUX_PROJECT_CONTEXT_H
#define SCOPEMUX_PROJECT_CONTEXT_H

#include "parser.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Error codes for ProjectContext operations
 */
typedef enum {
  PROJECT_ERROR_NONE = 0,           ///< No error
  PROJECT_ERROR_MEMORY = 1,         ///< Memory allocation failure
  PROJECT_ERROR_TOO_MANY_FILES = 2, ///< Exceeded maximum file count
  PROJECT_ERROR_INCLUDE_DEPTH = 3,  ///< Exceeded maximum include/import depth
  PROJECT_ERROR_INVALID_PATH = 4,   ///< Invalid or unresolvable file path
  PROJECT_ERROR_IO = 5,             ///< I/O error (file or directory access)
  // Add more as needed for project context errors
} ProjectErrorCode;

/**
 * @brief Configuration options for project parsing
 */
typedef struct {
  bool parse_headers;             ///< Whether to parse header files
  bool follow_includes;           ///< Whether to automatically follow include/import statements
  bool resolve_external_symbols;  ///< Whether to resolve symbols from external libraries
  unsigned int max_files;         ///< Maximum number of files to parse (0 for no limit)
  unsigned int max_include_depth; ///< Maximum recursion depth for include/import resolution
  LogLevel log_level;             ///< Logging verbosity level
} ProjectConfig;

/**
 * @brief A collection of related source files forming a project
 *
 * The ProjectContext manages multiple ParserContext instances, enabling
 * cross-file analysis and relationship tracking.
 */
typedef struct ProjectContext {
  char *root_directory;            ///< Root directory of the project
  ParserContext **file_contexts;   ///< Array of parsed file contexts
  size_t num_files;                ///< Number of files in the project
  size_t files_capacity;           ///< Allocated capacity for file_contexts array
  GlobalSymbolTable *symbol_table; ///< Project-wide symbol table
  ProjectConfig config;            ///< Configuration options

  // Error reporting
  char *error_message; ///< Last error message
  int error_code;      ///< Last error code

  // Statistics and metadata
  size_t total_symbols;         ///< Total number of symbols in the project
  size_t total_references;      ///< Total number of cross-file references resolved
  size_t unresolved_references; ///< Count of references that could not be resolved

  // File discovery state
  char **discovered_files;      ///< Files discovered but not yet parsed
  size_t num_discovered;        ///< Number of discovered files
  size_t discovered_capacity;   ///< Capacity of discovered_files array
  size_t current_include_depth; ///< Current include depth during dependency resolution
} ProjectContext;

/**
 * @brief Create a new project context
 *
 * @param root_directory Root directory of the project (will be copied)
 * @return ProjectContext* New project context or NULL on failure
 */
ProjectContext *project_context_create(const char *root_directory);

/**
 * @brief Free all resources associated with a project context
 *
 * This includes all contained ParserContexts and their resources
 *
 * @param project Project context to free
 */
void project_context_free(ProjectContext *project);

/**
 * @brief Set project configuration options
 *
 * @param project Project context
 * @param config Configuration settings
 */
void project_context_set_config(ProjectContext *project, const ProjectConfig *config);

/**
 * @brief Add a file to the project for parsing
 *
 * @param project Project context
 * @param filepath Absolute or project-relative filepath
 * @param language Language hint (LANG_UNKNOWN for auto-detection)
 * @return bool True if file was added successfully, false otherwise
 */
bool project_add_file(ProjectContext *project, const char *filepath, Language language);

/**
 * @brief Add all files in a directory to the project
 *
 * @param project Project context
 * @param dirpath Directory path (absolute or project-relative)
 * @param extensions NULL-terminated array of file extensions to include (e.g., ".c", ".h")
 * @param recursive Whether to recursively search subdirectories
 * @return size_t Number of files added
 */
size_t project_add_directory(ProjectContext *project, const char *dirpath, const char **extensions,
                             bool recursive);

/**
 * @brief Parse all files in the project
 *
 * This function parses all added files, builds the symbol table,
 * and resolves cross-file references.
 *
 * @param project Project context
 * @return bool True if all files were parsed successfully, false otherwise
 */
bool project_parse_all_files(ProjectContext *project);

/**
 * @brief Resolve references across all files in the project
 *
 * This should be called after all files have been parsed and
 * symbols have been registered in the global symbol table.
 *
 * @param project Project context
 * @return bool True if references were resolved successfully, false otherwise
 */
bool project_resolve_references(ProjectContext *project);

/**
 * @brief Get a file context by filename
 *
 * @param project Project context
 * @param filepath Absolute filepath or project-relative path
 * @return ParserContext* Matching file context or NULL if not found
 */
ParserContext *project_get_file_context(const ProjectContext *project, const char *filepath);

/**
 * @brief Get a symbol by its qualified name from anywhere in the project
 *
 * @param project Project context
 * @param qualified_name Fully qualified name of the symbol
 * @return const ASTNode* Matching node or NULL if not found
 */
const ASTNode *project_get_symbol(const ProjectContext *project, const char *qualified_name);

/**
 * @brief Get all symbols of a specific type across the entire project
 *
 * @param project Project context
 * @param type Node type to filter by
 * @param out_nodes Output array of nodes (can be NULL to just get the count)
 * @param max_nodes Maximum number of nodes to return
 * @return size_t Number of nodes found
 */
size_t project_get_symbols_by_type(const ProjectContext *project, ASTNodeType type,
                                   const ASTNode **out_nodes, size_t max_nodes);

/**
 * @brief Find all references to a symbol across the project
 *
 * @param project Project context
 * @param node Symbol to find references to
 * @param out_references Output array of referencing nodes
 * @param max_references Maximum number of references to return
 * @return size_t Number of references found
 */
size_t project_find_references(const ProjectContext *project, const ASTNode *node,
                               const ASTNode **out_references, size_t max_references);

/**
 * @brief Get project statistics
 *
 * @param project Project context
 * @param out_total_files Output parameter for total file count
 * @param out_total_symbols Output parameter for total symbol count
 * @param out_total_references Output parameter for total reference count
 * @param out_unresolved Output parameter for unresolved reference count
 */
void project_get_stats(const ProjectContext *project, size_t *out_total_files,
                       size_t *out_total_symbols, size_t *out_total_references,
                       size_t *out_unresolved);

/**
 * @brief Set an error message in the project context
 *
 * @param project Project context
 * @param code Error code
 * @param message Error message (will be copied)
 */
void project_set_error(ProjectContext *project, int code, const char *message);

/**
 * @brief Get the last error message
 *
 * @param project Project context
 * @param out_code Output parameter for error code
 * @return const char* Error message or NULL if no error
 */
const char *project_get_error(const ProjectContext *project, int *out_code);

#endif /* SCOPEMUX_PROJECT_CONTEXT_H */
