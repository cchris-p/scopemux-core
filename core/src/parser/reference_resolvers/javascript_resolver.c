#include "../../../include/scopemux/ast.h"
#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/project_context.h"
#include "../../../include/scopemux/reference_resolver.h"
#include "../../../include/scopemux/source_range.h"
#include "../../../include/scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of the generic resolver
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table);

// Forward declaration of main resolver function
ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

/**
 * Implementation required by test infrastructure
 * This serves as the bridge between the unit tests and the actual resolver implementation
 */
ResolutionStatus javascript_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                          GlobalSymbolTable *symbol_table, void *resolver_data) {
  // Simply delegate to the main resolver implementation
  return reference_resolver_javascript(node, ref_type, name, symbol_table, resolver_data);
}

/**
 * Statistics specific to JavaScript language resolution
 */
typedef struct {
  size_t total_lookups;
  size_t resolved_count;
  size_t import_resolved;
  size_t property_resolved;
  size_t prototype_resolved;
} JavaScriptResolverStats;

static JavaScriptResolverStats js_resolver_stats = {0};

/**
 * JavaScript language resolver implementation
 *
 * Handles JavaScript-specific reference resolution, including ES module imports,
 * CommonJS requires, property access, and prototype chain resolution.
 */
ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_FAILED;
  }

  // Track statistics
  js_resolver_stats.total_lookups++;

  // Handle special cases based on reference type
  switch (ref_type) {
  case REF_IMPORT: {
    js_resolver_stats.import_resolved++;
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, name);
    if (module_entry && module_entry->node->type == NODE_MODULE) {
      ast_node_add_reference_with_metadata(node, module_entry->node, ref_type);
      return RESOLUTION_SUCCESS;
    }
    return RESOLUTION_NOT_FOUND;
  }
  case REF_PROPERTY: {
    js_resolver_stats.property_resolved++;
    char *dot = strchr(name, '.');
    if (dot) {
      size_t obj_name_len = dot - name;
      char obj_name[256];
      if (obj_name_len < sizeof(obj_name)) {
        strncpy(obj_name, name, obj_name_len);
        obj_name[obj_name_len] = '\0';
        SymbolEntry *obj_entry = symbol_table_lookup(symbol_table, obj_name);
        if (obj_entry) {
          SymbolEntry *prop_entry = symbol_table_lookup(symbol_table, name);
          if (prop_entry) {
            ast_node_add_reference_with_metadata(node, prop_entry->node, ref_type);
            js_resolver_stats.resolved_count++;
            return RESOLUTION_SUCCESS;
          } else {
            // TODO: If not found directly, search prototype chain
          }
        }
      }
    }
    break;
  }
  default:
    break;
  }

  // Fallback: generic resolution
  ResolutionStatus result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
  if (result == RESOLUTION_SUCCESS)
    js_resolver_stats.resolved_count++;
  return result;
}

/**
 * Get JavaScript resolver statistics
 */
void javascript_resolver_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                                   size_t *property_resolved, size_t *prototype_resolved) {
  if (total)
    *total = js_resolver_stats.total_lookups;
  if (resolved)
    *resolved = js_resolver_stats.resolved_count;
  if (import_resolved)
    *import_resolved = js_resolver_stats.import_resolved;
  if (property_resolved)
    *property_resolved = js_resolver_stats.property_resolved;
  if (prototype_resolved)
    *prototype_resolved = js_resolver_stats.prototype_resolved;
}

/**
 * Reset JavaScript resolver statistics
 */
void javascript_resolver_reset_stats() { memset(&js_resolver_stats, 0, sizeof(js_resolver_stats)); }
