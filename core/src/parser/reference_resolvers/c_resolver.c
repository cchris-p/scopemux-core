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
 * Statistics specific to C language resolution
 */
typedef struct {
    size_t total_lookups;
    size_t resolved_count;
    size_t macro_resolved;
    size_t header_resolved;
    size_t struct_fields_resolved;
} CResolverStats;

static CResolverStats c_resolver_stats = {0};

/**
 * C language resolver implementation
 * 
 * Handles C-specific reference resolution, including header inclusion,
 * macro expansion, and C symbol lookup rules
 */
ResolutionResult reference_resolver_c(ASTNode *node, ReferenceType ref_type,
                                   const char *name, GlobalSymbolTable *symbol_table,
                                   void *resolver_data) {
    if (!node || !name || !symbol_table) {
        return RESOLUTION_FAILED;
    }
    
    // Track statistics
    c_resolver_stats.total_lookups++;
    
    // Handle special cases based on reference type
    switch (ref_type) {
        case REFERENCE_INCLUDE:
            // Handle header file inclusion
            // For header files, we need to look in standard include paths and project include paths
            // This would require access to the ProjectContext
            // TODO: Implement header resolution when ProjectContext is available via resolver_data
            c_resolver_stats.header_resolved++;
            return RESOLUTION_SUCCESS;
            
        case REFERENCE_MACRO:
            // Handle macro resolution
            // Currently not implemented in the symbol table
            // TODO: Implement macro tracking in the symbol table
            c_resolver_stats.macro_resolved++;
            return RESOLUTION_NOT_SUPPORTED;
            
        case REFERENCE_STRUCT_MEMBER:
            // Handle struct/union field access
            // Parse out struct name from reference
            {
                char struct_name[256] = {0};
                char field_name[256] = {0};
                
                // Handle struct field access (struct->field or struct.field)
                const char *arrow = strstr(name, "->");
                const char *dot = strstr(name, ".");
                
                if (arrow || dot) {
                    const char *separator = arrow ? arrow : dot;
                    size_t struct_part_len = separator - name;
                    
                    // Extract struct name and field name
                    if (struct_part_len < sizeof(struct_name)) {
                        strncpy(struct_name, name, struct_part_len);
                        struct_name[struct_part_len] = '\0';
                        
                        // Field name starts after separator
                        size_t field_start = separator - name + (arrow ? 2 : 1);
                        if (field_start < strlen(name)) {
                            strcpy(field_name, name + field_start);
                            
                            // Look up struct type
                            SymbolEntry *struct_entry = symbol_table_lookup(symbol_table, struct_name);
                            if (struct_entry && struct_entry->node) {
                                // Now look for the field in the struct's children
                                ASTNode *struct_node = struct_entry->node;
                                
                                for (size_t i = 0; i < struct_node->num_children; i++) {
                                    ASTNode *field = struct_node->children[i];
                                    
                                    if (field && field->name && 
                                        strcmp(field->name, field_name) == 0) {
                                        // Found the field, add reference
                                        if (node->num_references < node->references_capacity) {
                                            node->references[node->num_references++] = field;
                                            c_resolver_stats.struct_fields_resolved++;
                                            c_resolver_stats.resolved_count++;
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
                                                node->references[node->num_references++] = field;
                                                c_resolver_stats.struct_fields_resolved++;
                                                c_resolver_stats.resolved_count++;
                                                return RESOLUTION_SUCCESS;
                                            }
                                        }
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
    
    // C-specific scope resolution:
    // 1. Try exact name match first (handles local variables)
    SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
    if (entry) {
        c_resolver_stats.resolved_count++;
        // Add reference to node
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
    
    // 2. Try current scope lookup
    char *current_scope = NULL;
    if (node->parent && node->parent->qualified_name) {
        current_scope = node->parent->qualified_name;
    }
    entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_C);
    
    if (entry) {
        c_resolver_stats.resolved_count++;
        // Add reference to node
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
    
    // If we got this far, fallback to generic resolution
    ResolutionResult result = reference_resolver_generic_resolve(node, ref_type, name, symbol_table);
    
    if (result == RESOLUTION_SUCCESS) {
        c_resolver_stats.resolved_count++;
    }
    
    return result;
}

/**
 * Get C resolver statistics
 */
void c_resolver_get_stats(size_t *total, size_t *resolved, 
                        size_t *macro_resolved, size_t *header_resolved,
                        size_t *struct_fields_resolved) {
    if (total) *total = c_resolver_stats.total_lookups;
    if (resolved) *resolved = c_resolver_stats.resolved_count;
    if (macro_resolved) *macro_resolved = c_resolver_stats.macro_resolved;
    if (header_resolved) *header_resolved = c_resolver_stats.header_resolved;
    if (struct_fields_resolved) *struct_fields_resolved = c_resolver_stats.struct_fields_resolved;
}

/**
 * Reset C resolver statistics
 */
void c_resolver_reset_stats() {
    memset(&c_resolver_stats, 0, sizeof(c_resolver_stats));
}
