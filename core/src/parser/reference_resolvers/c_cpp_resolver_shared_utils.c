#include "c_cpp_resolver_shared_utils.h"
#include <string.h>

static ReferenceResolverCCppStats c_cpp_stats = {0};

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
    if (header && header->node) {
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
