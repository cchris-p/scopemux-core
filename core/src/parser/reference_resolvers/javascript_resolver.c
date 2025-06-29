#include "../../../include/scopemux/ast_node.h"
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
  case REFERENCE_IMPORT:
    // Handle ES module import or CommonJS require
    js_resolver_stats.import_resolved++;

    // Try to find the module in the symbol table
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, name);
    if (module_entry && module_entry->node->type == NODE_MODULE) {
      // Add reference to the module
      if (node->num_references < node->references_capacity) {
        node->references[node->num_references++] = module_entry->node;
        return RESOLUTION_SUCCESS;
      } else {
        // Resize references array
        size_t new_capacity = node->references_capacity * 2;
        if (new_capacity == 0)
          new_capacity = 4;

        ASTNode **new_refs =
            (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

        if (new_refs) {
          node->references = new_refs;
          node->references_capacity = new_capacity;
          node->references[node->num_references++] = module_entry->node;
          return RESOLUTION_SUCCESS;
        }
      }
    }
    return RESOLUTION_NOT_FOUND;

  case REFERENCE_PROPERTY:
    // Handle property access (obj.prop)
    js_resolver_stats.property_resolved++;

    // Parse the property access (object.property)
    {
      char *dot = strchr(name, '.');
      if (dot) {
        size_t obj_name_len = dot - name;
        char obj_name[256]; // Reasonable limit

        if (obj_name_len < sizeof(obj_name)) {
          strncpy(obj_name, name, obj_name_len);
          obj_name[obj_name_len] = '\0';

          // Look up the object
          SymbolEntry *obj_entry = symbol_table_lookup(symbol_table, obj_name);
          if (obj_entry) {
            // Now look up the fully qualified property
            SymbolEntry *prop_entry = symbol_table_lookup(symbol_table, name);
            if (prop_entry) {
              // Add reference to the property
              if (node->num_references < node->references_capacity) {
                node->references[node->num_references++] = prop_entry->node;
                js_resolver_stats.resolved_count++;
                return RESOLUTION_SUCCESS;
              } else {
                // Resize references array
                size_t new_capacity = node->references_capacity * 2;
                if (new_capacity == 0)
                  new_capacity = 4;

                ASTNode **new_refs =
                    (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

                if (new_refs) {
                  node->references = new_refs;
                  node->references_capacity = new_capacity;
                  node->references[node->num_references++] = prop_entry->node;
                  js_resolver_stats.resolved_count++;
                  return RESOLUTION_SUCCESS;
                }
              }
            }

            // TODO: If not found directly, we should search the prototype chain
            // JavaScript has prototype-based inheritance
          }
        }
      }
    }
    break;

  case REFERENCE_PROTOTYPE:
    // Handle prototype chain access
    js_resolver_stats.prototype_resolved++;
    // TODO: Implement prototype chain resolution
    // This is a more complex operation that requires tracking prototype relationships
    return RESOLUTION_NOT_SUPPORTED;

  default:
    // Standard symbol resolution procedure
    break;
  }

  // JavaScript scope resolution has some unique characteristics:
  // - Variable hoisting
  // - Function scope vs block scope (let/const vs var)
  // - Global object properties

  // 1. Try exact name match first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry) {
    js_resolver_stats.resolved_count++;
    // Add reference
    if (node->num_references < node->references_capacity) {
      node->references[node->num_references++] = entry->node;
      return RESOLUTION_SUCCESS;
    } else {
      // Resize references array
      size_t new_capacity = node->references_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 4;

      ASTNode **new_refs = (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

      if (new_refs) {
        node->references = new_refs;
        node->references_capacity = new_capacity;
        node->references[node->num_references++] = entry->node;
        return RESOLUTION_SUCCESS;
      }
    }
    return RESOLUTION_FAILED; // Failed to add reference
  }

  // 2. Try in enclosing function scopes (simulate hoisting and closure behavior)
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_JAVASCRIPT);
  if (entry) {
    js_resolver_stats.resolved_count++;
    // Add reference
    if (node->num_references < node->references_capacity) {
      node->references[node->num_references++] = entry->node;
      return RESOLUTION_SUCCESS;
    } else {
      // Resize references array
      size_t new_capacity = node->references_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 4;

      ASTNode **new_refs = (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

      if (new_refs) {
        node->references = new_refs;
        node->references_capacity = new_capacity;
        node->references[node->num_references++] = entry->node;
        return RESOLUTION_SUCCESS;
      }
    }
    return RESOLUTION_FAILED; // Failed to add reference
  }

  // 3. Check global object (window in browsers, global in Node.js)
  entry = symbol_table_lookup(symbol_table, "global");
  if (entry && entry->node) {
    char global_name[256];
    snprintf(global_name, sizeof(global_name), "global.%s", name);
    entry = symbol_table_lookup(symbol_table, global_name);
    if (entry) {
      js_resolver_stats.resolved_count++;
      // Add reference
      if (node->num_references < node->references_capacity) {
        node->references[node->num_references++] = entry->node;
        return RESOLUTION_SUCCESS;
      } else {
        // Resize references array
        size_t new_capacity = node->references_capacity * 2;
        if (new_capacity == 0)
          new_capacity = 4;

        ASTNode **new_refs =
            (ASTNode **)realloc(node->references, new_capacity * sizeof(ASTNode *));

        if (new_refs) {
          node->references = new_refs;
          node->references_capacity = new_capacity;
          node->references[node->num_references++] = entry->node;
          return RESOLUTION_SUCCESS;
        }
      }
      return RESOLUTION_FAILED; // Failed to add reference
    }
  }

  // If we got this far, fallback to generic resolution
  ResolutionStatus result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);

  if (result == RESOLUTION_SUCCESS) {
    js_resolver_stats.resolved_count++;
  }

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
