#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include "scopemux/parser.h"
#include "scopemux/logging.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declaration of the generic resolver
ResolutionResult reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                 const char *name, GlobalSymbolTable *symbol_table);

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
ResolutionResult reference_resolver_python(ASTNode *node, ReferenceType ref_type,
                                        const char *name, GlobalSymbolTable *symbol_table,
                                        void *resolver_data) {
    if (!node || !name || !symbol_table) {
        return RESOLUTION_FAILED;
    }
    
    // Track statistics
    python_resolver_stats.total_lookups++;
    
    // Handle special cases based on reference type
    switch (ref_type) {
        case REFERENCE_IMPORT:
            // Handle module import resolution
            // This would require access to the ProjectContext
            python_resolver_stats.import_resolved++;
            
            // Try to find the module in the symbol table
            SymbolEntry *module_entry = symbol_table_lookup(symbol_table, name);
            if (module_entry && 
                (module_entry->node->type == NODE_MODULE || 
                 module_entry->node->type == NODE_PACKAGE)) {
                
                // Add reference to the module
                if (node->num_references < node->references_capacity) {
                    node->references[node->num_references++] = module_entry->node;
                    return RESOLUTION_SUCCESS;
                } else {
                    // Resize references array
                    size_t new_capacity = node->references_capacity * 2;
                    if (new_capacity == 0) new_capacity = 4;
                    
                    ASTNode **new_refs = (ASTNode **)realloc(
                        node->references, new_capacity * sizeof(ASTNode *));
                        
                    if (new_refs) {
                        node->references = new_refs;
                        node->references_capacity = new_capacity;
                        node->references[node->num_references++] = module_entry->node;
                        return RESOLUTION_SUCCESS;
                    }
                }
            }
            return RESOLUTION_NOT_FOUND;
            
        case REFERENCE_ATTRIBUTE:
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
                                if (node->num_references < node->references_capacity) {
                                    node->references[node->num_references++] = attr_entry->node;
                                    python_resolver_stats.resolved_count++;
                                    return RESOLUTION_SUCCESS;
                                } else {
                                    // Resize references array
                                    size_t new_capacity = node->references_capacity * 2;
                                    if (new_capacity == 0) new_capacity = 4;
                                    
                                    ASTNode **new_refs = (ASTNode **)realloc(
                                        node->references, new_capacity * sizeof(ASTNode *));
                                        
                                    if (new_refs) {
                                        node->references = new_refs;
                                        node->references_capacity = new_capacity;
                                        node->references[node->num_references++] = attr_entry->node;
                                        python_resolver_stats.resolved_count++;
                                        return RESOLUTION_SUCCESS;
                                    }
                                }
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
        if (node->num_references < node->references_capacity) {
            node->references[node->num_references++] = entry->node;
            return RESOLUTION_SUCCESS;
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
                return RESOLUTION_SUCCESS;
            }
        }
        return RESOLUTION_FAILED; // Failed to add reference
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
        if (node->num_references < node->references_capacity) {
            node->references[node->num_references++] = entry->node;
            return RESOLUTION_SUCCESS;
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
                return RESOLUTION_SUCCESS;
            }
        }
        return RESOLUTION_FAILED; // Failed to add reference
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
            if (node->num_references < node->references_capacity) {
                node->references[node->num_references++] = entry->node;
                return RESOLUTION_SUCCESS;
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
                    return RESOLUTION_SUCCESS;
                }
            }
            return RESOLUTION_FAILED; // Failed to add reference
        }
    }
    
    // If we got this far, fallback to generic resolution
    ResolutionResult result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
    
    if (result == RESOLUTION_SUCCESS) {
        python_resolver_stats.resolved_count++;
    }
    
    return result;
}

/**
 * Get Python resolver statistics
 */
void python_resolver_get_stats(size_t *total, size_t *resolved, 
                            size_t *import_resolved, size_t *attribute_resolved,
                            size_t *builtin_resolved) {
    if (total) *total = python_resolver_stats.total_lookups;
    if (resolved) *resolved = python_resolver_stats.resolved_count;
    if (import_resolved) *import_resolved = python_resolver_stats.import_resolved;
    if (attribute_resolved) *attribute_resolved = python_resolver_stats.attribute_resolved;
    if (builtin_resolved) *builtin_resolved = python_resolver_stats.builtin_resolved;
}

/**
 * Reset Python resolver statistics
 */
void python_resolver_reset_stats() {
    memset(&python_resolver_stats, 0, sizeof(python_resolver_stats));
}
