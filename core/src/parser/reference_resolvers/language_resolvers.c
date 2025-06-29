/**
 * @file language_resolvers.c
 * @brief Implementation of language-specific resolvers
 * 
 * This file contains implementations of resolvers for specific languages:
 * - C/C++ resolver
 * - Python resolver
 * - JavaScript resolver
 * - TypeScript resolver
 * 
 * Future language support can be added here or in separate files.
 */

#include "scopemux/reference_resolver.h"
#include "scopemux/logging.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of generic resolver for fallback
extern ResolutionStatus reference_resolver_generic_resolve_impl(
    ASTNode *node, ReferenceType ref_type, const char *name, GlobalSymbolTable *symbol_table);

/**
 * C language resolver implementation
 * 
 * Handles C-specific reference resolution, including header inclusion,
 * macro expansion, and C symbol lookup rules
 */
ResolutionStatus reference_resolver_c_impl(ASTNode *node, ReferenceType ref_type,
                                      const char *name, GlobalSymbolTable *symbol_table,
                                      void *resolver_data) {
    if (!node || !name || !symbol_table) {
        return RESOLVE_ERROR;
    }
    
    // Track C-specific resolution statistics if resolver_data is provided
    typedef struct {
        size_t total_lookups;
        size_t resolved_count;
        size_t macro_resolved;
    } CResolverStats;
    
    CResolverStats *stats = (CResolverStats *)resolver_data;
    if (stats) stats->total_lookups++;
    
    // 1. Try direct lookup first
    SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
    if (entry) {
        if (stats) stats->resolved_count++;
        // Add reference to node
        if (node->num_references < node->references_capacity || 
            ast_node_add_reference(node, entry->node)) {
            return RESOLVE_SUCCESS;
        }
        return RESOLVE_ERROR; // Failed to add reference
    }
    
    // 2. Try current scope lookup
    char *current_scope = NULL;
    if (node->parent && node->parent->qualified_name) {
        current_scope = node->parent->qualified_name;
    }
    entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_C);
    
    if (entry) {
        if (stats) stats->resolved_count++;
        // Add reference to node
        if (node->num_references < node->references_capacity || 
            ast_node_add_reference(node, entry->node)) {
            return RESOLVE_SUCCESS;
        }
        return RESOLVE_ERROR; // Failed to add reference
    }
    
    // If we got this far, fallback to generic resolution
    ResolutionStatus result = reference_resolver_generic_resolve_impl(node, ref_type, name, symbol_table);
    
    if (result == RESOLVE_SUCCESS && stats) {
        stats->resolved_count++;
    }
    
    return result;
}

/**
 * Python language resolver implementation
 * 
 * Handles Python-specific reference resolution, including module imports,
 * dot notation for attributes, and Python's scope resolution rules
 */
ResolutionStatus reference_resolver_python_impl(ASTNode *node, ReferenceType ref_type,
                                           const char *name, GlobalSymbolTable *symbol_table,
                                           void *resolver_data) {
    if (!node || !name || !symbol_table) {
        return RESOLVE_ERROR;
    }
    
    // Track Python-specific resolution statistics if resolver_data is provided
    typedef struct {
        size_t total_lookups;
        size_t resolved_count;
        size_t import_resolved;
        size_t attribute_resolved;
    } PythonResolverStats;
    
    PythonResolverStats *stats = (PythonResolverStats *)resolver_data;
    if (stats) stats->total_lookups++;
    
    // 1. Try direct lookup first (handles fully qualified names)
    SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
    if (entry) {
        if (stats) stats->resolved_count++;
        // Add reference to node
        if (node->num_references < node->references_capacity) {
            node->references[node->num_references++] = entry->node;
            return RESOLVE_SUCCESS;
        } else {
            // Resize references array
            size_t new_capacity = node->references_capacity * 2;
            if (new_capacity == 0) new_capacity = 4;
            
            ASTNode **new_refs = (ASTNode **)realloc(
                node->references, new_capacity * sizeof(ASTNode *));
                
            if (new_refs) {
                node->references = new_refs;
                node->references_capacity = new_capacity;
                node->references[node->num_references++] = entry->node;
                return RESOLVE_SUCCESS;
            }
        }
        return RESOLVE_ERROR; // Failed to add reference
    }
    
    // 2. Try scope-aware lookup (handles relative references within a module)
    char *current_scope = NULL;
    if (node->parent && node->parent->qualified_name) {
        current_scope = node->parent->qualified_name;
    }
    
    entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_PYTHON);
    if (entry) {
        if (stats) stats->resolved_count++;
        if (stats) stats->attribute_resolved++;
        
        // Add reference
        if (node->num_references < node->references_capacity) {
            node->references[node->num_references++] = entry->node;
            return RESOLVE_SUCCESS;
        } else {
            // Resize references array
            size_t new_capacity = node->references_capacity * 2;
            if (new_capacity == 0) new_capacity = 4;
            
            ASTNode **new_refs = (ASTNode **)realloc(
                node->references, new_capacity * sizeof(ASTNode *));
                
            if (new_refs) {
                node->references = new_refs;
                node->references_capacity = new_capacity;
                node->references[node->num_references++] = entry->node;
                return RESOLVE_SUCCESS;
            }
        }
        return RESOLVE_ERROR; // Failed to add reference
    }
    
    // 3. Try resolving as module attribute (for 'from X import Y' style references)
    // Note: module_path would need to be added to ASTNode or stored in additional_data
    if (node->parent && node->parent->type == NODE_IMPORT) {
        // For now, skip module attribute resolution until ASTNode is extended
        // This will be implemented when we add module_path field or use additional_data
        
        // TODO: Implement module attribute resolution when module_path is available
    }
    
    // 4. Look in builtins (Python has a builtin scope)
    entry = symbol_table_lookup(symbol_table, "builtins");
    if (entry && entry->node) {
        char builtin_name[256];
        snprintf(builtin_name, sizeof(builtin_name), "builtins.%s", name);
        entry = symbol_table_lookup(symbol_table, builtin_name);
        if (entry) {
            if (stats) stats->resolved_count++;
            // Add reference
            if (node->num_references < node->references_capacity) {
                node->references[node->num_references++] = entry->node;
                return RESOLVE_SUCCESS;
            } else {
                // Resize references array
                size_t new_capacity = node->references_capacity * 2;
                if (new_capacity == 0) new_capacity = 4;
                
                ASTNode **new_refs = (ASTNode **)realloc(
                    node->references, new_capacity * sizeof(ASTNode *));
                    
                if (new_refs) {
                    node->references = new_refs;
                    node->references_capacity = new_capacity;
                    node->references[node->num_references++] = entry->node;
                    return RESOLVE_SUCCESS;
                }
            }
            return RESOLVE_ERROR; // Failed to add reference
        }
    }
    
    // If we got this far, fallback to generic resolution
    ResolutionStatus result = reference_resolver_generic_resolve_impl(node, ref_type, name, symbol_table);
    
    if (result == RESOLVE_SUCCESS && stats) {
        stats->resolved_count++;
    }
    
    return result;
}

/**
 * JavaScript language resolver implementation
 * 
 * Handles JavaScript-specific reference resolution, including module imports,
 * CommonJS requires, and JavaScript's scope resolution rules
 */
ResolutionStatus reference_resolver_javascript_impl(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data) {
    // For now, fall back to the generic resolver
    // This will be enhanced with JavaScript-specific resolution logic in the future
    return reference_resolver_generic_resolve_impl(node, ref_type, name, symbol_table);
}

/**
 * TypeScript language resolver implementation
 * 
 * Handles TypeScript-specific reference resolution, including module imports,
 * type references, and TypeScript's scope resolution rules
 */
ResolutionStatus reference_resolver_typescript_impl(ASTNode *node, ReferenceType ref_type,
                                               const char *name, GlobalSymbolTable *symbol_table,
                                               void *resolver_data) {
    // For now, fall back to the generic resolver
    // This will be enhanced with TypeScript-specific resolution logic in the future
    return reference_resolver_generic_resolve_impl(node, ref_type, name, symbol_table);
}

// Export implementations as public symbols for use in delegation layer
ResolutionStatus reference_resolver_c(ASTNode *node, ReferenceType ref_type,
                                   const char *name, GlobalSymbolTable *symbol_table,
                                   void *resolver_data) {
    return reference_resolver_c_impl(node, ref_type, name, symbol_table, resolver_data);
}

ResolutionStatus reference_resolver_python(ASTNode *node, ReferenceType ref_type,
                                        const char *name, GlobalSymbolTable *symbol_table,
                                        void *resolver_data) {
    return reference_resolver_python_impl(node, ref_type, name, symbol_table, resolver_data);
}

ResolutionStatus reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                            const char *name, GlobalSymbolTable *symbol_table,
                                            void *resolver_data) {
    return reference_resolver_javascript_impl(node, ref_type, name, symbol_table, resolver_data);
}

ResolutionStatus reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                            const char *name, GlobalSymbolTable *symbol_table,
                                            void *resolver_data) {
    return reference_resolver_typescript_impl(node, ref_type, name, symbol_table, resolver_data);
}

// Test-specific function aliases that tests expect
ResolutionStatus c_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                GlobalSymbolTable *symbol_table, void *resolver_data) {
    return reference_resolver_c_impl(node, ref_type, name, symbol_table, resolver_data);
}

ResolutionStatus python_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                     GlobalSymbolTable *symbol_table, void *resolver_data) {
    return reference_resolver_python_impl(node, ref_type, name, symbol_table, resolver_data);
}

ResolutionStatus javascript_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                         GlobalSymbolTable *symbol_table, void *resolver_data) {
    return reference_resolver_javascript_impl(node, ref_type, name, symbol_table, resolver_data);
}

ResolutionStatus typescript_resolver_impl(ASTNode *node, ReferenceType ref_type, const char *name,
                                         GlobalSymbolTable *symbol_table, void *resolver_data) {
    return reference_resolver_typescript_impl(node, ref_type, name, symbol_table, resolver_data);
}
