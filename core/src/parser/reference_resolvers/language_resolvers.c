/**
 * @file language_resolvers.c
 * @brief Language-specific reference resolution implementations
 *
 * This file contains the resolver implementations for each supported language:
 * - C
 * - Python
 * - JavaScript
 * - TypeScript
 *
 * Each resolver handles language-specific scoping rules and symbol resolution.
 */

#include "scopemux/ast.h"
#include "scopemux/language.h"
#include "scopemux/logging.h"
#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations of helper functions for C resolver
static ResolutionStatus resolve_c_function(ASTNode *node, const char *name,
                                           GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_c_variable(ASTNode *node, const char *name,
                                           GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_c_type(ASTNode *node, const char *name,
                                       GlobalSymbolTable *symbol_table);

// Forward declarations of helper functions for Python resolver
static ResolutionStatus resolve_python_function(ASTNode *node, const char *name,
                                                GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_python_variable(ASTNode *node, const char *name,
                                                GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_python_class(ASTNode *node, const char *name,
                                             GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_python_import(ASTNode *node, const char *name,
                                              GlobalSymbolTable *symbol_table);

// Forward declarations of helper functions for JavaScript resolver
static ResolutionStatus resolve_javascript_function(ASTNode *node, const char *name,
                                                    GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_javascript_variable(ASTNode *node, const char *name,
                                                    GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_javascript_class(ASTNode *node, const char *name,
                                                 GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_javascript_import(ASTNode *node, const char *name,
                                                  GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_javascript_property(ASTNode *node, const char *name,
                                                    GlobalSymbolTable *symbol_table);

// Forward declarations of helper functions for TypeScript resolver
static ResolutionStatus resolve_typescript_interface(ASTNode *node, const char *name,
                                                     GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_typescript_generic(ASTNode *node, const char *name,
                                                   GlobalSymbolTable *symbol_table);
static ResolutionStatus resolve_typescript_type(ASTNode *node, const char *name,
                                                GlobalSymbolTable *symbol_table);

/**
 * C language reference resolver entrypoint (prototype only).
 * Implementation is in c_resolver.c.
 */
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type, const char *name,
                                      GlobalSymbolTable *symbol_table, void *resolver_data);

/**
 * Helper function to resolve C function references
 */
static ResolutionStatus resolve_c_function(ASTNode *node, const char *name,
                                           GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_FUNCTION) {
    // Found exact match
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_C);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_FUNCTION) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Check for function in included files
  // TODO: Implement include-based lookup

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve C variable references
 */
static ResolutionStatus resolve_c_variable(ASTNode *node, const char *name,
                                           GlobalSymbolTable *symbol_table) {
  // Start with local scope and work outward
  ASTNode *scope_node = node;
  while (scope_node) {
    // Build scoped name
    char scoped_name[256];
    if (scope_node->qualified_name) {
      snprintf(scoped_name, sizeof(scoped_name), "%s::%s", scope_node->qualified_name, name);
    } else {
      strncpy(scoped_name, name, sizeof(scoped_name) - 1);
      scoped_name[sizeof(scoped_name) - 1] = '\0';
    }

    // Try lookup in current scope
    SymbolEntry *entry = symbol_table_lookup(symbol_table, scoped_name);
    if (entry && entry->symbol &&
        (entry->symbol->node->type == NODE_VARIABLE ||
         entry->symbol->node->type == NODE_PARAMETER)) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }

    // Move to parent scope
    scope_node = scope_node->parent;
  }

  // Try global scope as last resort
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_VARIABLE || entry->symbol->node->type == NODE_PARAMETER)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve C type references
 */
static ResolutionStatus resolve_c_type(ASTNode *node, const char *name,
                                       GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_STRUCT || entry->symbol->node->type == NODE_UNION ||
       entry->symbol->node->type == NODE_TYPEDEF || entry->symbol->node->type == NODE_ENUM)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_C);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_STRUCT || entry->symbol->node->type == NODE_UNION ||
       entry->symbol->node->type == NODE_TYPEDEF || entry->symbol->node->type == NODE_ENUM)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Check for type in included files
  // TODO: Implement include-based lookup

  return RESOLUTION_NOT_FOUND;
}

/**
 * Python language reference resolver entrypoint (prototype only).
 * Implementation is in python_resolver.c.
 */
ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type, const char *name,
                                           GlobalSymbolTable *symbol_table, void *resolver_data);

/**
 * Helper function to resolve Python function references
 */
static ResolutionStatus resolve_python_function(ASTNode *node, const char *name,
                                                GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_FUNCTION || entry->symbol->node->type == NODE_METHOD)) {
    // Found exact match
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Try module-qualified lookup
  // In Python, functions can be accessed via module.function
  const char *dot = strchr(name, '.');
  if (dot) {
    // Extract module name (part before the dot)
    size_t module_len = dot - name;
    char *module_name = (char *)malloc(module_len + 1);
    if (!module_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(module_name, name, module_len);
    module_name[module_len] = '\0';

    // Look up the module
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, module_name);
    free(module_name);

    if (module_entry && module_entry->symbol && module_entry->symbol->node->type == NODE_MODULE) {
      // Found the module, now look for the function within it
      const char *func_name = dot + 1;

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", module_entry->qualified_name,
               func_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol &&
          (entry->symbol->node->type == NODE_FUNCTION ||
           entry->symbol->node->type == NODE_METHOD)) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_PYTHON);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_FUNCTION || entry->symbol->node->type == NODE_METHOD)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve Python variable references
 */
static ResolutionStatus resolve_python_variable(ASTNode *node, const char *name,
                                                GlobalSymbolTable *symbol_table) {
  // Start with local scope and work outward
  ASTNode *scope_node = node;
  while (scope_node) {
    // Build scoped name
    char scoped_name[256];
    if (scope_node->qualified_name) {
      snprintf(scoped_name, sizeof(scoped_name), "%s.%s", scope_node->qualified_name, name);
    } else {
      strncpy(scoped_name, name, sizeof(scoped_name) - 1);
      scoped_name[sizeof(scoped_name) - 1] = '\0';
    }

    // Try lookup in current scope
    SymbolEntry *entry = symbol_table_lookup(symbol_table, scoped_name);
    if (entry && entry->symbol &&
        (entry->symbol->node->type == NODE_VARIABLE ||
         entry->symbol->node->type == NODE_PARAMETER)) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }

    // Move to parent scope
    scope_node = scope_node->parent;
  }

  // Try global scope as last resort
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_VARIABLE || entry->symbol->node->type == NODE_PARAMETER)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve Python class references
 */
static ResolutionStatus resolve_python_class(ASTNode *node, const char *name,
                                             GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_CLASS) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Try module-qualified lookup
  const char *dot = strchr(name, '.');
  if (dot) {
    // Extract module name (part before the dot)
    size_t module_len = dot - name;
    char *module_name = (char *)malloc(module_len + 1);
    if (!module_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(module_name, name, module_len);
    module_name[module_len] = '\0';

    // Look up the module
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, module_name);
    free(module_name);

    if (module_entry && module_entry->symbol && module_entry->symbol->node->type == NODE_MODULE) {
      // Found the module, now look for the class within it
      const char *class_name = dot + 1;

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", module_entry->qualified_name,
               class_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol && entry->symbol->node->type == NODE_CLASS) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_PYTHON);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_CLASS) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve Python import references
 */
static ResolutionStatus resolve_python_import(ASTNode *node, const char *name,
                                              GlobalSymbolTable *symbol_table) {
  // Try direct lookup for the module
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_MODULE) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle from X import Y style imports
  const char *import_sep = strstr(name, " import ");
  if (import_sep) {
    // Extract module name (part before " import ")
    size_t module_len = import_sep - name;
    char *module_name = (char *)malloc(module_len + 1);
    if (!module_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(module_name, name, module_len);
    module_name[module_len] = '\0';

    // Look up the module
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, module_name);
    free(module_name);

    if (module_entry && module_entry->symbol && module_entry->symbol->node->type == NODE_MODULE) {
      // Found the module, now look for the imported symbol
      const char *imported_name = import_sep + 8; // Skip " import "

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", module_entry->qualified_name,
               imported_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve TypeScript interface references
 */
static ResolutionStatus resolve_typescript_interface(ASTNode *node, const char *name,
                                                     GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_INTERFACE) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle namespace-qualified interfaces
  const char *dot = strchr(name, '.');
  if (dot) {
    // Extract namespace name (part before the dot)
    size_t ns_len = dot - name;
    char *ns_name = (char *)malloc(ns_len + 1);
    if (!ns_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(ns_name, name, ns_len);
    ns_name[ns_len] = '\0';

    // Look up the namespace
    SymbolEntry *ns_entry = symbol_table_lookup(symbol_table, ns_name);
    free(ns_name);

    if (ns_entry && ns_entry->symbol &&
        (ns_entry->symbol->node->type == NODE_NAMESPACE ||
         ns_entry->symbol->node->type == NODE_MODULE)) {
      // Found the namespace, now look for the interface within it
      const char *interface_name = dot + 1;

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", ns_entry->qualified_name,
               interface_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol && entry->symbol->node->type == NODE_INTERFACE) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_TYPESCRIPT);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_INTERFACE) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve TypeScript generic type parameters
 */
static ResolutionStatus resolve_typescript_generic(ASTNode *node, const char *name,
                                                   GlobalSymbolTable *symbol_table) {
  // Check if this is a generic type parameter in the current scope
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  // Try to find the generic parameter in the current scope
  SymbolEntry *entry =
      symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_TYPESCRIPT);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_PARAMETER) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle generic type with constraints
  // Extract base type name (before angle brackets)
  char base_type[256];
  strncpy(base_type, name, sizeof(base_type) - 1);
  base_type[sizeof(base_type) - 1] = '\0';

  char *angle_bracket = strchr(base_type, '<');
  if (angle_bracket) {
    *angle_bracket = '\0'; // Truncate at the angle bracket

    // Look up the base type
    entry = symbol_table_lookup(symbol_table, base_type);
    if (entry && entry->symbol &&
        (entry->symbol->node->type == NODE_TYPE || entry->symbol->node->type == NODE_INTERFACE ||
         entry->symbol->node->type == NODE_CLASS)) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve TypeScript type references
 */
static ResolutionStatus resolve_typescript_type(ASTNode *node, const char *name,
                                                GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_TYPE || entry->symbol->node->type == NODE_INTERFACE ||
       entry->symbol->node->type == NODE_ENUM)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle primitive types (these don't need resolution)
  if (strcmp(name, "string") == 0 || strcmp(name, "number") == 0 || strcmp(name, "boolean") == 0 ||
      strcmp(name, "any") == 0 || strcmp(name, "void") == 0 || strcmp(name, "undefined") == 0 ||
      strcmp(name, "null") == 0 || strcmp(name, "never") == 0 || strcmp(name, "object") == 0 ||
      strcmp(name, "unknown") == 0) {
    // No need to add a reference for primitive types
    return RESOLUTION_SUCCESS;
  }

  // Handle namespace-qualified types
  const char *dot = strchr(name, '.');
  if (dot) {
    // Extract namespace name (part before the dot)
    size_t ns_len = dot - name;
    char *ns_name = (char *)malloc(ns_len + 1);
    if (!ns_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(ns_name, name, ns_len);
    ns_name[ns_len] = '\0';

    // Look up the namespace
    SymbolEntry *ns_entry = symbol_table_lookup(symbol_table, ns_name);
    free(ns_name);

    if (ns_entry && ns_entry->symbol &&
        (ns_entry->symbol->node->type == NODE_NAMESPACE ||
         ns_entry->symbol->node->type == NODE_MODULE)) {
      // Found the namespace, now look for the type within it
      const char *type_name = dot + 1;

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", ns_entry->qualified_name,
               type_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol &&
          (entry->symbol->node->type == NODE_TYPE || entry->symbol->node->type == NODE_INTERFACE ||
           entry->symbol->node->type == NODE_ENUM)) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_TYPESCRIPT);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_TYPE || entry->symbol->node->type == NODE_INTERFACE ||
       entry->symbol->node->type == NODE_ENUM)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * JavaScript language reference resolver entrypoint (prototype only).
 * Implementation is in javascript_resolver.c.
 */
ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);

/**
 * Helper function to resolve JavaScript function references
 */
static ResolutionStatus resolve_javascript_function(ASTNode *node, const char *name,
                                                    GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_FUNCTION || entry->symbol->node->type == NODE_METHOD)) {
    // Found exact match
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle module exports (CommonJS)
  // Look for module.exports.functionName or exports.functionName
  if (strstr(name, "module.exports.") == name) {
    const char *func_name = name + 15; // Skip "module.exports."
    entry = symbol_table_lookup(symbol_table, func_name);
    if (entry && entry->symbol &&
        (entry->symbol->node->type == NODE_FUNCTION || entry->symbol->node->type == NODE_METHOD)) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }
  } else if (strstr(name, "exports.") == name) {
    const char *func_name = name + 8; // Skip "exports."
    entry = symbol_table_lookup(symbol_table, func_name);
    if (entry && entry->symbol &&
        (entry->symbol->node->type == NODE_FUNCTION || entry->symbol->node->type == NODE_METHOD)) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }
  }

  // Handle ES6 module imports
  // For imported functions, they might be referenced directly

  // Try scope-based lookup (accounting for hoisting)
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_JAVASCRIPT);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_FUNCTION || entry->symbol->node->type == NODE_METHOD)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle prototype methods
  const char *dot = strchr(name, '.');
  if (dot && strstr(dot, ".prototype.")) {
    // Extract class name (part before .prototype)
    size_t class_len = dot - name;
    char *class_name = (char *)malloc(class_len + 1);
    if (!class_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(class_name, name, class_len);
    class_name[class_len] = '\0';

    // Look up the class
    SymbolEntry *class_entry = symbol_table_lookup(symbol_table, class_name);
    free(class_name);

    if (class_entry && class_entry->symbol && class_entry->symbol->node->type == NODE_CLASS) {
      // Found the class, now look for the method
      const char *method_name = strstr(dot, ".prototype.") + 11; // Skip ".prototype."

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", class_entry->qualified_name,
               method_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol && entry->symbol->node->type == NODE_METHOD) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve JavaScript variable references
 */
static ResolutionStatus resolve_javascript_variable(ASTNode *node, const char *name,
                                                    GlobalSymbolTable *symbol_table) {
  // Start with local scope and work outward
  ASTNode *scope_node = node;
  while (scope_node) {
    // Build scoped name
    char scoped_name[256];
    if (scope_node->qualified_name) {
      snprintf(scoped_name, sizeof(scoped_name), "%s.%s", scope_node->qualified_name, name);
    } else {
      strncpy(scoped_name, name, sizeof(scoped_name) - 1);
      scoped_name[sizeof(scoped_name) - 1] = '\0';
    }

    // Try lookup in current scope
    SymbolEntry *entry = symbol_table_lookup(symbol_table, scoped_name);
    if (entry && entry->symbol &&
        (entry->symbol->node->type == NODE_VARIABLE ||
         entry->symbol->node->type == NODE_PARAMETER)) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }

    // Move to parent scope
    scope_node = scope_node->parent;
  }

  // Try global scope as last resort (accounting for hoisting)
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol &&
      (entry->symbol->node->type == NODE_VARIABLE || entry->symbol->node->type == NODE_PARAMETER)) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve JavaScript class references
 */
static ResolutionStatus resolve_javascript_class(ASTNode *node, const char *name,
                                                 GlobalSymbolTable *symbol_table) {
  // Try direct lookup first
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_CLASS) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle ES6 module imports
  const char *dot = strchr(name, '.');
  if (dot) {
    // Extract module name (part before the dot)
    size_t module_len = dot - name;
    char *module_name = (char *)malloc(module_len + 1);
    if (!module_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(module_name, name, module_len);
    module_name[module_len] = '\0';

    // Look up the module
    SymbolEntry *module_entry = symbol_table_lookup(symbol_table, module_name);
    free(module_name);

    if (module_entry && module_entry->symbol && module_entry->symbol->node->type == NODE_MODULE) {
      // Found the module, now look for the class within it
      const char *class_name = dot + 1;

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", module_entry->qualified_name,
               class_name);

      entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol && entry->symbol->node->type == NODE_CLASS) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  // Try scope-based lookup
  char *current_scope = NULL;
  if (node->parent && node->parent->qualified_name) {
    current_scope = node->parent->qualified_name;
  }

  entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_JAVASCRIPT);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_CLASS) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve JavaScript import references
 */
static ResolutionStatus resolve_javascript_import(ASTNode *node, const char *name,
                                                  GlobalSymbolTable *symbol_table) {
  // Try direct lookup for the module
  SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
  if (entry && entry->symbol && entry->symbol->node->type == NODE_MODULE) {
    ast_node_add_reference(node, entry->node);
    return RESOLUTION_SUCCESS;
  }

  // Handle ES6 import syntax
  // import { X } from 'module'
  const char *import_from = strstr(name, "from ");
  if (import_from) {
    // Extract module name (part after "from ")
    const char *module_name_start = import_from + 5; // Skip "from "

    // Remove quotes if present
    if (*module_name_start == '\'' || *module_name_start == '"') {
      module_name_start++;
    }

    char module_name[256];
    strncpy(module_name, module_name_start, sizeof(module_name) - 1);
    module_name[sizeof(module_name) - 1] = '\0';

    // Remove trailing quote if present
    size_t len = strlen(module_name);
    if (len > 0 && (module_name[len - 1] == '\'' || module_name[len - 1] == '"')) {
      module_name[len - 1] = '\0';
    }

    // Look up the module
    entry = symbol_table_lookup(symbol_table, module_name);
    if (entry && entry->symbol && entry->symbol->node->type == NODE_MODULE) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }
  }

  // Handle CommonJS require
  // const X = require('module')
  const char *require_start = strstr(name, "require(");
  if (require_start) {
    // Extract module name (part inside require())
    const char *module_name_start = require_start + 8; // Skip "require("

    // Remove quotes if present
    if (*module_name_start == '\'' || *module_name_start == '"') {
      module_name_start++;
    }

    char module_name[256];
    strncpy(module_name, module_name_start, sizeof(module_name) - 1);
    module_name[sizeof(module_name) - 1] = '\0';

    // Remove trailing quote and parenthesis if present
    size_t len = strlen(module_name);
    if (len > 0) {
      if (module_name[len - 1] == ')') {
        module_name[len - 1] = '\0';
        len--;
      }
      if (len > 0 && (module_name[len - 1] == '\'' || module_name[len - 1] == '"')) {
        module_name[len - 1] = '\0';
      }
    }

    // Look up the module
    entry = symbol_table_lookup(symbol_table, module_name);
    if (entry && entry->symbol && entry->symbol->node->type == NODE_MODULE) {
      ast_node_add_reference(node, entry->node);
      return RESOLUTION_SUCCESS;
    }
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * Helper function to resolve JavaScript property references
 */
static ResolutionStatus resolve_javascript_property(ASTNode *node, const char *name,
                                                    GlobalSymbolTable *symbol_table) {
  // Handle object property access (obj.prop)
  const char *dot = strchr(name, '.');
  if (dot) {
    // Extract object name (part before the dot)
    size_t obj_len = dot - name;
    char *obj_name = (char *)malloc(obj_len + 1);
    if (!obj_name) {
      return RESOLUTION_ERROR;
    }

    strncpy(obj_name, name, obj_len);
    obj_name[obj_len] = '\0';

    // Look up the object
    SymbolEntry *obj_entry = symbol_table_lookup(symbol_table, obj_name);
    free(obj_name);

    if (obj_entry && obj_entry->symbol) {
      // Found the object, now look for the property
      const char *prop_name = dot + 1;

      // Build qualified name for lookup
      char qualified_name[256];
      snprintf(qualified_name, sizeof(qualified_name), "%s.%s", obj_entry->qualified_name,
               prop_name);

      SymbolEntry *entry = symbol_table_lookup(symbol_table, qualified_name);
      if (entry && entry->symbol) {
        ast_node_add_reference(node, entry->node);
        return RESOLUTION_SUCCESS;
      }
    }
  }

  return RESOLUTION_NOT_FOUND;
}

/**
 * TypeScript language reference resolver entrypoint (prototype only).
 * Implementation is in typescript_resolver.c.
 */
ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data);
