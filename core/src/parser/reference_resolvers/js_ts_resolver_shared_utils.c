#include "js_ts_resolver_shared_utils.h"
#include "scopemux/reference_resolver.h"

#include <string.h>

static ReferenceResolverJsTsStats js_ts_stats = {0};

ResolutionStatus reference_resolver_js_ts_resolve(ASTNode *node, ReferenceType ref_type,
                                                  const char *name, GlobalSymbolTable *symbol_table,
                                                  void *resolver_data, bool typescript_mode) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_FAILED;
  }

  js_ts_stats.num_total_lookups++;

  // Handle imports first
  if (ref_type == REF_IMPORT) {
    SymbolEntry *import = symbol_table_lookup(symbol_table, name);
    if (import && import->node) {
      js_ts_stats.num_import_resolved++;
      js_ts_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
    return RESOLUTION_FAILED;
  }

  // Handle modules
  if (ref_type == REF_USE) {
    SymbolEntry *module = symbol_table_lookup(symbol_table, name);
    if (module && module->node) {
      js_ts_stats.num_module_resolved++;
      js_ts_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
    return RESOLUTION_FAILED;
  }

  // Handle classes
  if (ref_type == REF_TYPE) {
    SymbolEntry *class_entry = symbol_table_lookup(symbol_table, name);
    if (class_entry && class_entry->node) {
      js_ts_stats.num_class_resolved++;
      js_ts_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
  }

  // TypeScript-specific features
  if (typescript_mode) {
    // Handle types
    if (ref_type == REF_TYPE) {
      SymbolEntry *type_entry = symbol_table_lookup(symbol_table, name);
      if (type_entry && type_entry->node) {
        js_ts_stats.num_type_resolved++;
        js_ts_stats.num_resolved++;
        return RESOLUTION_SUCCESS;
      }
    }

    // Handle interfaces
    if (ref_type == REF_INTERFACE) {
      SymbolEntry *interface_entry = symbol_table_lookup(symbol_table, name);
      if (interface_entry && interface_entry->node) {
        js_ts_stats.num_interface_resolved++;
        js_ts_stats.num_resolved++;
        return RESOLUTION_SUCCESS;
      }
    }

    // Handle generics
    const char *generic_start = strchr(name, '<');
    const char *generic_end = strrchr(name, '>');
    if (generic_start && generic_end && generic_end > generic_start) {
      js_ts_stats.num_generic_resolved++;
      js_ts_stats.num_resolved++;
      return RESOLUTION_SUCCESS;
    }
  }

  // Try generic resolution as last resort
  ResolutionStatus result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
  if (result == RESOLUTION_SUCCESS) {
    js_ts_stats.num_resolved++;
  }
  return result;
}

void reference_resolver_js_ts_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                                        size_t *module_resolved, size_t *class_resolved,
                                        size_t *type_resolved, size_t *interface_resolved,
                                        size_t *generic_resolved) {
  if (total)
    *total = js_ts_stats.num_total_lookups;
  if (resolved)
    *resolved = js_ts_stats.num_resolved;
  if (import_resolved)
    *import_resolved = js_ts_stats.num_import_resolved;
  if (module_resolved)
    *module_resolved = js_ts_stats.num_module_resolved;
  if (class_resolved)
    *class_resolved = js_ts_stats.num_class_resolved;
  if (type_resolved)
    *type_resolved = js_ts_stats.num_type_resolved;
  if (interface_resolved)
    *interface_resolved = js_ts_stats.num_interface_resolved;
  if (generic_resolved)
    *generic_resolved = js_ts_stats.num_generic_resolved;
}

void reference_resolver_js_ts_reset_stats(void) { memset(&js_ts_stats, 0, sizeof(js_ts_stats)); }
