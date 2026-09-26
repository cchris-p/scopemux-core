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

static bool is_symbol_ir_node_type(ASTNodeType type) {
  switch (type) {
  case NODE_FUNCTION:
  case NODE_METHOD:
  case NODE_CLASS:
  case NODE_STRUCT:
  case NODE_ENUM:
  case NODE_VARIABLE:
  case NODE_TYPEDEF:
  case NODE_NAMESPACE:
  case NODE_INTERFACE:
  case NODE_MODULE:
  case NODE_MACRO:
  case NODE_USING:
  case NODE_FRIEND:
  case NODE_OPERATOR:
  case NODE_PROPERTY:
    return true;
  default:
    return false;
  }
}

static bool is_function_like_node_type(ASTNodeType type) {
  return type == NODE_FUNCTION || type == NODE_METHOD;
}

static const char *get_node_property_value(const ASTNode *node, const char *name) {
  if (!node || !name) {
    return NULL;
  }

  for (size_t i = 0; i < node->num_properties; i++) {
    if (node->property_names[i] && strcmp(node->property_names[i], name) == 0) {
      return node->property_values[i];
    }
  }

  return NULL;
}

static ProjectIRVisibility infer_node_visibility(const ASTNode *node) {
  const char *visibility = get_node_property_value(node, "visibility");
  if (visibility) {
    if (strcmp(visibility, "public") == 0) {
      return PROJECT_IR_VISIBILITY_PUBLIC;
    }
    if (strcmp(visibility, "private") == 0) {
      return PROJECT_IR_VISIBILITY_PRIVATE;
    }
    if (strcmp(visibility, "protected") == 0) {
      return PROJECT_IR_VISIBILITY_PROTECTED;
    }
    if (strcmp(visibility, "internal") == 0 || strcmp(visibility, "file") == 0) {
      return PROJECT_IR_VISIBILITY_INTERNAL;
    }
  }

  if (node && node->name && node->name[0] == '_') {
    return PROJECT_IR_VISIBILITY_PRIVATE;
  }

  if (node && node->raw_content && strstr(node->raw_content, "static") != NULL) {
    return PROJECT_IR_VISIBILITY_INTERNAL;
  }

  return PROJECT_IR_VISIBILITY_PUBLIC;
}

static ProjectDependencyKind dependency_kind_from_node(const ASTNode *node) {
  if (!node) {
    return PROJECT_DEPENDENCY_UNKNOWN;
  }
  if (node->type == NODE_INCLUDE) {
    return PROJECT_DEPENDENCY_INCLUDE;
  }
  if (node->type == NODE_IMPORT) {
    if (node->raw_content && strstr(node->raw_content, "require") != NULL) {
      return PROJECT_DEPENDENCY_REQUIRE;
    }
    return PROJECT_DEPENDENCY_IMPORT;
  }
  return PROJECT_DEPENDENCY_UNKNOWN;
}

static const char *dependency_specifier_from_node(const ASTNode *node) {
  if (!node) {
    return NULL;
  }
  if (node->type == NODE_IMPORT && node->name) {
    return node->name;
  }
  if ((node->type == NODE_IMPORT || node->type == NODE_INCLUDE) && node->raw_content) {
    return node->raw_content;
  }
  return node->name;
}

static const char *path_basename_ptr(const char *path) {
  const char *slash = path ? strrchr(path, '/') : NULL;
  return slash ? slash + 1 : path;
}

static bool basename_matches_specifier(const char *file_path, const char *specifier) {
  const char *basename = path_basename_ptr(file_path);
  size_t basename_len;
  size_t specifier_len;

  if (!basename || !specifier) {
    return false;
  }

  basename_len = strlen(basename);
  specifier_len = strlen(specifier);

  if (strcmp(basename, specifier) == 0) {
    return true;
  }
  if (strstr(specifier, basename) != NULL) {
    return true;
  }
  if (strstr(basename, specifier) != NULL) {
    return true;
  }

  if (basename_len > 0 && specifier_len > 0) {
    const char *dot = strrchr(basename, '.');
    size_t stem_len = dot ? (size_t)(dot - basename) : basename_len;
    if (stem_len == specifier_len && strncmp(basename, specifier, stem_len) == 0) {
      return true;
    }
  }

  return false;
}

static const char *resolve_dependency_target(const ParserContext *ctx, const ASTNode *node) {
  const char *specifier = dependency_specifier_from_node(node);

  if (!ctx || ctx->num_dependencies == 0) {
    return NULL;
  }

  if (specifier) {
    for (size_t i = 0; i < ctx->num_dependencies; i++) {
      if (ctx->dependencies[i] && ctx->dependencies[i]->filename &&
          basename_matches_specifier(ctx->dependencies[i]->filename, specifier)) {
        return ctx->dependencies[i]->filename;
      }
    }
  }

  if (ctx->num_dependencies == 1 && ctx->dependencies[0]) {
    return ctx->dependencies[0]->filename;
  }

  return NULL;
}

typedef struct {
  size_t symbol_count;
  size_t resolved_reference_count;
  size_t call_graph_edge_count;
  size_t dependency_count;
} ProjectIRCountState;

typedef struct {
  ProjectIRSnapshot *snapshot;
  size_t symbol_index;
  size_t resolved_reference_index;
  size_t call_graph_edge_index;
  size_t dependency_index;
} ProjectIRBuildState;

static void count_symbol_ir_nodes(const ASTNode *node, const ASTNode *owner_symbol,
                                  const ASTNode *owner_function, ProjectIRCountState *state) {
  const ASTNode *current_owner_symbol = owner_symbol;
  const ASTNode *current_owner_function = owner_function;

  if (!node || !state) {
    return;
  }

  if (is_symbol_ir_node_type(node->type)) {
    state->symbol_count++;
    current_owner_symbol = node;
  }

  if (is_function_like_node_type(node->type)) {
    current_owner_function = node;
  }

  if (current_owner_symbol) {
    for (size_t i = 0; i < node->num_references; i++) {
      const ASTNode *target = node->references[i];
      if (!target) {
        continue;
      }
      if (node == current_owner_symbol && target == current_owner_symbol) {
        continue;
      }

      state->resolved_reference_count++;

      if (current_owner_function && is_function_like_node_type(target->type) &&
          !(node == current_owner_function && target == current_owner_function)) {
        state->call_graph_edge_count++;
      }
    }
  }

  if (node->type == NODE_IMPORT || node->type == NODE_INCLUDE) {
    state->dependency_count++;
  }

  for (size_t i = 0; i < node->num_children; i++) {
    count_symbol_ir_nodes(node->children[i], current_owner_symbol, current_owner_function, state);
  }
}

static void append_file_relationship_edges(const ParserContext *ctx, ProjectIRBuildState *state) {
  if (!ctx || !state || !state->snapshot) {
    return;
  }

  for (size_t i = 0; i < ctx->num_dependencies; i++) {
    ParserContext *target = ctx->dependencies[i];
    ProjectDependencyIR *edge = &state->snapshot->dependencies[state->dependency_index++];
    edge->node = NULL;
    edge->source_file_path = ctx->filename;
    edge->target_file_path = target ? target->filename : NULL;
    edge->specifier = NULL;
    edge->kind = PROJECT_DEPENDENCY_FILE_RELATION;
  }
}

static size_t fill_symbol_ir_nodes(const ParserContext *ctx, const ASTNode *node,
                                   const char *file_path,
                                   const ASTNode *owner_symbol, size_t owner_symbol_index,
                                   const ASTNode *owner_function, ProjectIRBuildState *state) {
  const ASTNode *current_owner_symbol = owner_symbol;
  const ASTNode *current_owner_function = owner_function;
  size_t current_owner_symbol_index = owner_symbol_index;

  if (!node || !state || !state->snapshot) {
    return (size_t)-1;
  }

  if (is_symbol_ir_node_type(node->type)) {
    ProjectSymbolIR *symbol = &state->snapshot->symbols[state->symbol_index];
    current_owner_symbol_index = state->symbol_index;
    state->symbol_index++;
    symbol->node = node;
    symbol->name = node->name;
    symbol->qualified_name = node->qualified_name ? node->qualified_name : node->name;
    symbol->signature = node->signature;
    symbol->docstring = node->docstring;
    symbol->scope_qualified_name =
        (node->parent && node->parent->qualified_name) ? node->parent->qualified_name : NULL;
    symbol->file_path = node->file_path ? node->file_path : file_path;
    symbol->type = node->type;
    symbol->visibility = infer_node_visibility(node);
    symbol->resolved_reference_start = state->resolved_reference_index;
    symbol->resolved_reference_count = 0;
    current_owner_symbol = node;
  }

  if (is_function_like_node_type(node->type)) {
    current_owner_function = node;
  }

  if (current_owner_symbol) {
    for (size_t i = 0; i < node->num_references; i++) {
      const ASTNode *target = node->references[i];
      ProjectResolvedReferenceIR *reference;

      if (!target) {
        continue;
      }
      if (node == current_owner_symbol && target == current_owner_symbol) {
        continue;
      }

      reference = &state->snapshot->resolved_references[state->resolved_reference_index++];
      reference->owner_symbol_node = current_owner_symbol;
      reference->reference_node = node;
      reference->target_node = target;
      reference->owner_symbol =
          current_owner_symbol->qualified_name ? current_owner_symbol->qualified_name : current_owner_symbol->name;
      reference->target_symbol = target->qualified_name ? target->qualified_name : target->name;
      reference->target_file_path = target->file_path;

      if (current_owner_symbol_index != (size_t)-1) {
        state->snapshot->symbols[current_owner_symbol_index].resolved_reference_count++;
      }

      if (current_owner_function && is_function_like_node_type(target->type) &&
          !(node == current_owner_function && target == current_owner_function)) {
        ProjectCallGraphEdgeIR *edge =
            &state->snapshot->call_graph_edges[state->call_graph_edge_index++];
        edge->caller_node = current_owner_function;
        edge->callee_node = target;
        edge->callsite_node = node;
        edge->caller_symbol = current_owner_function->qualified_name ? current_owner_function->qualified_name
                                                                     : current_owner_function->name;
        edge->callee_symbol = target->qualified_name ? target->qualified_name : target->name;
        edge->caller_file_path = current_owner_function->file_path ? current_owner_function->file_path : file_path;
        edge->callee_file_path = target->file_path;
      }
    }
  }

  if (node->type == NODE_IMPORT || node->type == NODE_INCLUDE) {
    ProjectDependencyIR *edge = &state->snapshot->dependencies[state->dependency_index++];
    edge->node = node;
    edge->source_file_path = file_path;
    edge->target_file_path = resolve_dependency_target(ctx, node);
    edge->specifier = dependency_specifier_from_node(node);
    edge->kind = dependency_kind_from_node(node);
  }

  for (size_t i = 0; i < node->num_children; i++) {
    fill_symbol_ir_nodes(ctx, node->children[i], file_path, current_owner_symbol,
                         current_owner_symbol_index, current_owner_function, state);
  }

  return current_owner_symbol_index;
}

static void project_context_free_ir_arrays(ProjectContext *project) {
  for (size_t i = 0; i < project->ir_snapshot.file_range_count; i++) {
    free(project->ir_snapshot.file_ranges[i].file_path);
  }
  free(project->ir_snapshot.file_ranges);
  free(project->ir_snapshot.symbols);
  free(project->ir_snapshot.resolved_references);
  free(project->ir_snapshot.call_graph_edges);
  free(project->ir_snapshot.dependencies);
  memset(&project->ir_snapshot, 0, sizeof(project->ir_snapshot));
}

/**
 * @brief Find the retained array range owned by a clean file.
 *
 * Matching is by owned file path in the range table, never by reading fields of
 * retained entries, because entries owned by a recomputed file may point at a
 * freed parser context.
 */
static const ProjectFileIRRange *find_file_ir_range(const ProjectIRSnapshot *snapshot,
                                                    const char *file_path) {
  if (!snapshot || !file_path) {
    return NULL;
  }
  for (size_t i = 0; i < snapshot->file_range_count; i++) {
    const ProjectFileIRRange *range = &snapshot->file_ranges[i];
    if (range->file_path && strcmp(range->file_path, file_path) == 0) {
      return range;
    }
  }
  return NULL;
}

static void project_context_free_reverse_edges(ProjectContext *project) {
  for (size_t i = 0; i < project->reverse_edge_count; i++) {
    free(project->reverse_edges[i].source_file);
    free(project->reverse_edges[i].target_file);
  }
  free(project->reverse_edges);
  project->reverse_edges = NULL;
  project->reverse_edge_count = 0;
  project->reverse_edge_capacity = 0;
}

static void project_context_clear_dirty_files(ProjectContext *project) {
  for (size_t i = 0; i < project->dirty_count; i++) {
    free(project->dirty_files[i]);
  }
  free(project->dirty_files);
  project->dirty_files = NULL;
  project->dirty_count = 0;
  project->dirty_capacity = 0;
}

static bool dirty_files_contains(const ProjectContext *project, const char *path) {
  if (!project || !path) {
    return false;
  }
  for (size_t i = 0; i < project->dirty_count; i++) {
    if (project->dirty_files[i] && strcmp(project->dirty_files[i], path) == 0) {
      return true;
    }
  }
  return false;
}

static bool dirty_files_add(ProjectContext *project, const char *path) {
  char *copy;

  if (!project || !path || dirty_files_contains(project, path)) {
    return project != NULL;
  }

  if (project->dirty_count >= project->dirty_capacity) {
    size_t new_capacity = project->dirty_capacity > 0 ? project->dirty_capacity * 2 : 8;
    char **grown = (char **)realloc(project->dirty_files, new_capacity * sizeof(char *));
    if (!grown) {
      project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to grow dirty file set");
      return false;
    }
    project->dirty_files = grown;
    project->dirty_capacity = new_capacity;
  }

  copy = strdup(path);
  if (!copy) {
    project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to record dirty file");
    return false;
  }
  project->dirty_files[project->dirty_count++] = copy;
  return true;
}

static void reverse_edge_add(ProjectContext *project, const char *source, const char *target) {
  ProjectReverseEdge *edge;

  if (!project || !source || !target) {
    return;
  }

  if (project->reverse_edge_count >= project->reverse_edge_capacity) {
    size_t new_capacity = project->reverse_edge_capacity > 0 ? project->reverse_edge_capacity * 2 : 16;
    ProjectReverseEdge *grown =
        (ProjectReverseEdge *)realloc(project->reverse_edges, new_capacity * sizeof(ProjectReverseEdge));
    if (!grown) {
      return;
    }
    project->reverse_edges = grown;
    project->reverse_edge_capacity = new_capacity;
  }

  edge = &project->reverse_edges[project->reverse_edge_count];
  edge->source_file = strdup(source);
  edge->target_file = strdup(target);
  if (!edge->source_file || !edge->target_file) {
    free(edge->source_file);
    free(edge->target_file);
    return;
  }
  project->reverse_edge_count++;
}

/**
 * Rebuild the reverse-edge index from the current IR snapshot.
 *
 * Each retained reference and dependency edge contributes a
 * (target file -> source file) relation. The index is used by
 * @ref project_context_mark_file_dirty to expand a changed file into the set of
 * files that must be recomputed.
 */
static void project_context_rebuild_reverse_edges(ProjectContext *project) {
  const ProjectIRSnapshot *snapshot;

  if (!project) {
    return;
  }

  project_context_free_reverse_edges(project);
  snapshot = &project->ir_snapshot;

  for (size_t i = 0; i < snapshot->resolved_reference_count; i++) {
    const ProjectResolvedReferenceIR *ref = &snapshot->resolved_references[i];
    const char *source = NULL;
    if (ref->reference_node && ref->reference_node->file_path) {
      source = ref->reference_node->file_path;
    } else if (ref->owner_symbol_node && ref->owner_symbol_node->file_path) {
      source = ref->owner_symbol_node->file_path;
    }
    reverse_edge_add(project, source, ref->target_file_path);
  }

  for (size_t i = 0; i < snapshot->dependency_count; i++) {
    const ProjectDependencyIR *dep = &snapshot->dependencies[i];
    reverse_edge_add(project, dep->source_file_path, dep->target_file_path);
  }
}

void project_context_mark_file_dirty(ProjectContext *project, const char *filepath) {
  char normalized[1024];
  const char *path = filepath;
  bool changed = true;

  if (!project || !filepath) {
    return;
  }

  if (normalize_file_path(project->root_directory, filepath, normalized, sizeof(normalized))) {
    path = normalized;
  }

  if (!dirty_files_add(project, path)) {
    return;
  }

  // Walk reverse edges to a fixpoint: anything depending on or referencing a
  // dirty file is itself dirty.
  while (changed) {
    changed = false;
    for (size_t i = 0; i < project->reverse_edge_count; i++) {
      const ProjectReverseEdge *edge = &project->reverse_edges[i];
      if (!edge->source_file || !edge->target_file) {
        continue;
      }
      if (dirty_files_contains(project, edge->target_file) &&
          !dirty_files_contains(project, edge->source_file)) {
        if (!dirty_files_add(project, edge->source_file)) {
          return;
        }
        changed = true;
      }
    }
  }

  project->ir_ready = false;
  project_context_clear_info_blocks(project);
}

void project_context_clear_ir(ProjectContext *project) {
  if (!project) {
    return;
  }

  project_context_clear_info_blocks(project);
  project_context_free_ir_arrays(project);
  project_context_free_reverse_edges(project);
  project_context_clear_dirty_files(project);

  project->ir_snapshot_retained = false;
  project->last_recomputed_file_count = 0;
  project->last_rebuild_file_count = 0;
  project->last_rebuild_incremental = false;
  project->ir_ready = false;
}

static bool project_context_rebuild_ir_full(ProjectContext *project) {
  ProjectIRCountState counts = {0};
  ProjectIRBuildState state;
  size_t file_count = 0;

  project_context_clear_ir(project);

  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    if (!ctx) {
      continue;
    }

    file_count++;
    counts.dependency_count += ctx->num_dependencies;
    for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
      count_symbol_ir_nodes(ctx->all_ast_nodes[j], NULL, NULL, &counts);
    }
  }

  if (counts.symbol_count > 0) {
    project->ir_snapshot.symbols = calloc(counts.symbol_count, sizeof(ProjectSymbolIR));
  }
  if (counts.resolved_reference_count > 0) {
    project->ir_snapshot.resolved_references =
        calloc(counts.resolved_reference_count, sizeof(ProjectResolvedReferenceIR));
  }
  if (counts.call_graph_edge_count > 0) {
    project->ir_snapshot.call_graph_edges =
        calloc(counts.call_graph_edge_count, sizeof(ProjectCallGraphEdgeIR));
  }
  if (counts.dependency_count > 0) {
    project->ir_snapshot.dependencies = calloc(counts.dependency_count, sizeof(ProjectDependencyIR));
  }

  if ((counts.symbol_count > 0 && !project->ir_snapshot.symbols) ||
      (counts.resolved_reference_count > 0 && !project->ir_snapshot.resolved_references) ||
      (counts.call_graph_edge_count > 0 && !project->ir_snapshot.call_graph_edges) ||
      (counts.dependency_count > 0 && !project->ir_snapshot.dependencies)) {
    project_context_clear_ir(project);
    project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate project IR snapshot");
    return false;
  }

  if (file_count > 0) {
    project->ir_snapshot.file_ranges = calloc(file_count, sizeof(ProjectFileIRRange));
    if (!project->ir_snapshot.file_ranges) {
      project_context_clear_ir(project);
      project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate project IR file ranges");
      return false;
    }
  }

  state.snapshot = &project->ir_snapshot;
  state.symbol_index = 0;
  state.resolved_reference_index = 0;
  state.call_graph_edge_index = 0;
  state.dependency_index = 0;

  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    ProjectFileIRRange *range;

    if (!ctx) {
      continue;
    }

    range = &project->ir_snapshot.file_ranges[project->ir_snapshot.file_range_count];
    range->file_path = strdup(ctx->filename ? ctx->filename : "");
    range->symbol_start = state.symbol_index;
    range->reference_start = state.resolved_reference_index;
    range->call_edge_start = state.call_graph_edge_index;
    range->dependency_start = state.dependency_index;

    for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
      fill_symbol_ir_nodes(ctx, ctx->all_ast_nodes[j], ctx->filename, NULL, (size_t)-1, NULL,
                           &state);
    }
    append_file_relationship_edges(ctx, &state);

    range->symbol_count = state.symbol_index - range->symbol_start;
    range->reference_count = state.resolved_reference_index - range->reference_start;
    range->call_edge_count = state.call_graph_edge_index - range->call_edge_start;
    range->dependency_count = state.dependency_index - range->dependency_start;
    project->ir_snapshot.file_range_count++;
  }

  project->ir_snapshot.symbol_count = state.symbol_index;
  project->ir_snapshot.resolved_reference_count = state.resolved_reference_index;
  project->ir_snapshot.call_graph_edge_count = state.call_graph_edge_index;
  project->ir_snapshot.dependency_count = state.dependency_index;
  project->total_symbols = state.symbol_index;
  project->total_references = state.resolved_reference_index;
  project->unresolved_references = 0;
  project->ir_ready = true;
  project->ir_snapshot_retained = true;
  project->last_recomputed_file_count = file_count;
  project->last_rebuild_file_count = file_count;
  project->last_rebuild_incremental = false;

  project_context_rebuild_reverse_edges(project);
  return true;
}

/**
 * Copy the retained IR entries owned by one clean file range into the next
 * snapshot.
 *
 * A file is only clean if it is not in the dirty set, so none of its entries
 * point at a recomputed file. Its symbols' reference ranges are re-based onto
 * the next snapshot's reference array, preserving per-file contiguity. Only the
 * file's own range is read, so no field of an entry owned by a recomputed
 * (possibly freed) file is touched.
 */
static void copy_retained_file_range(const ProjectIRSnapshot *old, const ProjectFileIRRange *range,
                                     ProjectIRSnapshot *next, ProjectIRBuildState *state) {
  for (size_t k = 0; k < range->symbol_count; k++) {
    const ProjectSymbolIR *symbol = &old->symbols[range->symbol_start + k];
    ProjectSymbolIR copy = *symbol;
    size_t old_reference_start = symbol->resolved_reference_start;

    copy.resolved_reference_start = state->resolved_reference_index;
    next->symbols[state->symbol_index++] = copy;

    for (size_t r = 0; r < symbol->resolved_reference_count; r++) {
      next->resolved_references[state->resolved_reference_index++] =
          old->resolved_references[old_reference_start + r];
    }
  }

  for (size_t k = 0; k < range->call_edge_count; k++) {
    next->call_graph_edges[state->call_graph_edge_index++] =
        old->call_graph_edges[range->call_edge_start + k];
  }

  for (size_t k = 0; k < range->dependency_count; k++) {
    next->dependencies[state->dependency_index++] =
        old->dependencies[range->dependency_start + k];
  }
}

static bool project_context_rebuild_ir_incremental(ProjectContext *project) {
  ProjectIRSnapshot old = project->ir_snapshot;
  ProjectIRSnapshot next;
  ProjectIRCountState counts = {0};
  ProjectIRBuildState state;
  size_t file_count = 0;
  size_t recomputed = 0;

  memset(&next, 0, sizeof(next));

  // Count pass: recompute dirty files, retain counts for clean file ranges.
  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    const ProjectFileIRRange *range;

    if (!ctx || !ctx->filename) {
      continue;
    }

    file_count++;
    range = dirty_files_contains(project, ctx->filename)
                ? NULL
                : find_file_ir_range(&old, ctx->filename);
    if (!range) {
      recomputed++;
      counts.dependency_count += ctx->num_dependencies;
      for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
        count_symbol_ir_nodes(ctx->all_ast_nodes[j], NULL, NULL, &counts);
      }
    } else {
      counts.symbol_count += range->symbol_count;
      counts.resolved_reference_count += range->reference_count;
      counts.call_graph_edge_count += range->call_edge_count;
      counts.dependency_count += range->dependency_count;
    }
  }

  if (counts.symbol_count > 0) {
    next.symbols = calloc(counts.symbol_count, sizeof(ProjectSymbolIR));
  }
  if (counts.resolved_reference_count > 0) {
    next.resolved_references =
        calloc(counts.resolved_reference_count, sizeof(ProjectResolvedReferenceIR));
  }
  if (counts.call_graph_edge_count > 0) {
    next.call_graph_edges = calloc(counts.call_graph_edge_count, sizeof(ProjectCallGraphEdgeIR));
  }
  if (counts.dependency_count > 0) {
    next.dependencies = calloc(counts.dependency_count, sizeof(ProjectDependencyIR));
  }

  if ((counts.symbol_count > 0 && !next.symbols) ||
      (counts.resolved_reference_count > 0 && !next.resolved_references) ||
      (counts.call_graph_edge_count > 0 && !next.call_graph_edges) ||
      (counts.dependency_count > 0 && !next.dependencies)) {
    free(next.symbols);
    free(next.resolved_references);
    free(next.call_graph_edges);
    free(next.dependencies);
    project_set_error(project, PROJECT_ERROR_MEMORY, "Failed to allocate incremental IR snapshot");
    return false;
  }

  if (file_count > 0) {
    next.file_ranges = calloc(file_count, sizeof(ProjectFileIRRange));
    if (!next.file_ranges) {
      free(next.symbols);
      free(next.resolved_references);
      free(next.call_graph_edges);
      free(next.dependencies);
      project_set_error(project, PROJECT_ERROR_MEMORY,
                        "Failed to allocate incremental IR file ranges");
      return false;
    }
  }

  state.snapshot = &next;
  state.symbol_index = 0;
  state.resolved_reference_index = 0;
  state.call_graph_edge_index = 0;
  state.dependency_index = 0;

  // Fill pass, in file order, so the result matches a full rebuild's ordering.
  for (size_t i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    const ProjectFileIRRange *range;
    ProjectFileIRRange *new_range;

    if (!ctx || !ctx->filename) {
      continue;
    }

    range = dirty_files_contains(project, ctx->filename)
                ? NULL
                : find_file_ir_range(&old, ctx->filename);

    new_range = &next.file_ranges[next.file_range_count];
    new_range->file_path = strdup(ctx->filename);
    new_range->symbol_start = state.symbol_index;
    new_range->reference_start = state.resolved_reference_index;
    new_range->call_edge_start = state.call_graph_edge_index;
    new_range->dependency_start = state.dependency_index;

    if (range) {
      copy_retained_file_range(&old, range, &next, &state);
    } else {
      for (size_t j = 0; j < ctx->num_ast_nodes; j++) {
        fill_symbol_ir_nodes(ctx, ctx->all_ast_nodes[j], ctx->filename, NULL, (size_t)-1, NULL,
                             &state);
      }
      append_file_relationship_edges(ctx, &state);
    }

    new_range->symbol_count = state.symbol_index - new_range->symbol_start;
    new_range->reference_count = state.resolved_reference_index - new_range->reference_start;
    new_range->call_edge_count = state.call_graph_edge_index - new_range->call_edge_start;
    new_range->dependency_count = state.dependency_index - new_range->dependency_start;
    next.file_range_count++;
  }

  next.symbol_count = state.symbol_index;
  next.resolved_reference_count = state.resolved_reference_index;
  next.call_graph_edge_count = state.call_graph_edge_index;
  next.dependency_count = state.dependency_index;

  project_context_free_ir_arrays(project);
  project->ir_snapshot = next;
  project->total_symbols = state.symbol_index;
  project->total_references = state.resolved_reference_index;
  project->unresolved_references = 0;
  project->ir_ready = true;
  project->ir_snapshot_retained = true;
  project->last_recomputed_file_count = recomputed;
  project->last_rebuild_file_count = file_count;
  project->last_rebuild_incremental = true;

  project_context_clear_dirty_files(project);
  project_context_rebuild_reverse_edges(project);
  return true;
}

bool project_context_rebuild_ir(ProjectContext *project) {
  if (!project) {
    return false;
  }

  if (project->dirty_count > 0 && project->ir_snapshot_retained) {
    return project_context_rebuild_ir_incremental(project);
  }

  return project_context_rebuild_ir_full(project);
}

size_t project_context_last_recomputed_file_count(const ProjectContext *project) {
  return project ? project->last_recomputed_file_count : 0;
}

size_t project_context_last_rebuild_file_count(const ProjectContext *project) {
  return project ? project->last_rebuild_file_count : 0;
}

bool project_context_last_rebuild_was_incremental(const ProjectContext *project) {
  return project ? project->last_rebuild_incremental : false;
}

const ProjectIRSnapshot *project_context_get_ir(const ProjectContext *project) {
  if (!project || !project->ir_ready) {
    return NULL;
  }

  return &project->ir_snapshot;
}

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
  // project_remove_file_impl marks the removed file dirty so the next rebuild
  // drops its retained nodes and recomputes its dependents and referrers.
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
  bool added = project_add_dependency(project, source_file, target_file);
  if (added) {
    // The source file's dependency edges changed; mark it dirty so the next
    // rebuild recomputes it (and its dependents) instead of the whole project.
    project_context_mark_file_dirty(project, source_file);
  }
  return added;
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
  bool added = project_add_file(project, filepath, language);
  if (added) {
    project_context_clear_ir(project);
  }
  return added;
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
  bool parsed = project_parse_all_files_impl(project);
  if (parsed) {
    // Bulk parsing can introduce files that were never marked dirty; a full
    // rebuild is required so none are skipped.
    project_context_clear_ir(project);
    return project_context_rebuild_ir(project);
  }
  return false;
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
  bool resolved = project_resolve_references_impl(project);
  if (resolved) {
    // Reference resolution mutates AST references across all files, so every
    // file's derived reference entries must be recomputed.
    project_context_clear_ir(project);
    return project_context_rebuild_ir(project);
  }
  return false;
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
