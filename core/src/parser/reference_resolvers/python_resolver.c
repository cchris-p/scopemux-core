#include "../../../include/scopemux/ast.h"
#include "../../../include/scopemux/logging.h"
#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/project_context.h"
#include "../../../include/scopemux/reference_resolver.h"

#include "../../../include/scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of the generic resolver
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table);

// Forward declaration of main resolver function
ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data);

/**
 * Implementation required by test infrastructure
 * This serves as the bridge between the unit tests and the actual resolver implementation
 */
ResolutionStatus python_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data) {
  // Simply delegate to the main resolver implementation
  return reference_resolver_python(node, ref_type, name, symbol_table, resolver_data);
}

/**
 * Statistics specific to Python language resolution
 */
typedef struct {
  size_t total_lookups;
  size_t resolved_count;
  size_t import_resolved;
  size_t attribute_resolved;
  size_t builtin_resolved;
} PythonResolverStats;

static PythonResolverStats python_resolver_stats = {0};

/**
 * Python language resolver implementation
 *
 * Handles Python-specific reference resolution, including module imports,
 * dot notation for attributes, and Python's scope resolution rules
 */
ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_FAILED;
  }

  // Track statistics
  python_resolver_stats.total_lookups++;

  // Handle special cases based on reference type
  switch (ref_type) {
  case REF_IMPORT:
    // Handle module import resolution
    // This would require access to the ProjectContext
    python_resolver_stats.import_resolved++;

    // Try to find the module in the symbol table
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, name);
    if (module_entry && (module_entry->node->type == NODE_MODULE)) {

      // Add reference to the module
      /* Reference capacity checks are handled by ast_node_add_reference_with_metadata */
      ast_node_add_reference_with_metadata(node, module_entry->node, ref_type);
      return RESOLUTION_SUCCESS;
    }
    return RESOLUTION_NOT_FOUND;

  case REF_USE:
    // Handle attribute access (obj.attr)
    // This is more complex and requires context about the object type
    python_resolver_stats.attribute_resolved++;

    // For now, only handle basic module attribute access
    {
      // Parse the attribute reference (module.attribute)
      char *dot = strchr(name, '.');
      if (dot) {
        size_t module_name_len = dot - name;
        char module_name[256]; // Reasonable limit

        if (module_name_len < sizeof(module_name)) {
          strncpy(module_name, name, module_name_len);
          module_name[module_name_len] = '\0';

          // Look up the module
          SymbolEntry *mod_entry = symbol_table_lookup(symbol_table, module_name);
          if (mod_entry) {
            // Now look up the fully qualified attribute
            SymbolEntry *attr_entry = symbol_table_lookup(symbol_table, name);
            if (attr_entry) {
              // Add reference to the attribute
              /* Reference capacity checks are handled by ast_node_add_reference_with_metadata */
              ast_node_add_reference_with_metadata(node, attr_entry->node, ref_type);
              python_resolver_stats.resolved_count++;
              return RESOLUTION_SUCCESS;
            }
          }
        }
      }
    }
    break;

  default:
    // Standard symbol resolution procedure
    break;
  }

  // Python-specific scope resolution:
  // 1. Try in local scopes first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry) {
    python_resolver_stats.resolved_count++;
    // Add reference
    ast_node_add_reference_with_metadata(node, entry->node, ref_type);
    return RESOLUTION_SUCCESS;
  }

  // 2. Try in enclosing scopes
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }
  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_PYTHON);

  if (entry) {
    python_resolver_stats.resolved_count++;
    // Add reference
    ast_node_add_reference_with_metadata(node, entry->node, ref_type);
    return RESOLUTION_SUCCESS;
  }

  // 3. Look in builtins (Python has a builtin scope)
  entry = symbol_table_lookup(symbol_table, "builtins");
  if (entry && entry->node) {
    char builtin_name[256];
    snprintf(builtin_name, sizeof(builtin_name), "builtins.%s", name);
    entry = symbol_table_lookup(symbol_table, builtin_name);
    if (entry) {
      python_resolver_stats.builtin_resolved++;
      python_resolver_stats.resolved_count++;
      // Add reference
      ast_node_add_reference_with_metadata(node, entry->node, ref_type);
      return RESOLUTION_SUCCESS;
    }
  }

  // If we got this far, fallback to generic resolution
  ResolutionStatus result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);

  if (result == RESOLUTION_SUCCESS) {
    python_resolver_stats.resolved_count++;
  }

  return result;
}

/**
 * Get Python resolver statistics
 */
void python_resolver_get_stats(size_t *total, size_t *resolved, size_t *import_resolved,
                               size_t *attribute_resolved, size_t *builtin_resolved) {
  if (total)
    *total = python_resolver_stats.total_lookups;
  if (resolved)
    *resolved = python_resolver_stats.resolved_count;
  if (import_resolved)
    *import_resolved = python_resolver_stats.import_resolved;
  if (attribute_resolved)
    *attribute_resolved = python_resolver_stats.attribute_resolved;
  if (builtin_resolved)
    *builtin_resolved = python_resolver_stats.builtin_resolved;
}

/**
 * Reset Python resolver statistics
 */
void python_resolver_reset_stats() {
  memset(&python_resolver_stats, 0, sizeof(python_resolver_stats));
}
