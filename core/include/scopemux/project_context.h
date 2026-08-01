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

#include "../src/parser/parser_context.h"
#include "parser.h"
#include "symbol_table.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Error codes for ProjectContext operations
 */
typedef enum {
  PROJECT_ERROR_NONE = 0,             ///< No error
  PROJECT_ERROR_MEMORY = 1,           ///< Memory allocation failure
  PROJECT_ERROR_TOO_MANY_FILES = 2,   ///< Exceeded maximum file count
  PROJECT_ERROR_INCLUDE_DEPTH = 3,    ///< Exceeded maximum include/import depth
  PROJECT_ERROR_INVALID_PATH = 4,     ///< Invalid or unresolvable file path
  PROJECT_ERROR_IO = 5,               ///< I/O error (file or directory access)
  PROJECT_ERROR_UNKNOWN_LANGUAGE = 6, ///< Unknown or unsupported language
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
 * @brief Normalized visibility values for project-level Symbol IR.
 */
typedef enum {
  PROJECT_IR_VISIBILITY_UNKNOWN = 0,
  PROJECT_IR_VISIBILITY_PUBLIC,
  PROJECT_IR_VISIBILITY_PRIVATE,
  PROJECT_IR_VISIBILITY_PROTECTED,
  PROJECT_IR_VISIBILITY_INTERNAL,
} ProjectIRVisibility;

/**
 * @brief Relationship kinds for project-level dependency edges.
 */
typedef enum {
  PROJECT_DEPENDENCY_UNKNOWN = 0,
  PROJECT_DEPENDENCY_INCLUDE,
  PROJECT_DEPENDENCY_IMPORT,
  PROJECT_DEPENDENCY_REQUIRE,
  PROJECT_DEPENDENCY_FILE_RELATION,
} ProjectDependencyKind;

/**
 * @brief A resolved reference owned by a project symbol.
 *
 * String pointers and AST node pointers are borrowed from the owning project.
 * They remain valid until the next IR rebuild or project destruction.
 */
typedef struct {
  const ASTNode *owner_symbol_node;
  const ASTNode *reference_node;
  const ASTNode *target_node;
  const char *owner_symbol;
  const char *target_symbol;
  const char *target_file_path;
} ProjectResolvedReferenceIR;

/**
 * @brief Stable Symbol IR entry for a declaration in the project.
 */
typedef struct {
  const ASTNode *node;
  const char *name;
  const char *qualified_name;
  const char *signature;
  const char *docstring;
  const char *scope_qualified_name;
  const char *file_path;
  ASTNodeType type;
  ProjectIRVisibility visibility;
  size_t resolved_reference_start;
  size_t resolved_reference_count;
} ProjectSymbolIR;

/**
 * @brief Project-wide call graph edge.
 */
typedef struct {
  const ASTNode *caller_node;
  const ASTNode *callee_node;
  const ASTNode *callsite_node;
  const char *caller_symbol;
  const char *callee_symbol;
  const char *caller_file_path;
  const char *callee_file_path;
} ProjectCallGraphEdgeIR;

/**
 * @brief Project-wide import/include/dependency edge.
 */
typedef struct {
  const ASTNode *node;
  const char *source_file_path;
  const char *target_file_path;
  const char *specifier;
  ProjectDependencyKind kind;
} ProjectDependencyIR;

/**
 * @brief In-memory snapshot of project-level IR.
 *
 * Array storage is owned by the ProjectContext. Entry fields borrow strings and
 * AST node pointers from parser contexts already stored in the project.
 */
typedef struct {
  ProjectSymbolIR *symbols;
  size_t symbol_count;
  ProjectResolvedReferenceIR *resolved_references;
  size_t resolved_reference_count;
  ProjectCallGraphEdgeIR *call_graph_edges;
  size_t call_graph_edge_count;
  ProjectDependencyIR *dependencies;
  size_t dependency_count;
} ProjectIRSnapshot;

/**
 * @brief Canonical InfoBlock kinds derived from project IR.
 */
typedef enum {
  PROJECT_INFO_BLOCK_SYMBOL = 0,
  PROJECT_INFO_BLOCK_REFERENCE,
  PROJECT_INFO_BLOCK_FILE,
  PROJECT_INFO_BLOCK_DIRECTORY,
  PROJECT_INFO_BLOCK_PROJECT,
} ProjectInfoBlockKind;

/**
 * @brief Standardized tier scale for machine-readable context selection.
 */
typedef enum {
  PROJECT_CONTEXT_TIER_0 = 0,
  PROJECT_CONTEXT_TIER_1 = 1,
  PROJECT_CONTEXT_TIER_2 = 2,
  PROJECT_CONTEXT_TIER_3 = 3,
  PROJECT_CONTEXT_TIER_4 = 4,
} ProjectContextTier;

/**
 * @brief Stable registry entry for a semantic unit or synthetic aggregate block.
 *
 * String pointers and AST node pointers are owned by the ProjectContext and stay
 * valid until the next IR or InfoBlock rebuild, or project destruction.
 */
typedef struct {
  char *id;
  char *name;
  char *qualified_name;
  char *file_path;
  const ASTNode *node;
  ASTNodeType node_type;
  Language language;
  ProjectInfoBlockKind kind;
  ProjectContextTier tier;
  size_t estimated_tokens;
  size_t related_symbol_count;
} ProjectInfoBlock;

/**
 * @brief Dense registry of canonical InfoBlocks derived from project IR.
 */
typedef struct {
  ProjectInfoBlock *blocks;
  size_t block_count;
  size_t tier_counts[5];
} ProjectInfoBlockRegistry;

/**
 * @brief Rendering disposition for a selected InfoBlock in a tiered context.
 */
typedef enum {
  PROJECT_CONTEXT_BLOCK_EXPANDED = 0,
  PROJECT_CONTEXT_BLOCK_SUMMARIZED,
  PROJECT_CONTEXT_BLOCK_PINNED,
} ProjectTieredContextDisposition;

/**
 * @brief Machine-readable tiered context request.
 */
typedef struct {
  const char **focus_block_ids;
  size_t focus_block_count;
  const char **exclude_block_ids;
  size_t exclude_block_count;
  const char **summary_only_block_ids;
  size_t summary_only_block_count;
  const char *anchor_symbol;
  const char *anchor_file_path;
  ProjectContextTier min_tier;
  ProjectContextTier max_tier;
  bool include_related;
  bool include_dependencies;
  size_t max_blocks;
  size_t max_tokens;
} ProjectTieredContextRequest;

/**
 * @brief A selected InfoBlock inside a tiered context result.
 */
typedef struct {
  const ProjectInfoBlock *block;
  ProjectTieredContextDisposition disposition;
  bool from_focus;
} ProjectTieredContextSelection;

/**
 * @brief Machine-readable tiered context response.
 */
typedef struct {
  ProjectTieredContextSelection *selections;
  size_t selection_count;
  size_t estimated_tokens;
  ProjectContextTier effective_min_tier;
  ProjectContextTier effective_max_tier;
} ProjectTieredContextResult;

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

  // Project-level IR snapshot
  ProjectIRSnapshot ir_snapshot; ///< Durable project-level IR derived from ASTs and references
  bool ir_ready;                 ///< True when ir_snapshot reflects current project state

  // Canonical InfoBlock registry derived from project IR
  ProjectInfoBlockRegistry info_block_registry;
  bool info_block_registry_ready;
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
 * @brief Get the number of files in the project
 *
 * @param project Project context
 * @return size_t Number of files
 */
size_t project_context_get_file_count(const ProjectContext *project);

/**
 * @brief Get a file context by index
 *
 * @param project Project context
 * @param index File index
 * @return ParserContext* File context or NULL if not found
 */
ParserContext *project_context_get_file_by_index(const ProjectContext *project, size_t index);

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
 * @brief Add a file to the project context
 *
 * @param project Project context
 * @param filepath Path to the file
 * @param language Language of the file
 * @return bool True if file was added successfully, false otherwise
 */
bool project_context_add_file(ProjectContext *project, const char *filepath, Language language);

/**
 * @brief Remove a file from the project context
 *
 * @param project Project context
 * @param filepath Path to the file
 * @return bool True if file was removed successfully, false otherwise
 */
bool project_context_remove_file(ProjectContext *project, const char *filepath);

/**
 * @brief Add a dependency between two files
 *
 * @param project Project context
 * @param source_file Source file path
 * @param target_file Target file path
 * @return bool True if dependency was added successfully, false otherwise
 */
bool project_context_add_dependency(ProjectContext *project, const char *source_file,
                                    const char *target_file);

/**
 * @brief Get dependencies for a file
 *
 * @param project Project context
 * @param filepath Path to the file
 * @param out_dependencies Output array for dependencies
 * @return size_t Number of dependencies
 */
size_t project_context_get_dependencies(ProjectContext *project, const char *filepath,
                                        const char ***out_dependencies);

/**
 * @brief Extract symbols from a parser context into the project's global symbol table
 *
 * @param project Project context
 * @param parser Parser context containing AST nodes
 * @param symbol_table Global symbol table
 * @return bool True if symbols were extracted successfully, false otherwise
 */
bool project_context_extract_symbols(ProjectContext *project, ParserContext *parser,
                                     GlobalSymbolTable *symbol_table);

bool extract_symbols_from_parser_context(ProjectContext *project, ParserContext *ctx,
                                         void *symbols);

/**
 * @brief Clear the current project IR snapshot.
 *
 * This releases snapshot storage but does not modify AST nodes or parser state.
 * Borrowed pointers obtained from previous snapshots become invalid.
 *
 * @param project Project context
 */
void project_context_clear_ir(ProjectContext *project);

/**
 * @brief Rebuild the project-level IR snapshot from current parser state.
 *
 * This emits Symbol IR, resolved-reference IR, Call Graph IR, and
 * Import/Dependency IR from the current project contents.
 *
 * @param project Project context
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_rebuild_ir(ProjectContext *project);

/**
 * @brief Get the current project-level IR snapshot.
 *
 * The returned pointer is owned by the project context and is invalidated by
 * the next rebuild, clear, or project destruction.
 *
 * @param project Project context
 * @return const ProjectIRSnapshot* Snapshot or NULL when unavailable
 */
const ProjectIRSnapshot *project_context_get_ir(const ProjectContext *project);

/**
 * @brief Rebuild the canonical InfoBlock registry from current project IR.
 *
 * This emits Tier 0-4 blocks covering symbol, reference, file, directory, and
 * project-level semantic units.
 *
 * @param project Project context
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_rebuild_info_blocks(ProjectContext *project);

/**
 * @brief Get the current InfoBlock registry, rebuilding it on demand.
 *
 * @param project Project context
 * @return const ProjectInfoBlockRegistry* Registry or NULL on failure
 */
const ProjectInfoBlockRegistry *project_context_get_info_block_registry(ProjectContext *project);

/**
 * @brief Find a canonical InfoBlock by its stable ID.
 *
 * @param project Project context
 * @param block_id Stable block identifier such as `sym:name` or `file:path`
 * @return const ProjectInfoBlock* Matching block or NULL if not found
 */
const ProjectInfoBlock *project_context_find_info_block(const ProjectContext *project,
                                                        const char *block_id);

/**
 * @brief Build a tiered context selection from the canonical InfoBlock registry.
 *
 * @param project Project context
 * @param request Machine-readable tiered context request
 * @param out_result Output result; caller must free with project_tiered_context_result_free()
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_build_tiered_context(ProjectContext *project,
                                          const ProjectTieredContextRequest *request,
                                          ProjectTieredContextResult *out_result);

/**
 * @brief Free heap storage owned by a tiered context result.
 *
 * @param result Result to clear
 */
void project_tiered_context_result_free(ProjectTieredContextResult *result);
#endif /* SCOPEMUX_PROJECT_CONTEXT_H */
