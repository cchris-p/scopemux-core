#define _POSIX_C_SOURCE 200809L

#include "project_context_internal.h"
#include "scopemux/project_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  ProjectTieredContextSelection *items;
  size_t count;
  size_t capacity;
  size_t estimated_tokens;
} SelectionBuilder;

struct ProjectSearchIndexEntry {
  const ProjectInfoBlock *block;
  char *normalized_text;
  const ProjectInfoBlock **related_blocks;
  size_t related_block_count;
  size_t related_block_capacity;
};

typedef struct {
  ProjectSearchHit hit;
  size_t original_index;
} SearchHitRank;

static char *dup_printf(const char *format, const char *value) {
  int needed;
  char *buffer;

  if (!format || !value) {
    return NULL;
  }

  needed = snprintf(NULL, 0, format, value);
  if (needed < 0) {
    return NULL;
  }

  buffer = malloc((size_t)needed + 1);
  if (!buffer) {
    return NULL;
  }

  snprintf(buffer, (size_t)needed + 1, format, value);
  return buffer;
}

static char *dup_printf_indexed(const char *format, const char *value, size_t index) {
  int needed;
  char *buffer;

  if (!format || !value) {
    return NULL;
  }

  needed = snprintf(NULL, 0, format, value, index);
  if (needed < 0) {
    return NULL;
  }

  buffer = malloc((size_t)needed + 1);
  if (!buffer) {
    return NULL;
  }

  snprintf(buffer, (size_t)needed + 1, format, value, index);
  return buffer;
}

static const char *path_basename_ptr(const char *path) {
  const char *slash = path ? strrchr(path, '/') : NULL;
  return slash ? slash + 1 : path;
}

static char *path_dirname_dup(const char *path) {
  const char *slash;
  size_t len;
  char *result;

  if (!path || path[0] == '\0') {
    return strdup(".");
  }

  slash = strrchr(path, '/');
  if (!slash) {
    return strdup(".");
  }

  if (slash == path) {
    return strdup("/");
  }

  len = (size_t)(slash - path);
  result = malloc(len + 1);
  if (!result) {
    return NULL;
  }

  memcpy(result, path, len);
  result[len] = '\0';
  return result;
}

static Language language_for_node(const ASTNode *node) {
  return node ? node->lang : LANG_UNKNOWN;
}

static size_t estimate_tokens_for_text(const char *text) {
  size_t len;

  if (!text || text[0] == '\0') {
    return 0;
  }

  len = strlen(text);
  return (len / 4U) + ((len % 4U) != 0U ? 1U : 0U);
}

static char lower_ascii_char(char ch) {
  if (ch >= 'A' && ch <= 'Z') {
    return (char)(ch - 'A' + 'a');
  }
  return ch;
}

static char *normalize_text_dup(const char *text) {
  size_t len;
  char *normalized;

  if (!text) {
    return strdup("");
  }

  len = strlen(text);
  normalized = malloc(len + 1);
  if (!normalized) {
    return NULL;
  }

  for (size_t i = 0; i < len; i++) {
    normalized[i] = lower_ascii_char(text[i]);
  }
  normalized[len] = '\0';
  return normalized;
}

static bool contains_normalized_text(const char *haystack, const char *needle) {
  return haystack && needle && needle[0] != '\0' && strstr(haystack, needle) != NULL;
}

static const char *kind_label(ProjectInfoBlockKind kind) {
  switch (kind) {
  case PROJECT_INFO_BLOCK_SYMBOL:
    return "symbol";
  case PROJECT_INFO_BLOCK_REFERENCE:
    return "reference";
  case PROJECT_INFO_BLOCK_FILE:
    return "file";
  case PROJECT_INFO_BLOCK_DIRECTORY:
    return "directory";
  case PROJECT_INFO_BLOCK_PROJECT:
    return "project";
  default:
    return "unknown";
  }
}

static const char *plan_kind_label(ProjectPlanNodeKind kind) {
  switch (kind) {
  case PROJECT_PLAN_NODE_NEW_SYMBOL:
    return "new_symbol";
  case PROJECT_PLAN_NODE_NEW_FILE:
    return "new_file";
  case PROJECT_PLAN_NODE_NEW_MODULE:
    return "new_module";
  case PROJECT_PLAN_NODE_NEW_TEST:
    return "new_test";
  case PROJECT_PLAN_NODE_MODIFY_SYMBOL:
    return "modify_symbol";
  case PROJECT_PLAN_NODE_REMOVE:
    return "remove";
  case PROJECT_PLAN_NODE_CONSOLIDATE:
    return "consolidate";
  case PROJECT_PLAN_NODE_REFACTOR_OPPORTUNITY:
    return "refactor_opportunity";
  case PROJECT_PLAN_NODE_OBSERVABILITY_POINT:
    return "observability_point";
  default:
    return "unknown";
  }
}

static ProjectInfoBlockKind plan_block_kind(ProjectPlanNodeKind kind) {
  switch (kind) {
  case PROJECT_PLAN_NODE_NEW_FILE:
    return PROJECT_INFO_BLOCK_FILE;
  case PROJECT_PLAN_NODE_NEW_MODULE:
    return PROJECT_INFO_BLOCK_DIRECTORY;
  default:
    return PROJECT_INFO_BLOCK_SYMBOL;
  }
}

static ProjectContextTier plan_block_tier(ProjectPlanNodeKind kind) {
  switch (kind) {
  case PROJECT_PLAN_NODE_NEW_FILE:
    return PROJECT_CONTEXT_TIER_2;
  case PROJECT_PLAN_NODE_NEW_MODULE:
    return PROJECT_CONTEXT_TIER_3;
  default:
    return PROJECT_CONTEXT_TIER_1;
  }
}

static ASTNodeType plan_block_node_type(ProjectPlanNodeKind kind) {
  switch (kind) {
  case PROJECT_PLAN_NODE_NEW_FILE:
  case PROJECT_PLAN_NODE_NEW_MODULE:
    return NODE_MODULE;
  default:
    return NODE_FUNCTION;
  }
}

static char *join_string_list(char **values, size_t count, char separator) {
  size_t total = 0;
  size_t offset = 0;
  char *joined;

  if (!values || count == 0) {
    return NULL;
  }

  for (size_t i = 0; i < count; i++) {
    total += values[i] ? strlen(values[i]) : 0;
  }
  total += count - 1;

  joined = malloc(total + 1);
  if (!joined) {
    return NULL;
  }

  for (size_t i = 0; i < count; i++) {
    size_t len = values[i] ? strlen(values[i]) : 0;
    if (values[i]) {
      memcpy(joined + offset, values[i], len);
      offset += len;
    }
    if (i + 1 < count) {
      joined[offset++] = separator;
    }
  }
  joined[offset] = '\0';
  return joined;
}

static const char *disposition_label(ProjectTieredContextDisposition disposition) {
  switch (disposition) {
  case PROJECT_CONTEXT_BLOCK_PINNED:
    return "pinned";
  case PROJECT_CONTEXT_BLOCK_SUMMARIZED:
    return "summarized";
  case PROJECT_CONTEXT_BLOCK_EXPANDED:
  default:
    return "expanded";
  }
}

static size_t summarized_block_tokens(const ProjectInfoBlock *block) {
  size_t reduced;

  if (!block) {
    return 0;
  }

  reduced = block->estimated_tokens / 4U;
  if (reduced < 8U) {
    reduced = 8U;
  }
  if (reduced > block->estimated_tokens && block->estimated_tokens > 0U) {
    reduced = block->estimated_tokens;
  }
  return reduced;
}

static bool append_text_part(char *buffer, size_t buffer_size, size_t *offset, const char *label,
                             const char *value) {
  int written;

  if (!buffer || !offset || !label || !value || value[0] == '\0') {
    return true;
  }

  written = snprintf(buffer + *offset, buffer_size - *offset, "%s%s\n", label, value);
  if (written < 0 || (size_t)written >= buffer_size - *offset) {
    return false;
  }

  *offset += (size_t)written;
  return true;
}

static const struct ProjectSearchIndexEntry *find_search_entry(const ProjectContext *project,
                                                               const ProjectInfoBlock *block) {
  size_t i;

  if (!project || !block || !project->search_index_entries) {
    return NULL;
  }

  for (i = 0; i < project->search_index_entry_count; i++) {
    if (project->search_index_entries[i].block == block) {
      return &project->search_index_entries[i];
    }
  }

  return NULL;
}

static bool search_entry_add_related(struct ProjectSearchIndexEntry *entry,
                                     const ProjectInfoBlock *related_block) {
  const ProjectInfoBlock **next_related;
  size_t next_capacity;

  if (!entry || !related_block || entry->block == related_block) {
    return true;
  }

  for (size_t i = 0; i < entry->related_block_count; i++) {
    if (entry->related_blocks[i] == related_block) {
      return true;
    }
  }

  if (entry->related_block_count == entry->related_block_capacity) {
    next_capacity = entry->related_block_capacity == 0 ? 8 : entry->related_block_capacity * 2;
    next_related = realloc(entry->related_blocks, next_capacity * sizeof(*entry->related_blocks));
    if (!next_related) {
      return false;
    }
    entry->related_blocks = next_related;
    entry->related_block_capacity = next_capacity;
  }

  entry->related_blocks[entry->related_block_count++] = related_block;
  return true;
}

static bool link_related_blocks(ProjectContext *project, const ProjectInfoBlock *source,
                                const ProjectInfoBlock *target) {
  struct ProjectSearchIndexEntry *source_entry;
  struct ProjectSearchIndexEntry *target_entry;

  if (!project || !source || !target) {
    return true;
  }

  source_entry = (struct ProjectSearchIndexEntry *)find_search_entry(project, source);
  target_entry = (struct ProjectSearchIndexEntry *)find_search_entry(project, target);
  if (!source_entry || !target_entry) {
    return true;
  }

  return search_entry_add_related(source_entry, target) &&
         search_entry_add_related(target_entry, source);
}

static bool build_search_text_for_block(const ProjectInfoBlock *block, char **out_text) {
  size_t size = 256;
  size_t offset = 0;
  char *buffer;

  if (!out_text) {
    return false;
  }

  *out_text = NULL;
  buffer = calloc(size, 1);
  if (!buffer) {
    return false;
  }

  if (!append_text_part(buffer, size, &offset, "id:", block && block->id ? block->id : "") ||
      !append_text_part(buffer, size, &offset, "name:", block && block->name ? block->name : "") ||
      !append_text_part(buffer, size, &offset, "qualified:",
                        block && block->qualified_name ? block->qualified_name : "") ||
      !append_text_part(buffer, size, &offset, "file:",
                        block && block->file_path ? block->file_path : "") ||
      !append_text_part(buffer, size, &offset, "kind:", kind_label(block ? block->kind : 0))) {
    free(buffer);
    return false;
  }

  if (block && block->node) {
    if (!append_text_part(buffer, size, &offset, "signature:",
                          block->node->signature ? block->node->signature : "") ||
        !append_text_part(buffer, size, &offset, "doc:",
                          block->node->docstring ? block->node->docstring : "") ||
        !append_text_part(buffer, size, &offset, "content:",
                          block->node->raw_content ? block->node->raw_content : "")) {
      free(buffer);
      return false;
    }
  }

  // WI-032: projected plan-node attributes are searchable alongside parsed ones.
  if (block && block->origin == PROJECT_INFO_BLOCK_ORIGIN_PLANNED) {
    if (!append_text_part(buffer, size, &offset, "plan_kind:", plan_kind_label(block->plan_kind)) ||
        !append_text_part(buffer, size, &offset, "desired:",
                          block->desired_shape ? block->desired_shape : "") ||
        !append_text_part(buffer, size, &offset, "rationale:",
                          block->rationale ? block->rationale : "") ||
        !append_text_part(buffer, size, &offset, "anchors:",
                          block->anchor_list ? block->anchor_list : "")) {
      free(buffer);
      return false;
    }
  }

  *out_text = normalize_text_dup(buffer);
  free(buffer);
  return *out_text != NULL;
}

static int compare_search_hits_desc(const void *left, const void *right) {
  const SearchHitRank *a = left;
  const SearchHitRank *b = right;

  if (a->hit.score != b->hit.score) {
    return a->hit.score < b->hit.score ? 1 : -1;
  }
  if (a->hit.block && b->hit.block && a->hit.block->tier != b->hit.block->tier) {
    return a->hit.block->tier > b->hit.block->tier ? 1 : -1;
  }
  if (a->original_index != b->original_index) {
    return a->original_index > b->original_index ? 1 : -1;
  }
  return 0;
}

static void project_context_clear_search_index(ProjectContext *project) {
  size_t i;

  if (!project) {
    return;
  }

  for (i = 0; i < project->search_index_entry_count; i++) {
    free(project->search_index_entries[i].normalized_text);
    free(project->search_index_entries[i].related_blocks);
  }
  free(project->search_index_entries);
  project->search_index_entries = NULL;
  project->search_index_entry_count = 0;
  project->search_index_ready = false;
}

static size_t estimate_tokens_for_node(const ASTNode *node) {
  size_t total = 0;

  if (!node) {
    return 0;
  }

  total += estimate_tokens_for_text(node->signature);
  total += estimate_tokens_for_text(node->docstring);
  total += estimate_tokens_for_text(node->raw_content);
  if (total == 0) {
    total += estimate_tokens_for_text(node->qualified_name ? node->qualified_name : node->name);
  }
  if (total == 0) {
    total = 1;
  }
  return total;
}

static ProjectContextTier tier_for_symbol_node_type(ASTNodeType type) {
  switch (type) {
  case NODE_MODULE:
  case NODE_NAMESPACE:
    return PROJECT_CONTEXT_TIER_2;
  default:
    return PROJECT_CONTEXT_TIER_1;
  }
}

static bool block_in_requested_tier(const ProjectInfoBlock *block,
                                    const ProjectTieredContextRequest *request) {
  if (!block || !request) {
    return false;
  }

  if (request->origin_mask != 0 &&
      (request->origin_mask & (1u << (unsigned)block->origin)) == 0) {
    return false;
  }

  if (request->lifecycle_mask != 0 &&
      (request->lifecycle_mask & (1u << (unsigned)block->lifecycle)) == 0) {
    return false;
  }

  return block->tier >= request->min_tier && block->tier <= request->max_tier;
}

static bool id_in_list(const char *id, const char **values, size_t count) {
  size_t i;

  if (!id || !values) {
    return false;
  }

  for (i = 0; i < count; i++) {
    if (values[i] && strcmp(values[i], id) == 0) {
      return true;
    }
  }

  return false;
}

static const ProjectInfoBlock *find_block_by_id_in_registry(const ProjectInfoBlockRegistry *registry,
                                                            const char *block_id) {
  size_t i;

  if (!registry || !block_id) {
    return NULL;
  }

  for (i = 0; i < registry->block_count; i++) {
    if (registry->blocks[i].id && strcmp(registry->blocks[i].id, block_id) == 0) {
      return &registry->blocks[i];
    }
  }

  return NULL;
}

static const ProjectInfoBlock *find_symbol_block(const ProjectInfoBlockRegistry *registry,
                                                 const char *symbol_name) {
  size_t i;

  if (!registry || !symbol_name) {
    return NULL;
  }

  for (i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->kind != PROJECT_INFO_BLOCK_SYMBOL) {
      continue;
    }
    if (block->qualified_name && strcmp(block->qualified_name, symbol_name) == 0) {
      return block;
    }
    if (block->name && strcmp(block->name, symbol_name) == 0) {
      return block;
    }
  }

  return NULL;
}

static const ProjectInfoBlock *find_file_block(const ProjectInfoBlockRegistry *registry,
                                               const char *file_path) {
  size_t i;

  if (!registry || !file_path) {
    return NULL;
  }

  for (i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->kind == PROJECT_INFO_BLOCK_FILE && block->file_path &&
        strcmp(block->file_path, file_path) == 0) {
      return block;
    }
  }

  return NULL;
}

static const ProjectInfoBlock *find_directory_block(const ProjectInfoBlockRegistry *registry,
                                                    const char *dir_path) {
  size_t i;

  if (!registry || !dir_path) {
    return NULL;
  }

  for (i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->kind == PROJECT_INFO_BLOCK_DIRECTORY && block->qualified_name &&
        strcmp(block->qualified_name, dir_path) == 0) {
      return block;
    }
  }

  return NULL;
}

static const ProjectInfoBlock *find_project_block(const ProjectInfoBlockRegistry *registry) {
  size_t i;

  if (!registry) {
    return NULL;
  }

  for (i = 0; i < registry->block_count; i++) {
    if (registry->blocks[i].kind == PROJECT_INFO_BLOCK_PROJECT) {
      return &registry->blocks[i];
    }
  }

  return NULL;
}

static bool reserve_selection_capacity(SelectionBuilder *builder, size_t min_capacity) {
  ProjectTieredContextSelection *next_items;
  size_t next_capacity;

  if (!builder) {
    return false;
  }

  if (builder->capacity >= min_capacity) {
    return true;
  }

  next_capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;
  while (next_capacity < min_capacity) {
    next_capacity *= 2;
  }

  next_items = realloc(builder->items, next_capacity * sizeof(*builder->items));
  if (!next_items) {
    return false;
  }

  builder->items = next_items;
  builder->capacity = next_capacity;
  return true;
}

static bool add_selection(SelectionBuilder *builder, const ProjectInfoBlock *block,
                          ProjectTieredContextDisposition disposition, bool from_focus) {
  size_t i;

  if (!builder || !block) {
    return false;
  }

  for (i = 0; i < builder->count; i++) {
    if (builder->items[i].block == block) {
      if (disposition == PROJECT_CONTEXT_BLOCK_PINNED) {
        builder->items[i].disposition = PROJECT_CONTEXT_BLOCK_PINNED;
      } else if (builder->items[i].disposition != PROJECT_CONTEXT_BLOCK_PINNED) {
        builder->items[i].disposition = disposition;
      }
      builder->items[i].from_focus = builder->items[i].from_focus || from_focus;
      return true;
    }
  }

  if (!reserve_selection_capacity(builder, builder->count + 1)) {
    return false;
  }

  builder->items[builder->count].block = block;
  builder->items[builder->count].disposition = disposition;
  builder->items[builder->count].from_focus = from_focus;
  builder->count++;
  builder->estimated_tokens += block->estimated_tokens;
  return true;
}

static bool append_block_if_allowed(SelectionBuilder *builder, const ProjectInfoBlock *block,
                                    const ProjectTieredContextRequest *request,
                                    ProjectTieredContextDisposition disposition, bool from_focus) {
  if (!block || !request) {
    return false;
  }

  if (!block_in_requested_tier(block, request)) {
    return true;
  }
  if (id_in_list(block->id, request->exclude_block_ids, request->exclude_block_count)) {
    return true;
  }

  if (id_in_list(block->id, request->summary_only_block_ids, request->summary_only_block_count) &&
      disposition != PROJECT_CONTEXT_BLOCK_PINNED) {
    disposition = PROJECT_CONTEXT_BLOCK_SUMMARIZED;
  }

  return add_selection(builder, block, disposition, from_focus);
}

static bool append_symbol_relations(ProjectContext *project, const ProjectInfoBlockRegistry *registry,
                                    const ProjectInfoBlock *focus,
                                    const ProjectTieredContextRequest *request,
                                    SelectionBuilder *builder) {
  const ProjectIRSnapshot *snapshot;
  size_t i;

  if (!project || !registry || !focus || !request || !builder) {
    return false;
  }

  snapshot = project_context_get_ir(project);
  if (!snapshot) {
    return false;
  }

  for (i = 0; i < snapshot->resolved_reference_count; i++) {
    const ProjectResolvedReferenceIR *ref = &snapshot->resolved_references[i];
    const char *focus_name = focus->qualified_name ? focus->qualified_name : focus->name;
    if (!focus_name) {
      continue;
    }

    if ((ref->owner_symbol && strcmp(ref->owner_symbol, focus_name) == 0) ||
        (ref->target_symbol && strcmp(ref->target_symbol, focus_name) == 0)) {
      char *ref_id = dup_printf_indexed("ref:%s:%zu", focus_name, i);
      const ProjectInfoBlock *ref_block;
      const ProjectInfoBlock *other_symbol;

      if (!ref_id) {
        return false;
      }
      ref_block = find_block_by_id_in_registry(registry, ref_id);
      free(ref_id);
      if (ref_block && !append_block_if_allowed(builder, ref_block, request,
                                                PROJECT_CONTEXT_BLOCK_EXPANDED, false)) {
        return false;
      }

      if (ref->owner_symbol && strcmp(ref->owner_symbol, focus_name) == 0) {
        other_symbol = find_symbol_block(registry, ref->target_symbol);
      } else {
        other_symbol = find_symbol_block(registry, ref->owner_symbol);
      }

      if (other_symbol && !append_block_if_allowed(builder, other_symbol, request,
                                                   PROJECT_CONTEXT_BLOCK_EXPANDED, false)) {
        return false;
      }
    }
  }

  for (i = 0; i < snapshot->call_graph_edge_count; i++) {
    const ProjectCallGraphEdgeIR *edge = &snapshot->call_graph_edges[i];
    const char *focus_name = focus->qualified_name ? focus->qualified_name : focus->name;
    const ProjectInfoBlock *related_block = NULL;

    if (!focus_name) {
      continue;
    }

    if (edge->caller_symbol && strcmp(edge->caller_symbol, focus_name) == 0) {
      related_block = find_symbol_block(registry, edge->callee_symbol);
    } else if (edge->callee_symbol && strcmp(edge->callee_symbol, focus_name) == 0) {
      related_block = find_symbol_block(registry, edge->caller_symbol);
    }

    if (related_block && !append_block_if_allowed(builder, related_block, request,
                                                  PROJECT_CONTEXT_BLOCK_EXPANDED, false)) {
      return false;
    }
  }

  return true;
}

static bool append_file_ancestors(const ProjectInfoBlockRegistry *registry, const ProjectInfoBlock *block,
                                  const ProjectTieredContextRequest *request,
                                  SelectionBuilder *builder) {
  const ProjectInfoBlock *file_block;
  const ProjectInfoBlock *directory_block;
  const ProjectInfoBlock *project_block;
  char *dir_path;
  bool ok = true;

  if (!registry || !block || !request || !builder || !block->file_path) {
    return true;
  }

  file_block = find_file_block(registry, block->file_path);
  if (file_block) {
    ok = append_block_if_allowed(builder, file_block, request, PROJECT_CONTEXT_BLOCK_SUMMARIZED, false);
    if (!ok) {
      return false;
    }
  }

  dir_path = path_dirname_dup(block->file_path);
  if (!dir_path) {
    return false;
  }

  directory_block = find_directory_block(registry, dir_path);
  if (directory_block) {
    ok = append_block_if_allowed(builder, directory_block, request,
                                 PROJECT_CONTEXT_BLOCK_SUMMARIZED, false);
  }
  free(dir_path);
  if (!ok) {
    return false;
  }

  project_block = find_project_block(registry);
  if (project_block) {
    ok = append_block_if_allowed(builder, project_block, request, PROJECT_CONTEXT_BLOCK_SUMMARIZED,
                                 false);
  }

  return ok;
}

static bool append_dependency_related(ProjectContext *project, const ProjectInfoBlockRegistry *registry,
                                      const ProjectInfoBlock *focus,
                                      const ProjectTieredContextRequest *request,
                                      SelectionBuilder *builder) {
  const ProjectIRSnapshot *snapshot;
  size_t i;

  if (!project || !registry || !focus || !request || !builder || !focus->file_path) {
    return true;
  }

  snapshot = project_context_get_ir(project);
  if (!snapshot) {
    return false;
  }

  for (i = 0; i < snapshot->dependency_count; i++) {
    const ProjectDependencyIR *dep = &snapshot->dependencies[i];
    const char *other_path = NULL;
    const ProjectInfoBlock *file_block;

    if (dep->source_file_path && strcmp(dep->source_file_path, focus->file_path) == 0) {
      other_path = dep->target_file_path;
    } else if (dep->target_file_path && strcmp(dep->target_file_path, focus->file_path) == 0) {
      other_path = dep->source_file_path;
    }

    if (!other_path) {
      continue;
    }

    file_block = find_file_block(registry, other_path);
    if (file_block && !append_block_if_allowed(builder, file_block, request,
                                               PROJECT_CONTEXT_BLOCK_SUMMARIZED, false)) {
      return false;
    }
  }

  return true;
}

void project_context_clear_info_blocks(ProjectContext *project) {
  size_t i;

  if (!project) {
    return;
  }

  for (i = 0; i < project->info_block_registry.block_count; i++) {
    free(project->info_block_registry.blocks[i].id);
    free(project->info_block_registry.blocks[i].name);
    free(project->info_block_registry.blocks[i].qualified_name);
    free(project->info_block_registry.blocks[i].file_path);
    free(project->info_block_registry.blocks[i].provenance);
    free(project->info_block_registry.blocks[i].desired_shape);
    free(project->info_block_registry.blocks[i].rationale);
    free(project->info_block_registry.blocks[i].anchor_list);
  }

  free(project->info_block_registry.blocks);
  memset(&project->info_block_registry, 0, sizeof(project->info_block_registry));
  project->info_block_registry_ready = false;

  for (i = 0; i < project->search_index_entry_count; i++) {
    free(project->search_index_entries[i].normalized_text);
    free(project->search_index_entries[i].related_blocks);
  }
  free(project->search_index_entries);
  project->search_index_entries = NULL;
  project->search_index_entry_count = 0;
  project->search_index_ready = false;
}

bool project_context_rebuild_info_blocks(ProjectContext *project) {
  const ProjectIRSnapshot *snapshot;
  size_t total_blocks;
  size_t i;
  size_t file_block_count;
  size_t directory_count = 0;
  char **directories = NULL;
  size_t block_index = 0;

  if (!project) {
    return false;
  }

  if (!project->ir_ready && !project_context_rebuild_ir(project)) {
    return false;
  }

  snapshot = project_context_get_ir(project);
  if (!snapshot) {
    return false;
  }

  project_context_clear_info_blocks(project);

  file_block_count = project->num_files;
  if (file_block_count > 0) {
    directories = calloc(file_block_count, sizeof(*directories));
    if (!directories) {
      return false;
    }
  }

  for (i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    char *dir_path;
    size_t j;
    bool seen = false;

    if (!ctx || !ctx->filename) {
      continue;
    }

    dir_path = path_dirname_dup(ctx->filename);
    if (!dir_path) {
      for (j = 0; j < directory_count; j++) {
        free(directories[j]);
      }
      free(directories);
      return false;
    }

    for (j = 0; j < directory_count; j++) {
      if (strcmp(directories[j], dir_path) == 0) {
        seen = true;
        break;
      }
    }

    if (seen) {
      free(dir_path);
      continue;
    }

    directories[directory_count++] = dir_path;
  }

  total_blocks = snapshot->symbol_count + snapshot->resolved_reference_count + file_block_count +
                 directory_count + 1 + project->plan_node_count;
  if (total_blocks > 0) {
    project->info_block_registry.blocks = calloc(total_blocks, sizeof(ProjectInfoBlock));
    if (!project->info_block_registry.blocks) {
      for (i = 0; i < directory_count; i++) {
        free(directories[i]);
      }
      free(directories);
      return false;
    }
  }

  for (i = 0; i < snapshot->symbol_count; i++) {
    const ProjectSymbolIR *symbol = &snapshot->symbols[i];
    ProjectInfoBlock *block = &project->info_block_registry.blocks[block_index++];
    const char *symbol_name = symbol->qualified_name ? symbol->qualified_name : symbol->name;

    block->id = dup_printf("sym:%s", symbol_name ? symbol_name : "anonymous");
    block->name = strdup(symbol->name ? symbol->name : symbol_name ? symbol_name : "anonymous");
    block->qualified_name = symbol_name ? strdup(symbol_name) : NULL;
    block->file_path = symbol->file_path ? strdup(symbol->file_path) : NULL;
    block->node = symbol->node;
    block->node_type = symbol->type;
    block->language = language_for_node(symbol->node);
    block->kind = PROJECT_INFO_BLOCK_SYMBOL;
    block->tier = tier_for_symbol_node_type(symbol->type);
    block->estimated_tokens = estimate_tokens_for_node(symbol->node);
    block->related_symbol_count = symbol->resolved_reference_count;
    block->origin = PROJECT_INFO_BLOCK_ORIGIN_PARSED;
    block->lifecycle = PROJECT_INFO_BLOCK_LIFECYCLE_NONE;
    block->provenance = block->file_path ? strdup(block->file_path) : NULL;
    block->confidence = 1.0f;
  }

  for (i = 0; i < snapshot->resolved_reference_count; i++) {
    const ProjectResolvedReferenceIR *ref = &snapshot->resolved_references[i];
    ProjectInfoBlock *block = &project->info_block_registry.blocks[block_index++];
    const char *owner_name = ref->owner_symbol ? ref->owner_symbol : "reference";
    const char *target_name = ref->target_symbol ? ref->target_symbol : "unresolved";
    int name_len = snprintf(NULL, 0, "%s -> %s", owner_name, target_name);

    block->id = dup_printf_indexed("ref:%s:%zu", owner_name, i);
    if (name_len >= 0) {
      block->name = malloc((size_t)name_len + 1);
      if (block->name) {
        snprintf(block->name, (size_t)name_len + 1, "%s -> %s", owner_name, target_name);
      }
    }
    block->qualified_name = block->name ? strdup(block->name) : NULL;
    block->file_path = ref->reference_node && ref->reference_node->file_path
                           ? strdup(ref->reference_node->file_path)
                           : NULL;
    block->node = ref->reference_node;
    block->node_type = ref->reference_node ? ref->reference_node->type : NODE_IDENTIFIER;
    block->language = language_for_node(ref->reference_node);
    block->kind = PROJECT_INFO_BLOCK_REFERENCE;
    block->tier = PROJECT_CONTEXT_TIER_0;
    block->estimated_tokens = estimate_tokens_for_node(ref->reference_node);
    if (block->estimated_tokens == 0) {
      block->estimated_tokens = 1;
    }
    block->related_symbol_count = ref->target_symbol ? 1 : 0;
    block->origin = PROJECT_INFO_BLOCK_ORIGIN_PARSED;
    block->lifecycle = PROJECT_INFO_BLOCK_LIFECYCLE_NONE;
    block->provenance = block->file_path ? strdup(block->file_path) : NULL;
    block->confidence = 1.0f;
  }

  for (i = 0; i < project->num_files; i++) {
    ParserContext *ctx = project->file_contexts[i];
    ProjectInfoBlock *block;

    if (!ctx || !ctx->filename) {
      continue;
    }

    block = &project->info_block_registry.blocks[block_index++];
    block->id = dup_printf("file:%s", ctx->filename);
    block->name = strdup(path_basename_ptr(ctx->filename));
    block->qualified_name = strdup(ctx->filename);
    block->file_path = strdup(ctx->filename);
    block->node = NULL;
    block->node_type = NODE_MODULE;
    block->language = ctx->language;
    block->kind = PROJECT_INFO_BLOCK_FILE;
    block->tier = PROJECT_CONTEXT_TIER_2;
    block->estimated_tokens = 8;
    block->related_symbol_count = ctx->num_ast_nodes;
    block->origin = PROJECT_INFO_BLOCK_ORIGIN_PARSED;
    block->lifecycle = PROJECT_INFO_BLOCK_LIFECYCLE_NONE;
    block->provenance = strdup(ctx->filename);
    block->confidence = 1.0f;
  }

  for (i = 0; i < directory_count; i++) {
    ProjectInfoBlock *block = &project->info_block_registry.blocks[block_index++];
    block->id = dup_printf("dir:%s", directories[i]);
    block->name = strdup(path_basename_ptr(directories[i]));
    block->qualified_name = strdup(directories[i]);
    block->file_path = NULL;
    block->node = NULL;
    block->node_type = NODE_MODULE;
    block->language = LANG_UNKNOWN;
    block->kind = PROJECT_INFO_BLOCK_DIRECTORY;
    block->tier = PROJECT_CONTEXT_TIER_3;
    block->estimated_tokens = 16;
    block->related_symbol_count = 0;
    block->origin = PROJECT_INFO_BLOCK_ORIGIN_PARSED;
    block->lifecycle = PROJECT_INFO_BLOCK_LIFECYCLE_NONE;
    block->provenance = strdup(directories[i]);
    block->confidence = 1.0f;
  }

  {
    ProjectInfoBlock *block = &project->info_block_registry.blocks[block_index++];
    block->id = dup_printf("project:%s", project->root_directory ? project->root_directory : ".");
    block->name = strdup(path_basename_ptr(project->root_directory ? project->root_directory : "."));
    block->qualified_name =
        strdup(project->root_directory ? project->root_directory : ".");
    block->file_path = NULL;
    block->node = NULL;
    block->node_type = NODE_ROOT;
    block->language = LANG_UNKNOWN;
    block->kind = PROJECT_INFO_BLOCK_PROJECT;
    block->tier = PROJECT_CONTEXT_TIER_4;
    block->estimated_tokens = 32;
    block->related_symbol_count = snapshot->symbol_count;
    block->origin = PROJECT_INFO_BLOCK_ORIGIN_PARSED;
    block->lifecycle = PROJECT_INFO_BLOCK_LIFECYCLE_NONE;
    block->provenance =
        strdup(project->root_directory ? project->root_directory : ".");
    block->confidence = 1.0f;
  }

  // WI-032: project durable plan nodes into the same registry as parsed blocks.
  for (i = 0; i < project->plan_node_count; i++) {
    const ProjectPlanNode *node = &project->plan_nodes[i];
    ProjectInfoBlock *block = &project->info_block_registry.blocks[block_index++];
    const char *qualified = node->projected_symbol ? node->projected_symbol : node->id;
    char *anchor_join = join_string_list(node->anchor_ids, node->anchor_count, ';');
    size_t tokens = estimate_tokens_for_text(node->desired_shape) +
                    estimate_tokens_for_text(node->rationale) + estimate_tokens_for_text(node->title);

    block->id = node->id ? strdup(node->id) : NULL;
    block->name = strdup(node->title ? node->title : (node->slug ? node->slug : "plan"));
    block->qualified_name = qualified ? strdup(qualified) : NULL;
    block->file_path = node->file_path ? strdup(node->file_path) : NULL;
    block->node = NULL;
    block->node_type = plan_block_node_type(node->kind);
    block->language = LANG_UNKNOWN;
    block->kind = plan_block_kind(node->kind);
    block->tier = plan_block_tier(node->kind);
    block->estimated_tokens = tokens > 0 ? tokens : 1;
    block->related_symbol_count = node->anchor_count;
    block->origin = PROJECT_INFO_BLOCK_ORIGIN_PLANNED;
    block->lifecycle = node->lifecycle;
    block->provenance =
        strdup(node->provenance ? node->provenance : (node->task_id ? node->task_id : "plan"));
    block->confidence = node->confidence;
    block->plan_kind = node->kind;
    block->desired_shape = node->desired_shape ? strdup(node->desired_shape) : NULL;
    block->rationale = node->rationale ? strdup(node->rationale) : NULL;
    block->anchor_list = anchor_join;
  }

  project->info_block_registry.block_count = block_index;
  memset(project->info_block_registry.tier_counts, 0, sizeof(project->info_block_registry.tier_counts));
  for (i = 0; i < block_index; i++) {
    project->info_block_registry.tier_counts[project->info_block_registry.blocks[i].tier]++;
  }
  project->info_block_registry_ready = true;

  for (i = 0; i < directory_count; i++) {
    free(directories[i]);
  }
  free(directories);

  return true;
}

const ProjectInfoBlockRegistry *project_context_get_info_block_registry(ProjectContext *project) {
  if (!project) {
    return NULL;
  }

  if (!project->info_block_registry_ready && !project_context_rebuild_info_blocks(project)) {
    return NULL;
  }

  return &project->info_block_registry;
}

const ProjectInfoBlock *project_context_find_info_block(const ProjectContext *project,
                                                        const char *block_id) {
  if (!project || !project->info_block_registry_ready || !block_id) {
    return NULL;
  }

  return find_block_by_id_in_registry(&project->info_block_registry, block_id);
}

static bool project_context_build_search_index(ProjectContext *project) {
  const ProjectInfoBlockRegistry *registry;
  const ProjectIRSnapshot *snapshot;
  size_t i;

  if (!project) {
    return false;
  }

  if (project->search_index_ready) {
    return true;
  }

  registry = project_context_get_info_block_registry(project);
  snapshot = project_context_get_ir(project);
  if (!registry || !snapshot) {
    return false;
  }

  if (registry->block_count > 0) {
    project->search_index_entries = calloc(registry->block_count, sizeof(*project->search_index_entries));
    if (!project->search_index_entries) {
      return false;
    }
  }

  project->search_index_entry_count = registry->block_count;
  for (i = 0; i < registry->block_count; i++) {
    project->search_index_entries[i].block = &registry->blocks[i];
    if (!build_search_text_for_block(&registry->blocks[i], &project->search_index_entries[i].normalized_text)) {
      project_context_clear_search_index(project);
      return false;
    }
  }

  for (i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->file_path) {
      const ProjectInfoBlock *file_block = find_file_block(registry, block->file_path);
      const ProjectInfoBlock *project_block = find_project_block(registry);
      char *dir_path = path_dirname_dup(block->file_path);
      const ProjectInfoBlock *directory_block = dir_path ? find_directory_block(registry, dir_path) : NULL;

      if ((file_block && !link_related_blocks(project, block, file_block)) ||
          (directory_block && !link_related_blocks(project, block, directory_block)) ||
          (project_block && !link_related_blocks(project, block, project_block))) {
        free(dir_path);
        project_context_clear_search_index(project);
        return false;
      }

      free(dir_path);
    }
  }

  for (i = 0; i < snapshot->resolved_reference_count; i++) {
    const ProjectResolvedReferenceIR *ref = &snapshot->resolved_references[i];
    char *ref_id;
    const ProjectInfoBlock *ref_block;
    const ProjectInfoBlock *owner_block;
    const ProjectInfoBlock *target_block;

    ref_id = dup_printf_indexed("ref:%s:%zu", ref->owner_symbol ? ref->owner_symbol : "reference", i);
    if (!ref_id) {
      project_context_clear_search_index(project);
      return false;
    }
    ref_block = find_block_by_id_in_registry(registry, ref_id);
    free(ref_id);
    owner_block = find_symbol_block(registry, ref->owner_symbol);
    target_block = find_symbol_block(registry, ref->target_symbol);

    if ((ref_block && owner_block && !link_related_blocks(project, ref_block, owner_block)) ||
        (ref_block && target_block && !link_related_blocks(project, ref_block, target_block)) ||
        (owner_block && target_block && !link_related_blocks(project, owner_block, target_block))) {
      project_context_clear_search_index(project);
      return false;
    }
  }

  for (i = 0; i < snapshot->call_graph_edge_count; i++) {
    const ProjectCallGraphEdgeIR *edge = &snapshot->call_graph_edges[i];
    const ProjectInfoBlock *caller_block = find_symbol_block(registry, edge->caller_symbol);
    const ProjectInfoBlock *callee_block = find_symbol_block(registry, edge->callee_symbol);
    if (caller_block && callee_block && !link_related_blocks(project, caller_block, callee_block)) {
      project_context_clear_search_index(project);
      return false;
    }
  }

  for (i = 0; i < snapshot->dependency_count; i++) {
    const ProjectDependencyIR *edge = &snapshot->dependencies[i];
    const ProjectInfoBlock *source_block = find_file_block(registry, edge->source_file_path);
    const ProjectInfoBlock *target_block = find_file_block(registry, edge->target_file_path);
    if (source_block && target_block && !link_related_blocks(project, source_block, target_block)) {
      project_context_clear_search_index(project);
      return false;
    }
  }

  project->search_index_ready = true;
  return true;
}

bool project_context_build_tiered_context(ProjectContext *project,
                                          const ProjectTieredContextRequest *request,
                                          ProjectTieredContextResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  SelectionBuilder builder = {0};
  size_t i;

  if (!project || !request || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  for (i = 0; i < request->focus_block_count; i++) {
    const ProjectInfoBlock *block = find_block_by_id_in_registry(registry, request->focus_block_ids[i]);
    if (block && !append_block_if_allowed(&builder, block, request, PROJECT_CONTEXT_BLOCK_PINNED, true)) {
      project_tiered_context_result_free(out_result);
      free(builder.items);
      return false;
    }
  }

  if (request->anchor_symbol) {
    const ProjectInfoBlock *block = find_symbol_block(registry, request->anchor_symbol);
    if (block && !append_block_if_allowed(&builder, block, request, PROJECT_CONTEXT_BLOCK_PINNED, true)) {
      free(builder.items);
      return false;
    }
  }

  if (request->anchor_file_path) {
    for (i = 0; i < registry->block_count; i++) {
      const ProjectInfoBlock *block = &registry->blocks[i];
      if (block->file_path && strcmp(block->file_path, request->anchor_file_path) == 0 &&
          !append_block_if_allowed(&builder, block, request, PROJECT_CONTEXT_BLOCK_EXPANDED, true)) {
        free(builder.items);
        return false;
      }
    }
  }

  if (request->include_related) {
    size_t base_count = builder.count;
    for (i = 0; i < base_count; i++) {
      const ProjectInfoBlock *block = builder.items[i].block;
      if (block->kind == PROJECT_INFO_BLOCK_SYMBOL) {
        if (!append_symbol_relations(project, registry, block, request, &builder) ||
            !append_file_ancestors(registry, block, request, &builder)) {
          free(builder.items);
          return false;
        }
      } else if (block->kind == PROJECT_INFO_BLOCK_FILE || block->file_path) {
        if (!append_file_ancestors(registry, block, request, &builder)) {
          free(builder.items);
          return false;
        }
      }

      if (request->include_dependencies &&
          !append_dependency_related(project, registry, block, request, &builder)) {
        free(builder.items);
        return false;
      }
    }
  }

  if (builder.count == 0) {
    for (i = 0; i < registry->block_count; i++) {
      if (!append_block_if_allowed(&builder, &registry->blocks[i], request,
                                   PROJECT_CONTEXT_BLOCK_EXPANDED, false)) {
        free(builder.items);
        return false;
      }
    }
  }

  if (request->max_blocks > 0 && builder.count > request->max_blocks) {
    builder.count = request->max_blocks;
  }

  if (request->max_tokens > 0) {
    size_t token_total = 0;
    size_t keep_count = 0;
    for (i = 0; i < builder.count; i++) {
      size_t next_total = token_total + builder.items[i].block->estimated_tokens;
      if (keep_count > 0 && next_total > request->max_tokens) {
        break;
      }
      token_total = next_total;
      keep_count++;
    }
    builder.count = keep_count;
    builder.estimated_tokens = token_total;
  }

  out_result->selections = builder.items;
  out_result->selection_count = builder.count;
  out_result->estimated_tokens = builder.estimated_tokens;
  out_result->effective_min_tier = request->min_tier;
  out_result->effective_max_tier = request->max_tier;
  return true;
}

bool project_context_search_info_blocks(ProjectContext *project,
                                        const ProjectSearchRequest *request,
                                        ProjectSearchResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  char *normalized_query = NULL;
  SearchHitRank *ranked_hits = NULL;
  size_t hit_count = 0;
  size_t i;

  if (!project || !request || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry || !project_context_build_search_index(project)) {
    return false;
  }

  if (request->query_text) {
    normalized_query = normalize_text_dup(request->query_text);
    if (!normalized_query) {
      return false;
    }
  }

  if (registry->block_count > 0) {
    ranked_hits = calloc(registry->block_count, sizeof(*ranked_hits));
    if (!ranked_hits) {
      free(normalized_query);
      return false;
    }
  }

  for (i = 0; i < project->search_index_entry_count; i++) {
    const struct ProjectSearchIndexEntry *entry = &project->search_index_entries[i];
    const ProjectInfoBlock *block = entry->block;
    size_t score = 0;
    bool relationship_match = false;
    bool name_match = false;
    bool text_match = false;

    if (!block || block->tier < request->min_tier || block->tier > request->max_tier) {
      continue;
    }

    if (request->origin_mask != 0 &&
        (request->origin_mask & (1u << (unsigned)block->origin)) == 0) {
      continue;
    }

    if (request->lifecycle_mask != 0 &&
        (request->lifecycle_mask & (1u << (unsigned)block->lifecycle)) == 0) {
      continue;
    }

    if (normalized_query && normalized_query[0] != '\0') {
      char *normalized_name = normalize_text_dup(block->name ? block->name : "");
      char *normalized_qualified = normalize_text_dup(block->qualified_name ? block->qualified_name : "");
      if (!normalized_name || !normalized_qualified) {
        free(normalized_name);
        free(normalized_qualified);
        free(normalized_query);
        free(ranked_hits);
        return false;
      }

      if (contains_normalized_text(normalized_name, normalized_query) ||
          contains_normalized_text(normalized_qualified, normalized_query)) {
        name_match = true;
        score += strcmp(normalized_name, normalized_query) == 0 ||
                         strcmp(normalized_qualified, normalized_query) == 0
                     ? 220
                     : 140;
      }
      if (contains_normalized_text(entry->normalized_text, normalized_query)) {
        text_match = true;
        score += 60;
      }

      free(normalized_name);
      free(normalized_qualified);
    }

    if (request->anchor_symbol) {
      if ((block->qualified_name && strcmp(block->qualified_name, request->anchor_symbol) == 0) ||
          (block->name && strcmp(block->name, request->anchor_symbol) == 0)) {
        relationship_match = true;
        score += 260;
      }
    }

    if (request->anchor_file_path && block->file_path &&
        strcmp(block->file_path, request->anchor_file_path) == 0) {
      relationship_match = true;
      score += 120;
    }

    if ((request->include_related || request->include_dependencies) &&
        (request->anchor_symbol || request->anchor_file_path)) {
      for (size_t j = 0; j < entry->related_block_count; j++) {
        const ProjectInfoBlock *related = entry->related_blocks[j];
        if ((request->anchor_symbol &&
             ((related->qualified_name && strcmp(related->qualified_name, request->anchor_symbol) == 0) ||
              (related->name && strcmp(related->name, request->anchor_symbol) == 0))) ||
            (request->anchor_file_path && related->file_path &&
             strcmp(related->file_path, request->anchor_file_path) == 0)) {
          relationship_match = true;
          score += 90;
          break;
        }
      }
    }

    if (score == 0) {
      continue;
    }

    ranked_hits[hit_count].hit.block = block;
    ranked_hits[hit_count].hit.score = score;
    ranked_hits[hit_count].hit.name_match = name_match;
    ranked_hits[hit_count].hit.text_match = text_match;
    ranked_hits[hit_count].hit.relationship_match = relationship_match;
    ranked_hits[hit_count].original_index = hit_count;
    hit_count++;
  }

  if (hit_count > 1) {
    qsort(ranked_hits, hit_count, sizeof(*ranked_hits), compare_search_hits_desc);
  }

  out_result->total_match_count = hit_count;
  if (request->max_hits > 0 && hit_count > request->max_hits) {
    hit_count = request->max_hits;
  }
  if (hit_count > 0) {
    out_result->hits = calloc(hit_count, sizeof(*out_result->hits));
    if (!out_result->hits) {
      free(normalized_query);
      free(ranked_hits);
      return false;
    }
    for (i = 0; i < hit_count; i++) {
      out_result->hits[i] = ranked_hits[i].hit;
    }
  }
  out_result->hit_count = hit_count;

  free(normalized_query);
  free(ranked_hits);
  return true;
}

bool project_context_assemble_prompt(ProjectContext *project,
                                     const ProjectPromptAssemblyRequest *request,
                                     ProjectPromptAssemblyResult *out_result) {
  ProjectTieredContextResult context = {0};
  ProjectSearchRequest search_request = {0};
  ProjectSearchResult search_result = {0};
  size_t buffer_size = 2048;
  size_t offset = 0;
  char *buffer;
  size_t token_total = 0;
  size_t i;

  if (!project || !request || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  if (!project_context_build_tiered_context(project, &request->context_request, &context)) {
    return false;
  }

  search_request.query_text = request->user_query;
  search_request.anchor_symbol = request->context_request.anchor_symbol;
  search_request.anchor_file_path = request->context_request.anchor_file_path;
  search_request.min_tier = request->context_request.min_tier;
  search_request.max_tier = request->context_request.max_tier;
  search_request.include_related = request->context_request.include_related;
  search_request.include_dependencies = request->context_request.include_dependencies;
  search_request.max_hits = context.selection_count;

  if (!project_context_search_info_blocks(project, &search_request, &search_result)) {
    project_tiered_context_result_free(&context);
    return false;
  }

  for (i = 0; i < context.selection_count; i++) {
    size_t best_rank = search_result.hit_count + 1;
    for (size_t j = 0; j < search_result.hit_count; j++) {
      if (search_result.hits[j].block == context.selections[i].block) {
        best_rank = j;
        break;
      }
    }
    for (size_t j = i + 1; j < context.selection_count; j++) {
      size_t other_rank = search_result.hit_count + 1;
      for (size_t k = 0; k < search_result.hit_count; k++) {
        if (search_result.hits[k].block == context.selections[j].block) {
          other_rank = k;
          break;
        }
      }
      if (other_rank < best_rank) {
        ProjectTieredContextSelection temp = context.selections[i];
        context.selections[i] = context.selections[j];
        context.selections[j] = temp;
        best_rank = other_rank;
      }
    }
  }

  for (i = 0; i < context.selection_count; i++) {
    ProjectTieredContextSelection *selection = &context.selections[i];
    size_t expanded_tokens = selection->block->estimated_tokens;
    size_t summary_tokens = summarized_block_tokens(selection->block);
    size_t target_tokens = selection->disposition == PROJECT_CONTEXT_BLOCK_SUMMARIZED ? summary_tokens
                                                                                       : expanded_tokens;

    if (request->max_prompt_tokens > 0 && token_total + target_tokens > request->max_prompt_tokens) {
      if (selection->disposition != PROJECT_CONTEXT_BLOCK_SUMMARIZED &&
          token_total + summary_tokens <= request->max_prompt_tokens) {
        selection->disposition = PROJECT_CONTEXT_BLOCK_SUMMARIZED;
        target_tokens = summary_tokens;
      } else {
        out_result->omitted_block_count++;
        context.selection_count = i;
        break;
      }
    }

    token_total += target_tokens;
  }

  buffer_size += token_total * 8U;
  buffer = calloc(buffer_size, 1);
  if (!buffer) {
    project_search_result_free(&search_result);
    project_tiered_context_result_free(&context);
    return false;
  }

  if (request->system_preamble &&
      !append_text_part(buffer, buffer_size, &offset, "System: ", request->system_preamble)) {
    free(buffer);
    project_search_result_free(&search_result);
    project_tiered_context_result_free(&context);
    return false;
  }
  if (request->response_format &&
      !append_text_part(buffer, buffer_size, &offset, "Response format: ", request->response_format)) {
    free(buffer);
    project_search_result_free(&search_result);
    project_tiered_context_result_free(&context);
    return false;
  }
  if (request->user_query &&
      !append_text_part(buffer, buffer_size, &offset, "User query: ", request->user_query)) {
    free(buffer);
    project_search_result_free(&search_result);
    project_tiered_context_result_free(&context);
    return false;
  }

  {
    int written = snprintf(buffer + offset, buffer_size - offset, "\nContext package:\n");
    if (written < 0 || (size_t)written >= buffer_size - offset) {
      free(buffer);
      project_search_result_free(&search_result);
      project_tiered_context_result_free(&context);
      return false;
    }
    offset += (size_t)written;
  }

  for (i = 0; i < context.selection_count; i++) {
    const ProjectTieredContextSelection *selection = &context.selections[i];
    const ProjectInfoBlock *block = selection->block;
    int written;

    written = snprintf(buffer + offset, buffer_size - offset, "[%zu] %s tier=%d disposition=%s\n",
                       i + 1, block->id ? block->id : "block", (int)block->tier,
                       disposition_label(selection->disposition));
    if (written < 0 || (size_t)written >= buffer_size - offset) {
      free(buffer);
      project_search_result_free(&search_result);
      project_tiered_context_result_free(&context);
      return false;
    }
    offset += (size_t)written;

    if (request->include_block_metadata) {
      if (!append_text_part(buffer, buffer_size, &offset, "kind: ", kind_label(block->kind)) ||
          !append_text_part(buffer, buffer_size, &offset, "name: ", block->name ? block->name : "") ||
          !append_text_part(buffer, buffer_size, &offset, "qualified: ",
                            block->qualified_name ? block->qualified_name : "") ||
          !append_text_part(buffer, buffer_size, &offset, "file: ",
                            block->file_path ? block->file_path : "")) {
        free(buffer);
        project_search_result_free(&search_result);
        project_tiered_context_result_free(&context);
        return false;
      }
    }

    if (block->node) {
      const char *body = block->node->raw_content ? block->node->raw_content : "";
      const char *summary = block->node->docstring ? block->node->docstring
                          : block->node->signature ? block->node->signature
                          : block->name ? block->name : "";
      if (!append_text_part(buffer, buffer_size, &offset, "signature: ",
                            block->node->signature ? block->node->signature : "") ||
          !append_text_part(buffer, buffer_size, &offset, "doc: ",
                            block->node->docstring ? block->node->docstring : "") ||
          !append_text_part(buffer, buffer_size, &offset,
                            selection->disposition == PROJECT_CONTEXT_BLOCK_SUMMARIZED ? "summary: "
                                                                                       : "content: ",
                            selection->disposition == PROJECT_CONTEXT_BLOCK_SUMMARIZED ? summary : body) ||
          !append_text_part(buffer, buffer_size, &offset, "", "")) {
        free(buffer);
        project_search_result_free(&search_result);
        project_tiered_context_result_free(&context);
        return false;
      }
    }
  }

  out_result->context_result = context;
  out_result->prompt_text = buffer;
  out_result->prompt_length = strlen(buffer);
  out_result->estimated_tokens = token_total + estimate_tokens_for_text(request->user_query) +
                                 estimate_tokens_for_text(request->system_preamble) +
                                 estimate_tokens_for_text(request->response_format);
  project_search_result_free(&search_result);
  return true;
}

void project_tiered_context_result_free(ProjectTieredContextResult *result) {
  if (!result) {
    return;
  }

  free(result->selections);
  memset(result, 0, sizeof(*result));
}

void project_search_result_free(ProjectSearchResult *result) {
  if (!result) {
    return;
  }

  free(result->hits);
  memset(result, 0, sizeof(*result));
}

void project_prompt_assembly_result_free(ProjectPromptAssemblyResult *result) {
  if (!result) {
    return;
  }

  free(result->prompt_text);
  project_tiered_context_result_free(&result->context_result);
  memset(result, 0, sizeof(*result));
}

/* ------------------------------------------------------------------------- */
/* WI-032: durable plan-node store and reconciliation                        */
/* ------------------------------------------------------------------------- */

static bool plan_node_set_string(char **field, const char *value) {
  char *copy = NULL;

  if (!field) {
    return false;
  }

  if (value) {
    copy = strdup(value);
    if (!copy) {
      return false;
    }
  }

  free(*field);
  *field = copy;
  return true;
}

static void plan_node_free(ProjectPlanNode *node) {
  if (!node) {
    return;
  }

  free(node->id);
  free(node->task_id);
  free(node->slug);
  free(node->title);
  free(node->desired_shape);
  free(node->rationale);
  free(node->provenance);
  free(node->projected_symbol);
  free(node->file_path);
  for (size_t i = 0; i < node->anchor_count; i++) {
    free(node->anchor_ids[i]);
  }
  free(node->anchor_ids);
  memset(node, 0, sizeof(*node));
}

static bool plan_store_reserve(ProjectContext *project, size_t min_capacity) {
  ProjectPlanNode *next;
  size_t capacity;

  if (!project) {
    return false;
  }
  if (project->plan_node_capacity >= min_capacity) {
    return true;
  }

  capacity = project->plan_node_capacity == 0 ? 8 : project->plan_node_capacity * 2;
  while (capacity < min_capacity) {
    capacity *= 2;
  }

  next = realloc(project->plan_nodes, capacity * sizeof(*next));
  if (!next) {
    return false;
  }

  memset(next + project->plan_node_capacity, 0,
         (capacity - project->plan_node_capacity) * sizeof(*next));
  project->plan_nodes = next;
  project->plan_node_capacity = capacity;
  return true;
}

void project_context_clear_plan_nodes(ProjectContext *project) {
  if (!project) {
    return;
  }

  for (size_t i = 0; i < project->plan_node_count; i++) {
    plan_node_free(&project->plan_nodes[i]);
  }
  free(project->plan_nodes);
  project->plan_nodes = NULL;
  project->plan_node_count = 0;
  project->plan_node_capacity = 0;

  project_context_clear_info_blocks(project);
}

ProjectPlanNode *project_context_plan_node_create(ProjectContext *project, const char *task_id,
                                                  const char *slug, ProjectPlanNodeKind kind) {
  ProjectPlanNode *node;
  char *id;
  int needed;

  if (!project || !task_id || !slug) {
    return NULL;
  }

  needed = snprintf(NULL, 0, "plan:%s:%s", task_id, slug);
  if (needed < 0) {
    return NULL;
  }
  id = malloc((size_t)needed + 1);
  if (!id) {
    return NULL;
  }
  snprintf(id, (size_t)needed + 1, "plan:%s:%s", task_id, slug);

  if (project_context_find_plan_node(project, id)) {
    free(id);
    return NULL;
  }

  if (!plan_store_reserve(project, project->plan_node_count + 1)) {
    free(id);
    return NULL;
  }

  node = &project->plan_nodes[project->plan_node_count];
  memset(node, 0, sizeof(*node));
  node->id = id;
  node->task_id = strdup(task_id);
  node->slug = strdup(slug);
  node->kind = kind;
  node->lifecycle = PROJECT_INFO_BLOCK_LIFECYCLE_PLANNED;
  node->confidence = 0.5f;

  if (!node->task_id || !node->slug) {
    plan_node_free(node);
    return NULL;
  }

  project->plan_node_count++;
  project_context_clear_info_blocks(project);
  return node;
}

bool project_context_plan_node_set_title(ProjectContext *project, ProjectPlanNode *node,
                                         const char *title) {
  if (!project || !node || !plan_node_set_string(&node->title, title)) {
    return false;
  }
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_desired_shape(ProjectContext *project, ProjectPlanNode *node,
                                                 const char *desired_shape) {
  if (!project || !node || !plan_node_set_string(&node->desired_shape, desired_shape)) {
    return false;
  }
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_rationale(ProjectContext *project, ProjectPlanNode *node,
                                             const char *rationale) {
  if (!project || !node || !plan_node_set_string(&node->rationale, rationale)) {
    return false;
  }
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_provenance(ProjectContext *project, ProjectPlanNode *node,
                                              const char *provenance) {
  if (!project || !node || !plan_node_set_string(&node->provenance, provenance)) {
    return false;
  }
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_projected_symbol(ProjectContext *project, ProjectPlanNode *node,
                                                    const char *symbol_name) {
  if (!project || !node || !plan_node_set_string(&node->projected_symbol, symbol_name)) {
    return false;
  }
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_file_path(ProjectContext *project, ProjectPlanNode *node,
                                             const char *file_path) {
  if (!project || !node || !plan_node_set_string(&node->file_path, file_path)) {
    return false;
  }
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_lifecycle(ProjectContext *project, ProjectPlanNode *node,
                                             ProjectInfoBlockLifecycle lifecycle) {
  if (!project || !node) {
    return false;
  }
  node->lifecycle = lifecycle;
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_set_confidence(ProjectContext *project, ProjectPlanNode *node,
                                              float confidence) {
  if (!project || !node) {
    return false;
  }
  node->confidence = confidence;
  project_context_clear_info_blocks(project);
  return true;
}

bool project_context_plan_node_add_anchor(ProjectContext *project, ProjectPlanNode *node,
                                          const char *anchor_block_id) {
  char **next_anchors;
  size_t next_capacity;
  char *copy;

  if (!project || !node || !anchor_block_id || anchor_block_id[0] == '\0') {
    return false;
  }

  for (size_t i = 0; i < node->anchor_count; i++) {
    if (node->anchor_ids[i] && strcmp(node->anchor_ids[i], anchor_block_id) == 0) {
      return true;
    }
  }

  copy = strdup(anchor_block_id);
  if (!copy) {
    return false;
  }

  if (node->anchor_count == node->anchor_capacity) {
    next_capacity = node->anchor_capacity == 0 ? 4 : node->anchor_capacity * 2;
    next_anchors = realloc(node->anchor_ids, next_capacity * sizeof(*next_anchors));
    if (!next_anchors) {
      free(copy);
      return false;
    }
    node->anchor_ids = next_anchors;
    node->anchor_capacity = next_capacity;
  }

  node->anchor_ids[node->anchor_count++] = copy;
  project_context_clear_info_blocks(project);
  return true;
}

ProjectPlanNode *project_context_find_plan_node(ProjectContext *project, const char *plan_node_id) {
  if (!project || !plan_node_id) {
    return NULL;
  }

  for (size_t i = 0; i < project->plan_node_count; i++) {
    if (project->plan_nodes[i].id && strcmp(project->plan_nodes[i].id, plan_node_id) == 0) {
      return &project->plan_nodes[i];
    }
  }

  return NULL;
}

size_t project_context_get_plan_node_count(const ProjectContext *project) {
  return project ? project->plan_node_count : 0;
}

const ProjectPlanNode *project_context_get_plan_node_by_index(const ProjectContext *project,
                                                              size_t index) {
  if (!project || index >= project->plan_node_count) {
    return NULL;
  }
  return &project->plan_nodes[index];
}

static bool plan_anchor_exists_in_parsed_state(const ProjectInfoBlockRegistry *registry,
                                               const char *anchor_id) {
  if (!registry || !anchor_id) {
    return false;
  }

  for (size_t i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->origin == PROJECT_INFO_BLOCK_ORIGIN_PARSED && block->id &&
        strcmp(block->id, anchor_id) == 0) {
      return true;
    }
  }

  return false;
}

static const ProjectInfoBlock *find_parsed_symbol_block(const ProjectInfoBlockRegistry *registry,
                                                        const char *symbol_name) {
  if (!registry || !symbol_name) {
    return NULL;
  }

  for (size_t i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    if (block->origin != PROJECT_INFO_BLOCK_ORIGIN_PARSED ||
        block->kind != PROJECT_INFO_BLOCK_SYMBOL) {
      continue;
    }
    if (block->qualified_name && strcmp(block->qualified_name, symbol_name) == 0) {
      return block;
    }
    if (block->name && strcmp(block->name, symbol_name) == 0) {
      return block;
    }
  }

  return NULL;
}

bool project_context_reconcile_plan_nodes(ProjectContext *project,
                                          ProjectPlanNodeReconciliationResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  ProjectPlanNodeReconciliationEntry *entries = NULL;
  size_t entry_count = 0;

  if (!project || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  if (project->plan_node_count > 0) {
    entries = calloc(project->plan_node_count, sizeof(*entries));
    if (!entries) {
      return false;
    }
  }

  for (size_t i = 0; i < project->plan_node_count; i++) {
    ProjectPlanNode *node = &project->plan_nodes[i];
    ProjectInfoBlockLifecycle previous = node->lifecycle;
    ProjectInfoBlockLifecycle next = previous;
    size_t missing_anchors = 0;
    bool realized = false;

    for (size_t a = 0; a < node->anchor_count; a++) {
      if (!plan_anchor_exists_in_parsed_state(registry, node->anchor_ids[a])) {
        missing_anchors++;
      }
    }

    if (node->projected_symbol && find_parsed_symbol_block(registry, node->projected_symbol)) {
      realized = true;
    }

    if (realized) {
      if (previous != PROJECT_INFO_BLOCK_LIFECYCLE_IMPLEMENTED &&
          previous != PROJECT_INFO_BLOCK_LIFECYCLE_VERIFIED &&
          previous != PROJECT_INFO_BLOCK_LIFECYCLE_DOCUMENTED) {
        next = PROJECT_INFO_BLOCK_LIFECYCLE_IMPLEMENTED;
      }
    } else if (node->anchor_count > 0 && missing_anchors == node->anchor_count) {
      next = PROJECT_INFO_BLOCK_LIFECYCLE_STALE;
    } else if (missing_anchors > 0) {
      next = PROJECT_INFO_BLOCK_LIFECYCLE_CONFLICT;
    }

    if (next != previous) {
      node->lifecycle = next;
      entries[entry_count].node = node;
      entries[entry_count].previous_lifecycle = previous;
      entries[entry_count].new_lifecycle = next;
      entry_count++;
      if (next == PROJECT_INFO_BLOCK_LIFECYCLE_IMPLEMENTED) {
        out_result->implemented_count++;
      } else if (next == PROJECT_INFO_BLOCK_LIFECYCLE_STALE) {
        out_result->stale_count++;
      } else if (next == PROJECT_INFO_BLOCK_LIFECYCLE_CONFLICT) {
        out_result->conflict_count++;
      }
    }
  }

  if (entry_count == 0) {
    free(entries);
    entries = NULL;
  } else {
    // Lifecycle changed: refresh the derived registry projection on next access.
    project_context_clear_info_blocks(project);
  }

  out_result->entries = entries;
  out_result->entry_count = entry_count;
  return true;
}

void project_plan_node_reconciliation_result_free(ProjectPlanNodeReconciliationResult *result) {
  if (!result) {
    return;
  }

  free(result->entries);
  memset(result, 0, sizeof(*result));
}

/* ------------------------------------------------------------------------- */
/* WI-036: delta view and agent map query API                                */
/* ------------------------------------------------------------------------- */

static ProjectDeltaKind delta_kind_for_plan_node(const ProjectPlanNode *node, bool realized) {
  if (realized) {
    return PROJECT_DELTA_REUSE;
  }

  switch (node->kind) {
  case PROJECT_PLAN_NODE_NEW_SYMBOL:
  case PROJECT_PLAN_NODE_NEW_FILE:
  case PROJECT_PLAN_NODE_NEW_MODULE:
  case PROJECT_PLAN_NODE_NEW_TEST:
    return PROJECT_DELTA_ADD;
  case PROJECT_PLAN_NODE_REMOVE:
    return PROJECT_DELTA_REMOVE;
  default:
    return PROJECT_DELTA_CHANGE;
  }
}

bool project_context_compute_delta(ProjectContext *project, const char *task_id, const char *stage,
                                   ProjectDeltaResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  ProjectDeltaEntry *entries = NULL;
  size_t count = 0;

  if (!project || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  if (project->plan_node_count > 0) {
    entries = calloc(project->plan_node_count, sizeof(*entries));
    if (!entries) {
      return false;
    }
  }

  for (size_t i = 0; i < project->plan_node_count; i++) {
    const ProjectPlanNode *node = &project->plan_nodes[i];
    const ProjectInfoBlock *block;
    bool realized;
    ProjectDeltaKind kind;

    if (task_id && (!node->task_id || strcmp(node->task_id, task_id) != 0)) {
      continue;
    }

    block = find_block_by_id_in_registry(registry, node->id);
    realized = node->projected_symbol && find_parsed_symbol_block(registry, node->projected_symbol) != NULL;
    kind = delta_kind_for_plan_node(node, realized);

    entries[count].kind = kind;
    entries[count].block = block;
    entries[count].plan_node = node;
    entries[count].anchors = block ? block->anchor_list : NULL;
    entries[count].projected_shape = node->desired_shape;
    entries[count].provenance = node->provenance ? node->provenance : node->task_id;
    entries[count].confidence = node->confidence;
    entries[count].lifecycle = node->lifecycle;
    entries[count].estimated_tokens = block ? block->estimated_tokens : 0;
    out_result->estimated_tokens += entries[count].estimated_tokens;
    count++;

    switch (kind) {
    case PROJECT_DELTA_ADD:
      out_result->add_count++;
      break;
    case PROJECT_DELTA_CHANGE:
      out_result->change_count++;
      break;
    case PROJECT_DELTA_REMOVE:
      out_result->remove_count++;
      break;
    case PROJECT_DELTA_REUSE:
      out_result->reuse_count++;
      break;
    }
  }

  if (count == 0) {
    free(entries);
    entries = NULL;
  }

  out_result->task_id = task_id;
  out_result->stage = stage;
  out_result->entries = entries;
  out_result->entry_count = count;
  return true;
}

void project_delta_result_free(ProjectDeltaResult *result) {
  if (!result) {
    return;
  }

  free(result->entries);
  memset(result, 0, sizeof(*result));
}

typedef struct {
  ProjectMapResultItem *items;
  size_t count;
  size_t capacity;
  size_t estimated_tokens;
} MapQueryBuilder;

static void map_query_builder_free(MapQueryBuilder *builder) {
  if (!builder) {
    return;
  }
  free(builder->items);
  memset(builder, 0, sizeof(*builder));
}

static bool map_query_builder_add(MapQueryBuilder *builder, const ProjectInfoBlock *block,
                                  ProjectMapQueryKind kind, const char *reason, size_t distance) {
  ProjectMapResultItem *next;
  size_t capacity;

  if (!builder || !block) {
    return true;
  }

  for (size_t i = 0; i < builder->count; i++) {
    if (builder->items[i].block == block) {
      return true;
    }
  }

  if (builder->count == builder->capacity) {
    capacity = builder->capacity == 0 ? 8 : builder->capacity * 2;
    next = realloc(builder->items, capacity * sizeof(*next));
    if (!next) {
      return false;
    }
    builder->items = next;
    builder->capacity = capacity;
  }

  builder->items[builder->count].block = block;
  builder->items[builder->count].kind = kind;
  builder->items[builder->count].reason = reason;
  builder->items[builder->count].provenance = block->provenance;
  builder->items[builder->count].confidence = block->confidence;
  builder->items[builder->count].estimated_tokens = block->estimated_tokens;
  builder->items[builder->count].distance = distance;
  builder->count++;
  builder->estimated_tokens += block->estimated_tokens;
  return true;
}

static bool map_query_finalize(MapQueryBuilder *builder, ProjectMapQueryKind kind,
                               ProjectMapQueryResult *out_result) {
  if (!builder || !out_result) {
    return false;
  }

  out_result->kind = kind;
  out_result->items = builder->items;
  out_result->item_count = builder->count;
  out_result->estimated_tokens = builder->estimated_tokens;
  builder->items = NULL;
  builder->count = 0;
  builder->capacity = 0;
  builder->estimated_tokens = 0;
  return true;
}

bool project_context_query_node(ProjectContext *project, const char *block_id,
                                ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  const ProjectInfoBlock *block;
  MapQueryBuilder builder = {0};

  if (!project || !block_id || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  block = find_block_by_id_in_registry(registry, block_id);
  if (block && !map_query_builder_add(&builder, block, PROJECT_MAP_QUERY_NODE, "resolved node", 0)) {
    map_query_builder_free(&builder);
    return false;
  }

  return map_query_finalize(&builder, PROJECT_MAP_QUERY_NODE, out_result);
}

bool project_context_query_resolve(ProjectContext *project, const char *task_id, const char *stage,
                                   ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  MapQueryBuilder builder = {0};

  (void)stage;

  if (!project || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  for (size_t i = 0; i < project->plan_node_count; i++) {
    const ProjectPlanNode *node = &project->plan_nodes[i];
    const ProjectInfoBlock *block;

    if (task_id && (!node->task_id || strcmp(node->task_id, task_id) != 0)) {
      continue;
    }

    block = find_block_by_id_in_registry(registry, node->id);
    if (block && !map_query_builder_add(&builder, block, PROJECT_MAP_QUERY_RESOLVE, "planned for task", 0)) {
      map_query_builder_free(&builder);
      return false;
    }

    for (size_t a = 0; a < node->anchor_count; a++) {
      const ProjectInfoBlock *anchor = find_block_by_id_in_registry(registry, node->anchor_ids[a]);
      if (anchor && !map_query_builder_add(&builder, anchor, PROJECT_MAP_QUERY_RESOLVE, "anchor", 1)) {
        map_query_builder_free(&builder);
        return false;
      }
    }
  }

  if (builder.count == 0) {
    const ProjectInfoBlock *project_block = find_project_block(registry);
    if (project_block && !map_query_builder_add(&builder, project_block, PROJECT_MAP_QUERY_RESOLVE,
                                                "project seed", 0)) {
      map_query_builder_free(&builder);
      return false;
    }
  }

  return map_query_finalize(&builder, PROJECT_MAP_QUERY_RESOLVE, out_result);
}

bool project_context_query_expand(ProjectContext *project, const char *block_id,
                                  ProjectContextTier to_tier, ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  const ProjectInfoBlock *base;
  const struct ProjectSearchIndexEntry *entry;
  MapQueryBuilder builder = {0};

  if (!project || !block_id || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry || !project_context_build_search_index(project)) {
    return false;
  }

  base = find_block_by_id_in_registry(registry, block_id);
  if (!base) {
    return map_query_finalize(&builder, PROJECT_MAP_QUERY_EXPAND, out_result);
  }

  if (!map_query_builder_add(&builder, base, PROJECT_MAP_QUERY_EXPAND, "seed node", 0)) {
    map_query_builder_free(&builder);
    return false;
  }

  entry = find_search_entry(project, base);
  if (entry) {
    for (size_t i = 0; i < entry->related_block_count; i++) {
      const ProjectInfoBlock *related = entry->related_blocks[i];
      if (related->tier >= base->tier && related->tier <= to_tier &&
          !map_query_builder_add(&builder, related, PROJECT_MAP_QUERY_EXPAND, "expanded relation", 1)) {
        map_query_builder_free(&builder);
        return false;
      }
    }
  }

  return map_query_finalize(&builder, PROJECT_MAP_QUERY_EXPAND, out_result);
}

static bool block_array_contains(const ProjectInfoBlock **blocks, size_t count,
                                 const ProjectInfoBlock *block) {
  for (size_t i = 0; i < count; i++) {
    if (blocks[i] == block) {
      return true;
    }
  }
  return false;
}

bool project_context_query_neighbors(ProjectContext *project, const char *block_id, size_t depth,
                                     ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  const ProjectInfoBlock *base;
  MapQueryBuilder builder = {0};
  const ProjectInfoBlock **visited = NULL;
  size_t visited_count = 0;
  size_t visited_capacity = 0;
  const ProjectInfoBlock **frontier = NULL;
  size_t frontier_count = 0;

  if (!project || !block_id || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry || !project_context_build_search_index(project)) {
    return false;
  }

  base = find_block_by_id_in_registry(registry, block_id);
  if (!base) {
    return map_query_finalize(&builder, PROJECT_MAP_QUERY_NEIGHBORS, out_result);
  }

  if (!map_query_builder_add(&builder, base, PROJECT_MAP_QUERY_NEIGHBORS, "seed node", 0)) {
    map_query_builder_free(&builder);
    return false;
  }

  frontier = calloc(1, sizeof(*frontier));
  visited = calloc(1, sizeof(*visited));
  if (!frontier || !visited) {
    free(frontier);
    free(visited);
    map_query_builder_free(&builder);
    return false;
  }
  frontier[0] = base;
  frontier_count = 1;
  visited[0] = base;
  visited_count = 1;
  visited_capacity = 1;

  for (size_t d = 1; d <= depth; d++) {
    const ProjectInfoBlock **next = NULL;
    size_t next_count = 0;
    size_t next_capacity = 0;

    for (size_t f = 0; f < frontier_count; f++) {
      const struct ProjectSearchIndexEntry *entry = find_search_entry(project, frontier[f]);
      if (!entry) {
        continue;
      }
      for (size_t r = 0; r < entry->related_block_count; r++) {
        const ProjectInfoBlock *related = entry->related_blocks[r];
        if (block_array_contains(visited, visited_count, related)) {
          continue;
        }

        if (visited_count == visited_capacity) {
          size_t next_capacity_visited = visited_capacity == 0 ? 8 : visited_capacity * 2;
          const ProjectInfoBlock **grown =
              realloc(visited, next_capacity_visited * sizeof(*grown));
          if (!grown) {
            free(next);
            free(frontier);
            free(visited);
            map_query_builder_free(&builder);
            return false;
          }
          visited = grown;
          visited_capacity = next_capacity_visited;
        }
        visited[visited_count++] = related;

        if (!map_query_builder_add(&builder, related, PROJECT_MAP_QUERY_NEIGHBORS, "neighbor", d)) {
          free(next);
          free(frontier);
          free(visited);
          map_query_builder_free(&builder);
          return false;
        }

        if (next_count == next_capacity) {
          size_t grown_capacity = next_capacity == 0 ? 8 : next_capacity * 2;
          const ProjectInfoBlock **grown = realloc(next, grown_capacity * sizeof(*grown));
          if (!grown) {
            free(next);
            free(frontier);
            free(visited);
            map_query_builder_free(&builder);
            return false;
          }
          next = grown;
          next_capacity = grown_capacity;
        }
        next[next_count++] = related;
      }
    }

    free(frontier);
    frontier = next;
    frontier_count = next_count;
    if (frontier_count == 0) {
      break;
    }
  }

  free(frontier);
  free(visited);
  return map_query_finalize(&builder, PROJECT_MAP_QUERY_NEIGHBORS, out_result);
}

static bool scope_matches(const ProjectInfoBlock *block, const char *scope) {
  if (!scope || scope[0] == '\0') {
    return true;
  }
  if (block->file_path && strstr(block->file_path, scope) != NULL) {
    return true;
  }
  if (block->qualified_name && strstr(block->qualified_name, scope) != NULL) {
    return true;
  }
  if (block->name && strstr(block->name, scope) != NULL) {
    return true;
  }
  return false;
}

bool project_context_query_duplicates(ProjectContext *project, const char *scope,
                                      ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  MapQueryBuilder builder = {0};

  if (!project || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  for (size_t i = 0; i < registry->block_count; i++) {
    const ProjectInfoBlock *block = &registry->blocks[i];
    char *normalized_name;
    bool duplicate = false;

    if (block->origin != PROJECT_INFO_BLOCK_ORIGIN_PARSED ||
        block->kind != PROJECT_INFO_BLOCK_SYMBOL || !scope_matches(block, scope)) {
      continue;
    }

    normalized_name = normalize_text_dup(block->name ? block->name : "");
    if (!normalized_name) {
      map_query_builder_free(&builder);
      return false;
    }

    for (size_t j = 0; j < i && !duplicate; j++) {
      const ProjectInfoBlock *other = &registry->blocks[j];
      if (other->origin != PROJECT_INFO_BLOCK_ORIGIN_PARSED ||
          other->kind != PROJECT_INFO_BLOCK_SYMBOL || (other->id && block->id &&
          strcmp(other->id, block->id) == 0)) {
        continue;
      }

      if (normalized_name[0] != '\0') {
        char *other_name = normalize_text_dup(other->name ? other->name : "");
        if (!other_name) {
          free(normalized_name);
          map_query_builder_free(&builder);
          return false;
        }
        if (strcmp(other_name, normalized_name) == 0) {
          duplicate = true;
        }
        free(other_name);
      }

      if (!duplicate && block->node && other->node && block->node->signature && other->node->signature &&
          strcmp(block->node->signature, other->node->signature) == 0) {
        duplicate = true;
      }
    }

    free(normalized_name);
    if (duplicate &&
        !map_query_builder_add(&builder, block, PROJECT_MAP_QUERY_DUPLICATES, "duplicate unit", 0)) {
      map_query_builder_free(&builder);
      return false;
    }
  }

  return map_query_finalize(&builder, PROJECT_MAP_QUERY_DUPLICATES, out_result);
}

static bool plan_node_anchors_symbol(const ProjectPlanNode *node, const ProjectInfoBlock *symbol_block) {
  if (!node || !symbol_block || !symbol_block->id) {
    return false;
  }
  if (node->projected_symbol && strcmp(node->projected_symbol, symbol_block->name ? symbol_block->name : "") == 0) {
    return true;
  }
  for (size_t i = 0; i < node->anchor_count; i++) {
    if (node->anchor_ids[i] && strcmp(node->anchor_ids[i], symbol_block->id) == 0) {
      return true;
    }
  }
  return false;
}

bool project_context_query_observability(ProjectContext *project, const char *symbol,
                                         ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  const ProjectInfoBlock *symbol_block;
  MapQueryBuilder builder = {0};

  if (!project || !symbol || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  symbol_block = find_symbol_block(registry, symbol);
  for (size_t i = 0; i < project->plan_node_count; i++) {
    const ProjectPlanNode *node = &project->plan_nodes[i];
    const ProjectInfoBlock *block;

    if (node->kind != PROJECT_PLAN_NODE_OBSERVABILITY_POINT ||
        !plan_node_anchors_symbol(node, symbol_block)) {
      continue;
    }

    block = find_block_by_id_in_registry(registry, node->id);
    if (block && !map_query_builder_add(&builder, block, PROJECT_MAP_QUERY_OBSERVABILITY,
                                        "observability for symbol", 0)) {
      map_query_builder_free(&builder);
      return false;
    }
  }

  return map_query_finalize(&builder, PROJECT_MAP_QUERY_OBSERVABILITY, out_result);
}

bool project_context_query_change_impact(ProjectContext *project, const char *const *files,
                                         size_t file_count, ProjectMapQueryResult *out_result) {
  const ProjectInfoBlockRegistry *registry;
  MapQueryBuilder builder = {0};

  if (!project || !files || !out_result) {
    return false;
  }

  memset(out_result, 0, sizeof(*out_result));
  registry = project_context_get_info_block_registry(project);
  if (!registry) {
    return false;
  }

  for (size_t f = 0; f < file_count; f++) {
    const char *file_path = files[f];
    const ProjectInfoBlock *file_block;

    if (!file_path) {
      continue;
    }

    file_block = find_file_block(registry, file_path);
    if (file_block && !map_query_builder_add(&builder, file_block, PROJECT_MAP_QUERY_CHANGE_IMPACT,
                                             "changed file", 0)) {
      map_query_builder_free(&builder);
      return false;
    }

    for (size_t i = 0; i < registry->block_count; i++) {
      const ProjectInfoBlock *block = &registry->blocks[i];
      if (block == file_block || !block->file_path || strcmp(block->file_path, file_path) != 0) {
        continue;
      }
      if (!map_query_builder_add(&builder, block, PROJECT_MAP_QUERY_CHANGE_IMPACT,
                                 "symbol in changed file", 1)) {
        map_query_builder_free(&builder);
        return false;
      }
    }

    for (size_t i = 0; i < project->plan_node_count; i++) {
      const ProjectPlanNode *node = &project->plan_nodes[i];
      const ProjectInfoBlock *planned;
      bool anchored = false;

      if (node->file_path && strcmp(node->file_path, file_path) == 0) {
        anchored = true;
      }
      if (file_block && file_block->id) {
        for (size_t a = 0; a < node->anchor_count; a++) {
          if (node->anchor_ids[a] && strcmp(node->anchor_ids[a], file_block->id) == 0) {
            anchored = true;
            break;
          }
        }
      }

      if (!anchored) {
        continue;
      }

      planned = find_block_by_id_in_registry(registry, node->id);
      if (planned && !map_query_builder_add(&builder, planned, PROJECT_MAP_QUERY_CHANGE_IMPACT,
                                            "plan anchored to changed file", 1)) {
        map_query_builder_free(&builder);
        return false;
      }
    }
  }

  return map_query_finalize(&builder, PROJECT_MAP_QUERY_CHANGE_IMPACT, out_result);
}

void project_map_query_result_free(ProjectMapQueryResult *result) {
  if (!result) {
    return;
  }

  free(result->items);
  memset(result, 0, sizeof(*result));
}
