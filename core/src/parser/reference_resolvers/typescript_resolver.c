#include "scopemux/reference_resolver.h"
#include "scopemux/symbol_table.h"
#include "scopemux/parser.h"
#include "scopemux/logging.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations
ResolutionResult reference_resolver_generic_resolve(ASTNode *node, ReferenceType ref_type,
                                                 const char *name, GlobalSymbolTable *symbol_table);

// Forward declaration for JavaScript resolver to reuse functionality
ResolutionResult reference_resolver_javascript(ASTNode *node, ReferenceType ref_type,
                                            const char *name, GlobalSymbolTable *symbol_table,
                                            void *resolver_data);

/**
 * Statistics specific to TypeScript language resolution
 */
typedef struct {
    size_t total_lookups;
    size_t resolved_count;
    size_t import_resolved;
    size_t property_resolved;
    size_t type_resolved;
    size_t interface_resolved;
    size_t generic_resolved;
} TypeScriptResolverStats;

static TypeScriptResolverStats ts_resolver_stats = {0};

/**
 * TypeScript language resolver implementation
 *
 * Handles TypeScript-specific reference resolution, including type interfaces,
 * generics, and extends JavaScript resolver for runtime features.
 */
ResolutionResult reference_resolver_typescript(ASTNode *node, ReferenceType ref_type,
                                            const char *name, GlobalSymbolTable *symbol_table,
                                            void *resolver_data) {
    if (!node || !name || !symbol_table) {
        return RESOLUTION_FAILED;
    }
    
    // Track statistics
    ts_resolver_stats.total_lookups++;
    
    // TypeScript has special reference types
    switch (ref_type) {
        case REFERENCE_TYPE:
            // Handle TypeScript type references (e.g., : MyType)
            ts_resolver_stats.type_resolved++;
            
            // Look up the type in the symbol table
            SymbolEntry *type_entry = symbol_table_lookup(symbol_table, name);
            if (type_entry && 
                (type_entry->node->type == NODE_TYPE || 
                 type_entry->node->type == NODE_INTERFACE ||
                 type_entry->node->type == NODE_CLASS)) {
                
                // Add reference to the type
                if (node->num_references < node->references_capacity) {
                    node->references[node->num_references++] = type_entry->node;
                    ts_resolver_stats.resolved_count++;
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
                        node->references[node->num_references++] = type_entry->node;
                        ts_resolver_stats.resolved_count++;
                        return RESOLUTION_SUCCESS;
                    }
                }
            }
            
            // Check namespaced types (Namespace.Type)
            {
                char *dot = strchr(name, '.');
                if (dot) {
                    size_t namespace_len = dot - name;
                    char namespace[256]; // Reasonable limit
                    
                    if (namespace_len < sizeof(namespace)) {
                        strncpy(namespace, name, namespace_len);
                        namespace[namespace_len] = '\0';
                        
                        // Look up the namespace
                        SymbolEntry *ns_entry = symbol_table_lookup(symbol_table, namespace);
                        if (ns_entry) {
                            // Now look up the fully qualified type
                            SymbolEntry *type_entry = symbol_table_lookup(symbol_table, name);
                            if (type_entry) {
                                // Add reference to the type
                                if (node->num_references < node->references_capacity) {
                                    node->references[node->num_references++] = type_entry->node;
                                    ts_resolver_stats.resolved_count++;
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
                                        node->references[node->num_references++] = type_entry->node;
                                        ts_resolver_stats.resolved_count++;
                                        return RESOLUTION_SUCCESS;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            return RESOLUTION_NOT_FOUND;
            
        case REFERENCE_INTERFACE:
            // Handle TypeScript interface references
            ts_resolver_stats.interface_resolved++;
            
            // Look up the interface in the symbol table
            SymbolEntry *interface_entry = symbol_table_lookup(symbol_table, name);
            if (interface_entry && interface_entry->node->type == NODE_INTERFACE) {
                // Add reference to the interface
                if (node->num_references < node->references_capacity) {
                    node->references[node->num_references++] = interface_entry->node;
                    ts_resolver_stats.resolved_count++;
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
                        node->references[node->num_references++] = interface_entry->node;
                        ts_resolver_stats.resolved_count++;
                        return RESOLUTION_SUCCESS;
                    }
                }
            }
            return RESOLUTION_NOT_FOUND;
            
        case REFERENCE_GENERIC:
            // Handle generic type parameters
            ts_resolver_stats.generic_resolved++;
            
            // In TypeScript, generics are usually scoped to their containing declaration
            // We need to check if it's a generic parameter of the current function/class/interface
            if (node->parent) {
                ASTNode *parent = node->parent;
                // Check parent's generic parameters
                // TODO: This would require extending ASTNode to track generic parameters
                // For now, just return not supported
            }
            
            return RESOLUTION_NOT_SUPPORTED;
            
        case REFERENCE_IMPORT:
            // Handle ES module imports with TypeScript extensions
            ts_resolver_stats.import_resolved++;
            
            // TypeScript adds support for type-only imports
            // But at the symbol table level, they're treated similarly
            SymbolEntry *module_entry = symbol_table_lookup(symbol_table, name);
            if (module_entry && module_entry->node->type == NODE_MODULE) {
                // Add reference to the module
                if (node->num_references < node->references_capacity) {
                    node->references[node->num_references++] = module_entry->node;
                    ts_resolver_stats.resolved_count++;
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
                        ts_resolver_stats.resolved_count++;
                        return RESOLUTION_SUCCESS;
                    }
                }
            }
            break;
            
        case REFERENCE_PROPERTY:
            // Handle property access (obj.prop) with type information
            ts_resolver_stats.property_resolved++;
            
            // Type-aware property resolution would need type information
            // For now, delegate to JavaScript resolver
            break;
            
        default:
            // For standard reference types, delegate to JavaScript resolver
            break;
    }
    
    // For most runtime constructs, TypeScript behaves like JavaScript
    // Delegate to JavaScript resolver for those cases
    ResolutionResult result = reference_resolver_javascript(node, ref_type, name, symbol_table, NULL);
    
    if (result == RESOLUTION_SUCCESS) {
        ts_resolver_stats.resolved_count++;
    }
    
    return result;
}

/**
 * Get TypeScript resolver statistics
 */
void typescript_resolver_get_stats(size_t *total, size_t *resolved, 
                                 size_t *import_resolved, size_t *property_resolved,
                                 size_t *type_resolved, size_t *interface_resolved,
                                 size_t *generic_resolved) {
    if (total) *total = ts_resolver_stats.total_lookups;
    if (resolved) *resolved = ts_resolver_stats.resolved_count;
    if (import_resolved) *import_resolved = ts_resolver_stats.import_resolved;
    if (property_resolved) *property_resolved = ts_resolver_stats.property_resolved;
    if (type_resolved) *type_resolved = ts_resolver_stats.type_resolved;
    if (interface_resolved) *interface_resolved = ts_resolver_stats.interface_resolved;
    if (generic_resolved) *generic_resolved = ts_resolver_stats.generic_resolved;
}

/**
 * Reset TypeScript resolver statistics
 */
void typescript_resolver_reset_stats() {
    memset(&ts_resolver_stats, 0, sizeof(ts_resolver_stats));
}
