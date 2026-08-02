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
                 directory_count + 1;
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
