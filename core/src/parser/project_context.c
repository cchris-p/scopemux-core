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
#include "../src/parser/project_context/project_utils.h"
#include "project_context/dependency_management.h"
#include "project_context/file_management.h"
#include "project_context/project_context_internal.h"
#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_management.h"
#include "scopemux/symbol_registration.h"
#include "scopemux/symbol_table.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @brief Remove a file from the project
 *
 * Delegates to implementation in file_management.c
 *
 * @param project The project context
 * @param filepath Path to the file to remove
 * @return true if file was removed successfully, false otherwise
 */
bool project_remove_file(ProjectContext *project, const char *filepath) {
  return project_remove_file_impl(project, filepath);
}

/**
 * @brief Public API: Remove a file from the project
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @param filepath Path to the file to remove
 * @return true if file was removed successfully, false otherwise
 */
bool project_context_remove_file(ProjectContext *project, const char *filepath) {
  return project_remove_file(project, filepath);
}

/**
 * @brief Add a dependency between two files
 *
 * Delegates to implementation in dependency_management.c
 *
 * @param project The project context
 * @param source_file Source file path
 * @param target_file Target file path
 * @return true if dependency was added successfully, false otherwise
 */
bool project_add_dependency(ProjectContext *project, const char *source_file,
                            const char *target_file) {
  return project_add_dependency_impl(project, source_file, target_file);
}

/**
 * @brief Public API: Add a dependency between two files
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @param source_file Source file path
 * @param target_file Target file path
 * @return true if dependency was added successfully, false otherwise
 */
bool project_context_add_dependency(ProjectContext *project, const char *source_file,
                                    const char *target_file) {
  return project_add_dependency(project, source_file, target_file);
}

/**
 * @brief Get dependencies for a file
 *
 * Delegates to implementation in dependency_management.c
 *
 * @param project The project context
 * @param filepath Path to the file
 * @param out_dependencies Output array for dependencies
 * @return Number of dependencies
 */
size_t project_get_dependencies(const ProjectContext *project, const char *filepath,
                                char ***out_dependencies) {
  return project_get_dependencies_impl(project, filepath, out_dependencies);
}

/**
 * @brief Public API: Get dependencies for a file
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @param filepath Path to the file
 * @param out_dependencies Output array for dependencies
 * @return Number of dependencies
 */
size_t project_context_get_dependencies(ProjectContext *project, const char *filepath,
                                        const char ***out_dependencies) {
  return project_get_dependencies(project, filepath, (char ***)out_dependencies);
}

/**
 * @brief Extract symbols from parsed files
 *
 * Delegates to implementation in symbol_management.c
 *
 * @param project The project context
 * @param parser Parser context containing AST nodes
 * @param symbol_table Global symbol table
 * @return true if symbols were extracted successfully, false otherwise
 */
bool project_extract_symbols(ProjectContext *project, ParserContext *parser,
                             GlobalSymbolTable *symbol_table) {
  return project_extract_symbols_impl(project, parser, symbol_table);
}

/**
 * @brief Public API: Extract symbols from a parser context into the project's global symbol table
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @param parser Parser context containing AST nodes
 * @param symbol_table Global symbol table
 * @return true if symbols were extracted successfully, false otherwise
 */
bool project_context_extract_symbols(ProjectContext *project, ParserContext *parser,
                                     GlobalSymbolTable *symbol_table) {
  return project_extract_symbols(project, parser, symbol_table);
}

/**
 * @brief External functions defined in the specialized modules
 *
 * All internal implementation functions are declared in project_context_internal.h, which is
 * included where needed. Redundant extern declarations have been removed for clarity and
 * maintainability.
 */

/**
 * @brief Create a new project context
 *
 * Delegates to implementation in project_utils.c
 *
 * @param root_directory Root directory of the project (will be copied)
 * @return New project context or NULL on failure
 */
ProjectContext *project_context_create(const char *root_directory) {
  return project_context_create_impl(root_directory);
}

/**
 * @brief Free all resources associated with a project context
 *
 * Delegates to implementation in project_utils.c
 *
 * @param project Project context to free
 */
void project_context_free(ProjectContext *project) { project_context_free_impl(project); }

/**
 * @brief Set project configuration options
 *
 * Delegates to implementation in project_utils.c
 *
 * @param project The project context
 * @param config Configuration options to set
 */
void project_context_set_config(ProjectContext *project, const ProjectConfig *config) {
  project_context_set_config_impl(project, config);
}

/**
 * @brief Add a file to the project for parsing
 *
 * Delegates to implementation in file_management.c
 *
 * @param project The project context
 * @param filepath Path to the file
 * @param language Language of the file
 * @return true if file was added successfully, false otherwise
 */
bool project_add_file(ProjectContext *project, const char *filepath, Language language) {
  return project_add_file_impl(project, filepath, language);
}

/**
 * @brief Public API: Add a file to the project for parsing
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @param filepath Path to the file
 * @param language Language of the file
 * @return true if file was added successfully, false otherwise
 */
bool project_context_add_file(ProjectContext *project, const char *filepath, Language language) {
  return project_add_file(project, filepath, language);
}

/**
 * @brief The project_context_extract_symbols function is already defined above
 */

/**
 * @brief Implementation for extracting symbols from a parser context
 *
 * This function forwards to the actual implementation in symbol_registration.c
 *
 * @param project The ProjectContext
 * @param ctx The ParserContext to extract symbols from
 * @param symbols The symbol collection to store extracted symbols in
 * @return true if successful, false otherwise
 */
bool extract_symbols_from_parser_context(ProjectContext *project, ParserContext *ctx,
                                         void *symbols) {
  // Forward to the implementation in symbol_registration.c
  return project_context_extract_symbols_impl(project, ctx, symbols);
}

/**
 * @brief Add all files in a directory to the project
 *
 * Delegates to implementation in file_management.c
 *
 * @param project The project context
 * @param dirpath Path to the directory
 * @param extensions Array of file extensions to include
 * @param recursive Whether to recursively search subdirectories
 * @return Number of files added
 */
size_t project_add_directory(ProjectContext *project, const char *dirpath, const char **extensions,
                             bool recursive) {
  return project_add_directory_impl(project, dirpath, extensions, recursive);
}

/**
 * @brief Public API: Add all files in a directory to the project
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @param dirpath Path to the directory
 * @param extensions Array of file extensions to include
 * @param recursive Whether to recursively search subdirectories
 * @return Number of files added
 */
size_t project_context_add_directory(ProjectContext *project, const char *dirpath,
                                     const char **extensions, bool recursive) {
  return project_add_directory(project, dirpath, extensions, recursive);
}

/**
 * @brief Parse all files in the project
 *
 * Delegates to implementation in dependency_management.c
 *
 * @param project The project context
 * @return true if all files were parsed successfully, false otherwise
 */
bool project_parse_all_files(ProjectContext *project) {
  return project_parse_all_files_impl(project);
}

/**
 * @brief Public API: Parse all files in the project
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @return true if all files were parsed successfully, false otherwise
 */
bool project_context_parse_all_files(ProjectContext *project) {
  return project_parse_all_files(project);
}

/**
 * @brief Resolve references across all files
 *
 * Delegates to implementation in symbol_management.c
 *
 * @param project The project context
 * @return true if references were resolved successfully, false otherwise
 */
bool project_resolve_references(ProjectContext *project) {
  return project_resolve_references_impl(project);
}

/**
 * @brief Public API: Resolve references across all files
 *
 * Delegates to implementation
 *
 * @param project The project context
 * @return true if references were resolved successfully, false otherwise
 */
bool project_context_resolve_references(ProjectContext *project) {
  return project_resolve_references(project);
}

/**
 * Get a file context by filename
 * Delegates to implementation in file_management.c
 */
ParserContext *project_get_file_context(const ProjectContext *project, const char *filepath) {
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
  project_get_stats_impl(project, out_total_files, out_total_symbols, out_total_references,
                         out_unresolved);
}
