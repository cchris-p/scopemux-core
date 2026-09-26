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
/**
 * @brief Contiguous slice of `ProjectIRSnapshot` arrays owned by one file.
 *
 * Entries are stored grouped by file in file order, so a file owns a
 * contiguous range in each array. Incremental rebuilds use these ranges to
 * retain a clean file's entries without re-deriving them and without reading
 * fields of entries owned by a recomputed (possibly freed) file.
 */
typedef struct {
  char *file_path; ///< Owned copy of the file path for this range
  uint64_t content_hash; ///< Content hash of the file at build time (`WI-018`)
  size_t symbol_start;
  size_t symbol_count;
  size_t reference_start;
  size_t reference_count;
  size_t call_edge_start;
  size_t call_edge_count;
  size_t dependency_start;
  size_t dependency_count;
} ProjectFileIRRange;

typedef struct {
  ProjectSymbolIR *symbols;
  size_t symbol_count;
  ProjectResolvedReferenceIR *resolved_references;
  size_t resolved_reference_count;
  ProjectCallGraphEdgeIR *call_graph_edges;
  size_t call_graph_edge_count;
  ProjectDependencyIR *dependencies;
  size_t dependency_count;
  ProjectFileIRRange *file_ranges; ///< Per-file array slices (`WI-018`)
  size_t file_range_count;
} ProjectIRSnapshot;

/**
 * @brief Reverse dependency edge used by incremental indexing (`WI-018`).
 *
 * Records that @c source_file references or depends on @c target_file. The
 * index is rebuilt from the current IR snapshot and drives the dirty-set
 * closure when a file changes or is removed, so dependents and referrers are
 * recomputed instead of being retained with dangling pointers.
 */
typedef struct {
  char *source_file; ///< File that owns the reference or dependency edge
  char *target_file; ///< File the edge points at
} ProjectReverseEdge;

/**
 * @brief Canonical InfoBlock kinds derived from project IR.
 */
typedef enum {
  PROJECT_INFO_BLOCK_SYMBOL = 0,
  PROJECT_INFO_BLOCK_REFERENCE,
  PROJECT_INFO_BLOCK_FILE,
  PROJECT_INFO_BLOCK_DIRECTORY,
  PROJECT_INFO_BLOCK_PROJECT,
  PROJECT_INFO_BLOCK_OBSERVABILITY, ///< Observability point (`WI-034`)
} ProjectInfoBlockKind;

/**
 * @brief Subtypes of an observability InfoBlock (`WI-034`).
 *
 * Reuses the error-annotation, invariant, and coverage concerns cataloged in
 * `wiki/supported-ir-structures.md` rather than a parallel model.
 */
typedef enum {
  PROJECT_OBSERVABILITY_LOG_POINT = 0,   ///< A log/emit point
  PROJECT_OBSERVABILITY_METRIC,          ///< A metric or counter
  PROJECT_OBSERVABILITY_ASSERTION,       ///< An assertion / check
  PROJECT_OBSERVABILITY_INVARIANT,       ///< An invariant contract
  PROJECT_OBSERVABILITY_EXPECTED_FAILURE, ///< An expected failure mode
  PROJECT_OBSERVABILITY_ERROR_ANNOTATION, ///< An error annotation
} ProjectObservabilityKind;

/**
 * @brief Whether a canonical InfoBlock was parsed from source or is a
 * projected (planned) node. Plan nodes are projection-only; see `WI-032`.
 */
typedef enum {
  PROJECT_INFO_BLOCK_ORIGIN_PARSED = 0,
  PROJECT_INFO_BLOCK_ORIGIN_PLANNED,
} ProjectInfoBlockOrigin;

/**
 * @brief Lifecycle for a canonical InfoBlock.
 *
 * Parsed blocks use `NONE`. Planned blocks move through delivery states and
 * may become `STALE`, `CONFLICT`, or `ABANDONED`; they are never silently
 * dropped.
 */
typedef enum {
  PROJECT_INFO_BLOCK_LIFECYCLE_NONE = 0,
  PROJECT_INFO_BLOCK_LIFECYCLE_PLANNED,
  PROJECT_INFO_BLOCK_LIFECYCLE_IN_PROGRESS,
  PROJECT_INFO_BLOCK_LIFECYCLE_IMPLEMENTED,
  PROJECT_INFO_BLOCK_LIFECYCLE_VERIFIED,
  PROJECT_INFO_BLOCK_LIFECYCLE_DOCUMENTED,
  PROJECT_INFO_BLOCK_LIFECYCLE_STALE,
  PROJECT_INFO_BLOCK_LIFECYCLE_CONFLICT,
  PROJECT_INFO_BLOCK_LIFECYCLE_ABANDONED,
} ProjectInfoBlockLifecycle;

/**
 * @brief Kinds of projected (target-state) plan nodes (`WI-032`).
 *
 * Plan nodes are projection-only: they describe intended code and never
 * override the external task record or a task's stage.
 */
typedef enum {
  PROJECT_PLAN_NODE_NEW_SYMBOL = 0,        ///< A symbol to add
  PROJECT_PLAN_NODE_NEW_FILE,              ///< A file to add
  PROJECT_PLAN_NODE_NEW_MODULE,            ///< A module/package to add
  PROJECT_PLAN_NODE_NEW_TEST,              ///< A test to add
  PROJECT_PLAN_NODE_MODIFY_SYMBOL,         ///< A change to an existing symbol
  PROJECT_PLAN_NODE_REMOVE,                ///< A removal
  PROJECT_PLAN_NODE_CONSOLIDATE,           ///< Consolidation of duplicate units
  PROJECT_PLAN_NODE_REFACTOR_OPPORTUNITY,  ///< A refactor to resolve
  PROJECT_PLAN_NODE_OBSERVABILITY_POINT,   ///< An observability point to add
} ProjectPlanNodeKind;

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
 * This is the canonical project registry block. It is a different type from the
 * @c ContextEngine @c InfoBlock in @c context_engine.h, which is the compression
 * engine's linked-list unit. String pointers and AST node pointers are owned by
 * the ProjectContext and stay valid until the next IR or InfoBlock rebuild, or
 * project destruction.
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
  ProjectObservabilityKind observability_kind; ///< Meaningful when kind is OBSERVABILITY
  ProjectContextTier tier;
  size_t estimated_tokens;
  size_t related_symbol_count;
  ProjectInfoBlockOrigin origin;        ///< parsed vs planned (`WI-033`)
  ProjectInfoBlockLifecycle lifecycle;  ///< parsed uses NONE (`WI-033`)
  char *provenance;                     ///< source range/file, or task record (`WI-033`)
  float confidence;                     ///< 1.0 for exact parsed facts (`WI-033`)
  /// Plan-node projection fields; meaningful only when `origin` is `PLANNED`.
  ProjectPlanNodeKind plan_kind;        ///< plan-node kind (`WI-032`)
  char *desired_shape;                  ///< projected signature/structure (`WI-032`)
  char *rationale;                      ///< why the node exists (`WI-032`)
  char *anchor_list;                    ///< `;`-joined anchor block ids (`WI-032`)

  /// Duplicate/refactor detection (`WI-035`). Computed lazily and cached on the
  /// block; `structural_hash_ready` guards the cached value.
  uint64_t structural_hash;      ///< Normalized AST-structure fingerprint
  bool structural_hash_ready;    ///< Whether `structural_hash` has been computed
} ProjectInfoBlock;

/**
 * @brief Per-file slice of the canonical InfoBlock registry (`WI-018`).
 *
 * Identifies the symbol and reference blocks owned by one file and records the
 * file's content hash at build time. A registry rebuild reuses a file's blocks
 * when its hash is unchanged, so clean files are not re-derived from IR/AST.
 */
typedef struct {
  char *file_path;       ///< Owned copy of the file path for this range
  uint64_t content_hash; ///< File content hash at the last registry build
  size_t symbol_start;   ///< First symbol block index in the registry
  size_t symbol_count;   ///< Number of symbol blocks for this file
  size_t reference_start; ///< First reference block index in the registry
  size_t reference_count; ///< Number of reference blocks for this file
} ProjectInfoBlockRange;

/**
 * @brief Dense registry of canonical InfoBlocks derived from project IR.
 */
typedef struct {
  ProjectInfoBlock *blocks;
  size_t block_count;
  size_t tier_counts[5];
  ProjectInfoBlockRange *file_ranges; ///< Per-file block slices (`WI-018`)
  size_t file_range_count;
} ProjectInfoBlockRegistry;

/**
 * @brief A projected target-state (plan) node (`WI-032`).
 *
 * Plan nodes are the durable, projection-only representation of intended code.
 * They are materialized into the canonical InfoBlock registry with
 * `origin == PROJECT_INFO_BLOCK_ORIGIN_PLANNED`. The external task record stays
 * authoritative; a plan node never advances a task stage or declares
 * completion.
 *
 * All strings are owned by the plan-node store and remain valid until the node
 * is cleared or the project is freed.
 */
typedef struct ProjectPlanNode {
  char *id;               ///< stable id: `plan:<task_id>:<slug>`
  char *task_id;          ///< external task-record id (provenance)
  char *slug;             ///< stable, human-readable slug
  char *title;            ///< human-readable title
  char *desired_shape;    ///< projected signature/structure/expected symbols
  char *rationale;        ///< why the node exists, tied to completion criteria
  char *provenance;       ///< task record/stage provenance
  char *projected_symbol; ///< expected symbol name once implemented (optional)
  char *file_path;        ///< projected file path (optional)
  ProjectPlanNodeKind kind;
  ProjectObservabilityKind observability_kind; ///< Subtype for observability points (`WI-034`)
  ProjectInfoBlockLifecycle lifecycle;
  float confidence;
  char **anchor_ids; ///< anchor block ids in the current-state registry
  size_t anchor_count;
  size_t anchor_capacity;
} ProjectPlanNode;

/**
 * @brief One plan-node lifecycle transition observed during reconciliation.
 */
typedef struct {
  const ProjectPlanNode *node;                     ///< node that transitioned
  ProjectInfoBlockLifecycle previous_lifecycle;    ///< lifecycle before reconcile
  ProjectInfoBlockLifecycle new_lifecycle;         ///< lifecycle after reconcile
} ProjectPlanNodeReconciliationEntry;

/**
 * @brief Machine-readable result of reconciling plan nodes against parsed state.
 *
 * Entries record every lifecycle transition. The `stale_count`, `conflict_count`,
 * and `implemented_count` counters summarize transitions into those states.
 * Reconciliation never deletes a plan node and never advances task state.
 */
typedef struct {
  ProjectPlanNodeReconciliationEntry *entries;
  size_t entry_count;
  size_t stale_count;
  size_t conflict_count;
  size_t implemented_count;
} ProjectPlanNodeReconciliationResult;

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
  /// Bitmask of ProjectInfoBlockOrigin to include; 0 means all origins.
  unsigned int origin_mask;
  /// Bitmask of ProjectInfoBlockLifecycle to include; 0 means all lifecycles.
  unsigned int lifecycle_mask;
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

typedef struct ProjectSearchIndexEntry ProjectSearchIndexEntry;

/**
 * @brief Machine-readable indexed search request over canonical InfoBlocks.
 */
typedef struct {
  const char *query_text;
  const char *anchor_symbol;
  const char *anchor_file_path;
  ProjectContextTier min_tier;
  ProjectContextTier max_tier;
  bool include_related;
  bool include_dependencies;
  size_t max_hits;
  /// Bitmask of ProjectInfoBlockOrigin to include; 0 means all origins.
  unsigned int origin_mask;
  /// Bitmask of ProjectInfoBlockLifecycle to include; 0 means all lifecycles.
  unsigned int lifecycle_mask;
} ProjectSearchRequest;

/**
 * @brief A scored search hit from the project search index.
 */
typedef struct {
  const ProjectInfoBlock *block;
  size_t score;
  bool name_match;
  bool text_match;
  bool relationship_match;
} ProjectSearchHit;

/**
 * @brief Machine-readable result set from the project search index.
 */
typedef struct {
  ProjectSearchHit *hits;
  size_t hit_count;
  size_t total_match_count;
} ProjectSearchResult;

/**
 * @brief Prompt assembly request built on top of tiered context selection.
 */
typedef struct {
  ProjectTieredContextRequest context_request;
  const char *user_query;
  const char *system_preamble;
  const char *response_format;
  bool include_block_metadata;
  size_t max_prompt_tokens;
} ProjectPromptAssemblyRequest;

/**
 * @brief Prompt assembly output for downstream LLM or tool consumers.
 */
typedef struct {
  ProjectTieredContextResult context_result;
  char *prompt_text;
  size_t prompt_length;
  size_t estimated_tokens;
  size_t omitted_block_count;
} ProjectPromptAssemblyResult;

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

  // Durable plan-node store projected into the registry; survives re-index (WI-032)
  ProjectPlanNode *plan_nodes;
  size_t plan_node_count;
  size_t plan_node_capacity;

  // Internal searchable index derived from the canonical InfoBlock registry
  ProjectSearchIndexEntry *search_index_entries;
  size_t search_index_entry_count;
  bool search_index_ready;

  // Incremental indexing state (WI-018). Reverse edges drive the dirty-set
  // closure; the pending dirty set is expanded when a file is marked dirty and
  // consumed by the next incremental IR rebuild. The IR arrays are retained
  // across a dirty mark so clean files are not recomputed.
  ProjectReverseEdge *reverse_edges; ///< Reverse reference/dependency edges
  size_t reverse_edge_count;
  size_t reverse_edge_capacity;
  char **dirty_files; ///< Pending dirty file set (absolute/normalized paths)
  size_t dirty_count;
  size_t dirty_capacity;
  bool ir_snapshot_retained; ///< IR arrays hold a complete previous snapshot
  size_t last_recomputed_file_count; ///< Files recomputed by the last IR rebuild
  size_t last_rebuild_file_count;    ///< Files present during the last IR rebuild
  bool last_rebuild_incremental;     ///< Whether the last IR rebuild was incremental

  // InfoBlock registry retention stats (`WI-018`): files whose blocks were
  // reused versus re-derived during the last registry rebuild.
  size_t last_info_block_reused_file_count;
  size_t last_info_block_recomputed_file_count;
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
 * @brief Hash a content buffer for incremental change detection (WI-018)
 *
 * Deterministic, dependency-free FNV-1a 64-bit hash. Not cryptographic.
 *
 * @param data Buffer to hash (may be NULL only when length is 0)
 * @param length Number of bytes to hash
 * @return uint64_t Content hash
 */
uint64_t project_context_hash_content(const void *data, size_t length);

/**
 * @brief Whether a parsed file's content is unchanged (WI-018)
 *
 * Compares the incoming content against the hash recorded for the file. Returns
 * false when the file is not currently parsed, so callers can treat it as new.
 *
 * @param project Project context
 * @param filepath Absolute or project-relative filepath
 * @param content Incoming content
 * @param content_length Length of @p content
 * @return bool True when the file is parsed and its content hash matches
 */
bool project_file_is_unchanged(ProjectContext *project, const char *filepath, const char *content,
                               size_t content_length);

/**
 * @brief Incrementally parse or update a file from an in-memory buffer (WI-018)
 *
 * If the file is already parsed and its content hash matches @p content the
 * call is a no-op and @p out_changed is set to false. Otherwise the file is
 * (re)parsed, its symbols are re-registered, and the derived IR / InfoBlock /
 * search caches are invalidated. Durable plan nodes are never touched.
 *
 * @param project Project context
 * @param filepath Absolute or project-relative filepath
 * @param content Source content
 * @param content_length Length of @p content
 * @param language Language hint (LANG_UNKNOWN to auto-detect from the extension)
 * @param out_changed Optional; set to true when the file was (re)parsed
 * @return bool True on success (including a no-op), false on error
 */
bool project_update_file_from_string(ProjectContext *project, const char *filepath,
                                     const char *content, size_t content_length, Language language,
                                     bool *out_changed);

/**
 * @brief Incrementally parse or update a file from disk (WI-018)
 *
 * Reads the file and delegates to project_update_file_from_string, so an
 * unchanged file is not re-parsed.
 *
 * @param project Project context
 * @param filepath Absolute or project-relative filepath
 * @param language Language hint (LANG_UNKNOWN to auto-detect from the extension)
 * @param out_changed Optional; set to true when the file was (re)parsed
 * @return bool True on success (including a no-op), false on error
 */
bool project_update_file(ProjectContext *project, const char *filepath, Language language,
                         bool *out_changed);

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
 * @brief Number of files recomputed by the most recent IR rebuild (`WI-018`).
 *
 * A full rebuild recomputes every file; an incremental rebuild recomputes only
 * the dirty set (the changed or removed file plus its transitive dependents and
 * referrers). Useful for asserting selective recomputation.
 *
 * @param project Project context
 * @return size_t Recomputed file count
 */
size_t project_context_last_recomputed_file_count(const ProjectContext *project);

/**
 * @brief Number of files present during the most recent IR rebuild (`WI-018`).
 *
 * @param project Project context
 * @return size_t File count
 */
size_t project_context_last_rebuild_file_count(const ProjectContext *project);

/**
 * @brief Whether the most recent IR rebuild was incremental (`WI-018`).
 *
 * @param project Project context
 * @return bool True when the rebuild reused retained clean-file entries
 */
bool project_context_last_rebuild_was_incremental(const ProjectContext *project);

/**
 * @brief Files whose InfoBlocks were reused by the last registry rebuild (`WI-018`).
 *
 * @param project Project context
 * @return size_t Reused (unchanged) file count
 */
size_t project_context_last_info_block_reused_file_count(const ProjectContext *project);

/**
 * @brief Files whose InfoBlocks were re-derived by the last registry rebuild (`WI-018`).
 *
 * @param project Project context
 * @return size_t Recomputed file count
 */
size_t project_context_last_info_block_recomputed_file_count(const ProjectContext *project);

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
 * @brief Search the canonical InfoBlock registry using indexed text and relationships.
 *
 * @param project Project context
 * @param request Machine-readable search request
 * @param out_result Output result; caller must free with project_search_result_free()
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_search_info_blocks(ProjectContext *project,
                                        const ProjectSearchRequest *request,
                                        ProjectSearchResult *out_result);

/**
 * @brief Build a token-aware prompt package for downstream LLM or tool consumers.
 *
 * @param project Project context
 * @param request Prompt assembly request
 * @param out_result Output result; caller must free with project_prompt_assembly_result_free()
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_assemble_prompt(ProjectContext *project,
                                     const ProjectPromptAssemblyRequest *request,
                                     ProjectPromptAssemblyResult *out_result);

/**
 * @brief Free heap storage owned by a tiered context result.
 *
 * @param result Result to clear
 */
void project_tiered_context_result_free(ProjectTieredContextResult *result);

/**
 * @brief Free heap storage owned by a project search result.
 *
 * @param result Result to clear
 */
void project_search_result_free(ProjectSearchResult *result);

/**
 * @brief Free heap storage owned by a prompt assembly result.
 *
 * @param result Result to clear
 */
void project_prompt_assembly_result_free(ProjectPromptAssemblyResult *result);

/**
 * @brief Create a projected plan node and add it to the durable plan store (`WI-032`).
 *
 * The node id is `plan:<task_id>:<slug>`. Plan nodes are projection-only; the
 * external task record referenced by `task_id` stays authoritative. Creating a
 * node invalidates the derived InfoBlock registry so the next registry access
 * re-projects it.
 *
 * @param project Project context
 * @param task_id External task-record id
 * @param slug Stable, human-readable slug
 * @param kind Plan-node kind
 * @return ProjectPlanNode* New node, or NULL on invalid input, duplicate id, or
 * allocation failure
 */
ProjectPlanNode *project_context_plan_node_create(ProjectContext *project, const char *task_id,
                                                  const char *slug, ProjectPlanNodeKind kind);

/**
 * @brief Set a plan node's human-readable title.
 * @return bool True on success
 */
bool project_context_plan_node_set_title(ProjectContext *project, ProjectPlanNode *node,
                                         const char *title);

/**
 * @brief Set a plan node's desired shape (projected signature/structure).
 * @return bool True on success
 */
bool project_context_plan_node_set_desired_shape(ProjectContext *project, ProjectPlanNode *node,
                                                 const char *desired_shape);

/**
 * @brief Set a plan node's rationale, tied to the task's completion criteria.
 * @return bool True on success
 */
bool project_context_plan_node_set_rationale(ProjectContext *project, ProjectPlanNode *node,
                                             const char *rationale);

/**
 * @brief Set a plan node's provenance (task record/stage).
 * @return bool True on success
 */
bool project_context_plan_node_set_provenance(ProjectContext *project, ProjectPlanNode *node,
                                              const char *provenance);

/**
 * @brief Set the symbol expected to exist once the plan node is implemented.
 * @return bool True on success
 */
bool project_context_plan_node_set_projected_symbol(ProjectContext *project, ProjectPlanNode *node,
                                                    const char *symbol_name);

/**
 * @brief Set the observability subtype of an observability plan node (`WI-034`).
 *
 * @param project Project context
 * @param node Plan node
 * @param kind Observability subtype
 * @return bool True on success
 */
bool project_context_plan_node_set_observability_kind(ProjectContext *project, ProjectPlanNode *node,
                                                      ProjectObservabilityKind kind);

/**
 * @brief Set a plan node's projected file path.
 * @return bool True on success
 */
bool project_context_plan_node_set_file_path(ProjectContext *project, ProjectPlanNode *node,
                                             const char *file_path);

/**
 * @brief Set a plan node's lifecycle state.
 * @return bool True on success
 */
bool project_context_plan_node_set_lifecycle(ProjectContext *project, ProjectPlanNode *node,
                                             ProjectInfoBlockLifecycle lifecycle);

/**
 * @brief Set a plan node's confidence (projected/heuristic nodes are < 1.0).
 * @return bool True on success
 */
bool project_context_plan_node_set_confidence(ProjectContext *project, ProjectPlanNode *node,
                                              float confidence);

/**
 * @brief Add an anchor block id from the current-state registry to a plan node.
 *
 * Adding an already-present anchor is a no-op.
 *
 * @return bool True on success
 */
bool project_context_plan_node_add_anchor(ProjectContext *project, ProjectPlanNode *node,
                                          const char *anchor_block_id);

/**
 * @brief Find a plan node by its stable id.
 *
 * @param project Project context
 * @param plan_node_id Stable id such as `plan:TASK-1:add-parser`
 * @return ProjectPlanNode* Matching node or NULL if not found
 */
ProjectPlanNode *project_context_find_plan_node(ProjectContext *project, const char *plan_node_id);

/**
 * @brief Get the number of durable plan nodes.
 * @return size_t Plan-node count
 */
size_t project_context_get_plan_node_count(const ProjectContext *project);

/**
 * @brief Get a plan node by index.
 *
 * @param project Project context
 * @param index Plan-node index
 * @return const ProjectPlanNode* Node or NULL when out of range
 */
const ProjectPlanNode *project_context_get_plan_node_by_index(const ProjectContext *project,
                                                              size_t index);

/**
 * @brief Remove every durable plan node.
 *
 * This does not affect parsed state or the external task record.
 *
 * @param project Project context
 */
void project_context_clear_plan_nodes(ProjectContext *project);

/**
 * @brief Reconcile plan nodes against current parsed state (`WI-032`).
 *
 * Anchors that all vanish mark a node `STALE`; a partial anchor divergence marks
 * it `CONFLICT`; a projected symbol that now appears in parsed state marks the
 * node `IMPLEMENTED`. Nodes are never deleted and task state is never advanced.
 *
 * @param project Project context
 * @param out_result Output result; caller must free with
 * project_plan_node_reconciliation_result_free()
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_reconcile_plan_nodes(ProjectContext *project,
                                          ProjectPlanNodeReconciliationResult *out_result);

/**
 * @brief Free heap storage owned by a plan-node reconciliation result.
 *
 * This frees only the result's entries; plan nodes are owned by the project.
 *
 * @param result Result to clear
 */
void project_plan_node_reconciliation_result_free(ProjectPlanNodeReconciliationResult *result);

/**
 * @brief Serialize the durable plan-node store to a JSON string (`WI-031`).
 *
 * The durable store holds plan nodes, their lifecycle, provenance, anchors, and
 * confidence; it is separate from the disposable derived store (parse, IR,
 * InfoBlocks, graph, search index). The returned string is heap-allocated and
 * owned by the caller.
 *
 * @param project Project context
 * @return char* JSON text or NULL on allocation failure
 */
char *project_context_plan_nodes_to_json(const ProjectContext *project);

/**
 * @brief Load durable plan nodes from a JSON string (`WI-031`).
 *
 * When @p merge is false the existing plan-node store is replaced; when true,
 * loaded nodes are added or updated by id and other nodes are preserved. The
 * derived registry is invalidated so the next access re-projects.
 *
 * @param project Project context
 * @param json JSON text previously emitted by project_context_plan_nodes_to_json()
 * @param merge true to merge into existing nodes, false to replace
 * @return bool True on success, false on parse or allocation failure
 */
bool project_context_plan_nodes_from_json(ProjectContext *project, const char *json, bool merge);

/**
 * @brief Save the durable plan-node store to a file as JSON (`WI-031`).
 *
 * @param project Project context
 * @param path Destination file path
 * @return bool True on success, false on serialization or I/O failure
 */
bool project_context_plan_nodes_save(const ProjectContext *project, const char *path);

/**
 * @brief Load durable plan nodes from a JSON file (`WI-031`).
 *
 * @param project Project context
 * @param path Source file path
 * @param merge true to merge into existing nodes, false to replace
 * @return bool True on success, false on I/O, parse, or allocation failure
 */
bool project_context_plan_nodes_load(ProjectContext *project, const char *path, bool merge);

/**
 * @brief Delta entry kinds: `current (+) target = { add, change, remove, reuse }` (`WI-036`).
 */
typedef enum {
  PROJECT_DELTA_ADD = 0, ///< target node absent from current state
  PROJECT_DELTA_CHANGE,  ///< target modifies an existing unit
  PROJECT_DELTA_REMOVE,  ///< target removes a current unit
  PROJECT_DELTA_REUSE,   ///< target can reuse an existing (already realized) unit
} ProjectDeltaKind;

/**
 * @brief One machine-readable delta entry (`WI-036`).
 *
 * Pointer fields are borrowed from the project (registry/plan store) and stay
 * valid until the next registry rebuild or project destruction.
 */
typedef struct {
  ProjectDeltaKind kind;
  const ProjectInfoBlock *block;     ///< target block, or current block for reuse
  const ProjectPlanNode *plan_node;  ///< owning plan node when applicable
  const char *anchors;               ///< `;`-joined anchor ids (borrowed)
  const char *projected_shape;       ///< desired shape (borrowed)
  const char *provenance;            ///< provenance (borrowed)
  float confidence;
  ProjectInfoBlockLifecycle lifecycle;
  size_t estimated_tokens;
} ProjectDeltaEntry;

/**
 * @brief Machine-readable delta result (`WI-036`).
 */
typedef struct {
  const char *task_id; ///< borrowed task id filter (may be NULL)
  const char *stage;   ///< borrowed runtime stage (may be NULL)
  ProjectDeltaEntry *entries;
  size_t entry_count;
  size_t add_count;
  size_t change_count;
  size_t remove_count;
  size_t reuse_count;
  size_t estimated_tokens;
} ProjectDeltaResult;

/**
 * @brief Map query operations (`WI-036`).
 */
typedef enum {
  PROJECT_MAP_QUERY_NODE = 0,       ///< resolve one node by id
  PROJECT_MAP_QUERY_RESOLVE,        ///< seed nodes for a task/stage
  PROJECT_MAP_QUERY_EXPAND,         ///< expand a node toward a tier
  PROJECT_MAP_QUERY_NEIGHBORS,      ///< graph neighbors to a depth
  PROJECT_MAP_QUERY_DUPLICATES,     ///< duplicate/near-duplicate units
  PROJECT_MAP_QUERY_OBSERVABILITY,  ///< observability points for a symbol
  PROJECT_MAP_QUERY_CHANGE_IMPACT,  ///< impact of changed files
} ProjectMapQueryKind;

/**
 * @brief One machine-readable map query result item (`WI-036`).
 */
typedef struct {
  const ProjectInfoBlock *block; ///< matched block (borrowed)
  ProjectMapQueryKind kind;
  const char *reason;   ///< static reason label for reconstructing selection
  const char *provenance; ///< borrowed provenance
  float confidence;
  size_t estimated_tokens;
  size_t distance; ///< graph distance for neighbors/expand (0 = seed)
} ProjectMapResultItem;

/**
 * @brief Machine-readable map query result (`WI-036`).
 */
typedef struct {
  ProjectMapQueryKind kind;
  ProjectMapResultItem *items;
  size_t item_count;
  size_t estimated_tokens;
} ProjectMapQueryResult;

/**
 * @brief Compute the delta between current (parsed) and target (planned) state.
 *
 * Each plan node yields one entry: `ADD` for new units, `CHANGE` for
 * modifications/observability/refactor nodes, `REMOVE` for removals, and
 * `REUSE` when the projected symbol already exists in parsed state. Entries
 * carry anchors, projected shape, provenance, confidence, and token cost.
 * Passing a non-NULL @p task_id restricts the delta to that task.
 *
 * @param project Project context
 * @param task_id Task-record id filter, or NULL for all tasks
 * @param stage Runtime stage label (recorded, not interpreted), or NULL
 * @param out_result Output result; free with project_delta_result_free()
 * @return bool True on success, false on allocation or state failure
 */
bool project_context_compute_delta(ProjectContext *project, const char *task_id, const char *stage,
                                   ProjectDeltaResult *out_result);

/**
 * @brief Resolve a single canonical node by id.
 * @return bool True on success (even when no node matches)
 */
bool project_context_query_node(ProjectContext *project, const char *block_id,
                                ProjectMapQueryResult *out_result);

/**
 * @brief Resolve seed nodes for a task and stage.
 *
 * Returns the task's plan nodes and their anchors, falling back to the project
 * block when the task has no plan nodes.
 *
 * @return bool True on success
 */
bool project_context_query_resolve(ProjectContext *project, const char *task_id, const char *stage,
                                   ProjectMapQueryResult *out_result);

/**
 * @brief Expand a node toward a target tier across related blocks.
 * @return bool True on success
 */
bool project_context_query_expand(ProjectContext *project, const char *block_id,
                                  ProjectContextTier to_tier, ProjectMapQueryResult *out_result);

/**
 * @brief Walk graph neighbors from a node to a bounded depth.
 * @return bool True on success
 */
bool project_context_query_neighbors(ProjectContext *project, const char *block_id, size_t depth,
                                     ProjectMapQueryResult *out_result);

/**
 * @brief Detect duplicate units, optionally scoped by a path/name substring.
 *
 * @param scope Optional substring filter, or NULL for whole project
 * @return bool True on success
 */
bool project_context_query_duplicates(ProjectContext *project, const char *scope,
                                      ProjectMapQueryResult *out_result);

/**
 * @brief List observability plan nodes attached to a symbol.
 * @return bool True on success
 */
bool project_context_query_observability(ProjectContext *project, const char *symbol,
                                         ProjectMapQueryResult *out_result);

/**
 * @brief Report the impact of changed files: their blocks plus anchored plans.
 *
 * @param files Array of changed file paths
 * @param file_count Number of entries in @p files
 * @return bool True on success
 */
bool project_context_query_change_impact(ProjectContext *project, const char *const *files,
                                         size_t file_count, ProjectMapQueryResult *out_result);

/**
 * @brief Free heap storage owned by a map query result.
 * @param result Result to clear
 */
void project_map_query_result_free(ProjectMapQueryResult *result);

/**
 * @brief Free heap storage owned by a delta result.
 * @param result Result to clear
 */
void project_delta_result_free(ProjectDeltaResult *result);
#endif /* SCOPEMUX_PROJECT_CONTEXT_H */
