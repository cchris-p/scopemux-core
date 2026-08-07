#include "c_cpp_resolver_shared_utils.h"
#include <string.h>

static ReferenceResolverCCppStats c_cpp_stats = {0};

static const ASTNode *find_ast_root(const ASTNode *node) {
  const ASTNode *current = node;
  while (current && current->parent) {
    current = current->parent;
  }
  return current;
}

static const char *path_basename_ptr(const char *path) {
  const char *last_slash;

  if (!path) {
    return NULL;
  }

  last_slash = strrchr(path, '/');
  return last_slash ? last_slash + 1 : path;
}

static bool file_path_matches_include(const char *file_path, const char *include_name) {
  size_t file_len;
  size_t include_len;
  const char *file_base;
  const char *include_base;

  if (!file_path || !include_name) {
    return false;
  }

  if (strcmp(file_path, include_name) == 0) {
    return true;
  }

  file_len = strlen(file_path);
  include_len = strlen(include_name);
  if (file_len >= include_len && strcmp(file_path + file_len - include_len, include_name) == 0 &&
      (file_len == include_len || file_path[file_len - include_len - 1] == '/')) {
    return true;
  }

  file_base = path_basename_ptr(file_path);
  include_base = path_basename_ptr(include_name);
  return file_base && include_base && strcmp(file_base, include_base) == 0;
}

static const char *include_name_from_node(const ASTNode *node) {
  const char *start;
  const char *end;
  static char include_name[256];
  size_t len;

  if (!node) {
    return NULL;
  }

  if (node->name && node->name[0] != '\0') {
    return node->name;
  }

  if (!node->raw_content) {
    return NULL;
  }

  start = strchr(node->raw_content, '"');
  if (!start) {
    start = strchr(node->raw_content, '<');
  }
  if (!start) {
    return NULL;
  }

  end = strchr(start + 1, start[0] == '"' ? '"' : '>');
  if (!end || end <= start + 1) {
    return NULL;
  }

  len = (size_t)(end - start - 1);
  if (len >= sizeof(include_name)) {
    len = sizeof(include_name) - 1;
  }

  memcpy(include_name, start + 1, len);
  include_name[len] = '\0';
  return include_name;
}

static bool root_has_matching_include(const ASTNode *node, const char *candidate_path) {
  const char *include_name;
  size_t i;

  if (!node || !candidate_path) {
    return false;
  }

  if (node->type == NODE_INCLUDE) {
    include_name = include_name_from_node(node);
    if (include_name && file_path_matches_include(candidate_path, include_name)) {
      return true;
    }
  }

  for (i = 0; i < node->num_children; i++) {
    if (root_has_matching_include(node->children[i], candidate_path)) {
      return true;
    }
  }

  return false;
}

static bool entry_matches_reference_type(const SymbolEntry *entry, ReferenceType ref_type) {
  ASTNodeType type;

  if (!entry || !entry->node) {
    return false;
  }

  type = entry->node->type;
  switch (ref_type) {
  case REF_CALL:
    return type == NODE_FUNCTION || type == NODE_METHOD;
  case REF_TYPE:
  case REF_NODE_TYPE:
    return type == NODE_STRUCT || type == NODE_UNION || type == NODE_TYPEDEF || type == NODE_ENUM ||
           type == NODE_CLASS || type == NODE_INTERFACE;
  case REF_INCLUDE:
    return true;
  default:
    return true;
  }
}

SymbolEntry *reference_resolver_c_cpp_find_in_included_files(ASTNode *node, const char *name,
                                                             GlobalSymbolTable *symbol_table,
                                                             ReferenceType ref_type) {
  const ASTNode *root;
  size_t i;

  if (!node || !symbol_table) {
    return NULL;
  }

  root = find_ast_root(node);
  if (!root) {
    return NULL;
  }

  for (i = 0; i < symbol_table->num_buckets; i++) {
    SymbolEntry *entry = symbol_table->buckets[i];
    while (entry) {
      bool name_matches = (ref_type == REF_INCLUDE);

      if (!name_matches && name) {
        name_matches =
            (entry->simple_name && strcmp(entry->simple_name, name) == 0) ||
            (entry->qualified_name && strcmp(entry->qualified_name, name) == 0);
      }

      if (name_matches && entry_matches_reference_type(entry, ref_type) &&
          root_has_matching_include(root, entry->file_path)) {
        return entry;
      }

      entry = entry->next;
    }
  }

  return NULL;
}

ResolutionStatus reference_resolver_c_cpp_resolve(ASTNode *node, ReferenceType ref_type,
                                                  const char *name, GlobalSymbolTable *symbol_table,
                                                  void *resolver_data, bool cpp_mode) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_FAILED;
  }

  c_cpp_stats.num_total_lookups++;

  // Handle header includes first
  if (ref_type == REF_INCLUDE) {
    SymbolEntry *header = symbol_table_lookup(symbol_table, name);
    if (!header) {
      header = reference_resolver_c_cpp_find_in_included_files(node, name, symbol_table, ref_type);
    }
    if (header && header->node) {
      ast_node_add_reference(node, header->node);
      c_cpp_stats.num_header_resolved++;
      c_cpp_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
    return RESOLUTION_FAILED;
  }

  // Handle macros
  if (ref_type == REF_USE) {
    SymbolEntry *macro = symbol_table_lookup(symbol_table, name);
    if (macro && macro->node) {
      c_cpp_stats.num_macro_resolved++;
      c_cpp_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
    return RESOLUTION_FAILED;
  }

  // Handle struct/class field access
  if (ref_type == REF_PROPERTY) {
    char struct_name[256] = {0};
    const char *separator = strchr(name, '.');
    if (!separator) {
      separator = strstr(name, "->");
    }

    if (separator) {
      size_t struct_part_len = separator - name;
      if (struct_part_len < sizeof(struct_name)) {
        strncpy(struct_name, name, struct_part_len);
        struct_name[struct_part_len] = '\0';
        const char *field_name = separator + (separator[0] == '.' ? 1 : 2);

        SymbolEntry *struct_entry = symbol_table_lookup(symbol_table, struct_name);
        if (struct_entry && struct_entry->node) {
          ASTNode *struct_node = struct_entry->node;
          for (size_t i = 0; i < struct_node->num_children; i++) {
            ASTNode *field = struct_node->children[i];
            if (field && strcmp(field->name, field_name) == 0) {
              c_cpp_stats.num_struct_fields_resolved++;
              c_cpp_stats.num_resolved++;
              return RESOLUTION_SUCCESS;
            }
          }
        }
      }
    }
  }

  // C++-specific features
  if (cpp_mode) {
    // Handle namespaces
    const char *namespace_sep = strstr(name, "::");
    if (namespace_sep) {
      char namespace_name[256] = {0};
      size_t namespace_len = namespace_sep - name;
      if (namespace_len < sizeof(namespace_name)) {
        strncpy(namespace_name, name, namespace_len);
        namespace_name[namespace_len] = '\0';

        SymbolEntry *namespace_entry = symbol_table_lookup(symbol_table, namespace_name);
        if (namespace_entry && namespace_entry->node) {
          c_cpp_stats.num_namespace_resolved++;
          c_cpp_stats.num_resolved++;
          return RESOLUTION_SUCCESS;
        }
      }
    }

    // Handle templates
    const char *template_start = strchr(name, '<');
    const char *template_end = strrchr(name, '>');
    if (template_start && template_end && template_end > template_start) {
      c_cpp_stats.num_template_resolved++;
      c_cpp_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }

    // Handle classes
    SymbolEntry *class_entry = symbol_table_lookup(symbol_table, name);
    if (class_entry && class_entry->node) {
      c_cpp_stats.num_class_resolved++;
      c_cpp_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
  }

  if (ref_type == REF_CALL || ref_type == REF_TYPE || ref_type == REF_NODE_TYPE) {
    SymbolEntry *included_symbol =
        reference_resolver_c_cpp_find_in_included_files(node, name, symbol_table, ref_type);
    if (included_symbol && included_symbol->node) {
      ast_node_add_reference(node, included_symbol->node);
      c_cpp_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
  }

  // Try generic resolution as last resort
  ResolutionStatus result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
  if (result == RESOLUTION_SUCCESS) {
    c_cpp_stats.num_resolved++;
  }
  return result;
}

void reference_resolver_c_cpp_get_stats(size_t *total, size_t *resolved, size_t *header_resolved,
                                        size_t *macro_resolved, size_t *struct_fields_resolved,
                                        size_t *class_resolved, size_t *template_resolved,
                                        size_t *namespace_resolved) {
  if (total)
    *total = c_cpp_stats.num_total_lookups;
  if (resolved)
    *resolved = c_cpp_stats.num_resolved;
  if (header_resolved)
    *header_resolved = c_cpp_stats.num_header_resolved;
  if (macro_resolved)
    *macro_resolved = c_cpp_stats.num_macro_resolved;
  if (struct_fields_resolved)
    *struct_fields_resolved = c_cpp_stats.num_struct_fields_resolved;
  if (class_resolved)
    *class_resolved = c_cpp_stats.num_class_resolved;
  if (template_resolved)
    *template_resolved = c_cpp_stats.num_template_resolved;
  if (namespace_resolved)
    *namespace_resolved = c_cpp_stats.num_namespace_resolved;
}

void reference_resolver_c_cpp_reset_stats(void) { memset(&c_cpp_stats, 0, sizeof(c_cpp_stats)); }
