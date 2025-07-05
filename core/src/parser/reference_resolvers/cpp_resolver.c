#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
ResolutionStatus reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                    const char *name,
                                                    GlobalSymbolTable *symbol_table);

// Forward declaration for C resolver to reuse functionality
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data);

/**
 * Statistics specific to C++ language resolution
 */
typedef struct {
  size_t total_lookups;
  size_t resolved_count;
  size_t namespace_resolved;
  size_t template_resolved;
  size_t method_resolved;
  size_t class_resolved;
} CppResolverStats;

static CppResolverStats cpp_resolver_stats = {0};

/**
 * C++ language resolver implementation
 *
 * Handles C++-specific reference resolution, including namespaces,
 * templates, classes and inheritance, method resolution with overloading
 */
ResolutionStatus reference_resolver_cpp(ASTNode *node, ReferenceType ref_type, const char *name,
                                        GlobalSymbolTable *symbol_table, void *resolver_data) {
  if (!node || !name || !symbol_table) {
    return RESOLUTION_FAILED;
  }

  // Track statistics
  cpp_resolver_stats.total_lookups++;

  // Handle special cases based on reference type
  switch (ref_type) {
  case REF_INCLUDE:
    // Similar to C, delegate to C resolver for include handling
    return reference_resolver_c(node, ref_type, name, symbol_table, NULL);

  case REF_TYPE: // Handles namespace and class/type references
  {
    // First, try to resolve as a namespace
    cpp_resolver_stats.namespace_resolved++;
    SymbolEntry *namespace_entry = symbol_table_lookup(symbol_table, name);
    if (namespace_entry && namespace_entry->node->type == NODE_NAMESPACE) {
      ast_node_add_reference_with_metadata(node, namespace_entry->node, ref_type);
      cpp_resolver_stats.resolved_count++;
      return RESOLUTION_SUCCESS;
    }

    // Next, try to resolve as a class/struct
    cpp_resolver_stats.class_resolved++;
    SymbolEntry *class_entry = symbol_table_lookup(symbol_table, name);
    if (class_entry &&
        (class_entry->node->type == NODE_CLASS || class_entry->node->type == NODE_STRUCT)) {
      ast_node_add_reference_with_metadata(node, class_entry->node, ref_type);
      cpp_resolver_stats.resolved_count++;
      return RESOLUTION_SUCCESS;
    }

    // Handle namespaced classes (Namespace::Class)
    char *double_colon = strstr(name, "::");
    if (double_colon) {
      size_t namespace_len = double_colon - name;
      char namespace[256];
      if (namespace_len < sizeof(namespace)) {
        strncpy(namespace, name, namespace_len);
        namespace[namespace_len] = '\0';

        // Look up the namespace
        SymbolEntry *ns_entry = symbol_table_lookup(symbol_table, namespace);
        if (ns_entry && ns_entry->node->type == NODE_NAMESPACE) {
          // Now look up the fully qualified class
          SymbolEntry *class_entry = symbol_table_lookup(symbol_table, name);
          if (class_entry) {
            ast_node_add_reference_with_metadata(node, class_entry->node, ref_type);
            cpp_resolver_stats.resolved_count++;
            return RESOLUTION_SUCCESS;
          }
        }
      }
    }
    return RESOLUTION_NOT_FOUND;
  }

  case REF_TEMPLATE:
    // Handle template specialization
    cpp_resolver_stats.template_resolved++;
    // Extract template name without parameters
    char template_name[256] = {0};
    char *lt_pos = strchr(name, '<');
    if (lt_pos) {
      size_t name_len = lt_pos - name;
      if (name_len < sizeof(template_name)) {
        strncpy(template_name, name, name_len);
        template_name[name_len] = '\0';
        // Look up the template
        SymbolEntry *template_entry = symbol_table_lookup(symbol_table, template_name);
        if (template_entry) {
          ast_node_add_reference_with_metadata(node, template_entry->node, ref_type);
          cpp_resolver_stats.resolved_count++;
          return RESOLUTION_SUCCESS;
        }
      }
    }
    break;

  case REF_CALL: // Handles method and function call references
  {
    // Handle method resolution, including overloaded methods
    cpp_resolver_stats.method_resolved++;

    // Method resolution in C++ is complex due to overloading
    // For now, resolve by looking up the fully qualified name
    SymbolEntry *method_entry = symbol_table_lookup(symbol_table, name);
    if (method_entry) {
      ast_node_add_reference_with_metadata(node, method_entry->node, ref_type);
      cpp_resolver_stats.resolved_count++;
      return RESOLUTION_SUCCESS;
    }
    // Optionally, try resolving in class scope if available
    char *double_colon = strstr(name, "::");
    if (double_colon) {
      size_t class_name_len = double_colon - name;
      char class_name[256];
      if (class_name_len < sizeof(class_name)) {
        strncpy(class_name, name, class_name_len);
        class_name[class_name_len] = '\0';

        // Look up the class
        SymbolEntry *class_entry = symbol_table_lookup(symbol_table, class_name);
        if (class_entry) {
          // Now look up the fully qualified method
          SymbolEntry *method_entry = symbol_table_lookup(symbol_table, name);
          if (method_entry) {
            ast_node_add_reference_with_metadata(node, method_entry->node, ref_type);
            cpp_resolver_stats.resolved_count++;
            return RESOLUTION_SUCCESS;
          }
        }
      }
    }
    break;
  }

  default:
    // Fallback: try C++-specific rules, then fall back to C resolver
    break;
  }

  // C++ scope resolution with namespaces:
  // 1. Try exact name match first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry) {
    cpp_resolver_stats.resolved_count++;
    ast_node_add_reference_with_metadata(node, entry->node, ref_type);
    return RESOLUTION_SUCCESS;
  }

  // 2. Try in current scope chain
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }
  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_CPP);
  if (entry) {
    cpp_resolver_stats.resolved_count++;
    ast_node_add_reference_with_metadata(node, entry->node, ref_type);
    return RESOLUTION_SUCCESS;
  }

  // 3. Check for the symbol in the global namespace (::symbol)
  char global_name[256];
  snprintf(global_name, sizeof(global_name), "::%s", name);
  entry = symbol_table_lookup(symbol_table, global_name);
  if (entry) {
    cpp_resolver_stats.resolved_count++;
    ast_node_add_reference_with_metadata(node, entry->node, ref_type);
    return RESOLUTION_SUCCESS;
  }

  // 4. If not found so far, fall back to C resolver
  // Many C constructs are valid in C++ as well
  ResolutionStatus result = reference_resolver_c(node, ref_type, name, symbol_table, NULL);
  if (result == RESOLUTION_SUCCESS) {
    cpp_resolver_stats.resolved_count++;
  }
  return result;
}

/**
 * Get C++ resolver statistics
 */
void cpp_resolver_get_stats(size_t *total, size_t *resolved, size_t *namespace_resolved,
                            size_t *template_resolved, size_t *method_resolved,
                            size_t *class_resolved) {
  if (total)
    *total = cpp_resolver_stats.total_lookups;
  if (resolved)
    *resolved = cpp_resolver_stats.resolved_count;
  if (namespace_resolved)
    *namespace_resolved = cpp_resolver_stats.namespace_resolved;
  if (template_resolved)
    *template_resolved = cpp_resolver_stats.template_resolved;
  if (method_resolved)
    *method_resolved = cpp_resolver_stats.method_resolved;
  if (class_resolved)
    *class_resolved = cpp_resolver_stats.class_resolved;
}

/**
 * Reset C++ resolver statistics
 */
void cpp_resolver_reset_stats() { memset(&cpp_resolver_stats, 0, sizeof(cpp_resolver_stats)); }
