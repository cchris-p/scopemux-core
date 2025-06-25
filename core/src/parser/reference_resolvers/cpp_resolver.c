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
                                                 
// Forward declaration for C resolver to reuse functionality
ResolutionResult reference_resolver_c(ASTNode *node, ReferenceType ref_type,
                                   const char *name, GlobalSymbolTable *symbol_table,
                                   void *resolver_data);

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
ResolutionResult reference_resolver_cpp(ASTNode *node, ReferenceType ref_type,
                                     const char *name, GlobalSymbolTable *symbol_table,
                                     void *resolver_data) {
    if (!node || !name || !symbol_table) {
        return RESOLUTION_FAILED;
    }
    
    // Track statistics
    cpp_resolver_stats.total_lookups++;
    
    // Handle special cases based on reference type
    switch (ref_type) {
        case REFERENCE_INCLUDE:
            // Similar to C, delegate to C resolver for include handling
            return reference_resolver_c(node, ref_type, name, symbol_table, NULL);
            
        case REFERENCE_NAMESPACE:
            // Handle C++ namespace resolution
            cpp_resolver_stats.namespace_resolved++;
            
            // Look up the namespace in the symbol table
            SymbolEntry *namespace_entry = symbol_table_lookup(symbol_table, name);
            if (namespace_entry && namespace_entry->node->type == NODE_NAMESPACE) {
                // Add reference to the namespace
                if (node->num_references < node->references_capacity) {
                    node->references[node->num_references++] = namespace_entry->node;
                    cpp_resolver_stats.resolved_count++;
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
                        node->references[node->num_references++] = namespace_entry->node;
                        cpp_resolver_stats.resolved_count++;
                        return RESOLUTION_SUCCESS;
                    }
                }
            }
            return RESOLUTION_NOT_FOUND;
            
        case REFERENCE_TEMPLATE:
            // Handle template specialization
            cpp_resolver_stats.template_resolved++;
            
            // Template resolution is complex and would require parsing the template parameters
            // TODO: Implement template resolution logic
            // For now, just try to find the template name without parameters
            {
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
                            // Add reference to the template
                            if (node->num_references < node->references_capacity) {
                                node->references[node->num_references++] = template_entry->node;
                                cpp_resolver_stats.resolved_count++;
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
                                    node->references[node->num_references++] = template_entry->node;
                                    cpp_resolver_stats.resolved_count++;
                                    return RESOLUTION_SUCCESS;
                                }
                            }
                        }
                    }
                }
            }
            break;
            
        case REFERENCE_CLASS:
            // Handle class/struct references
            cpp_resolver_stats.class_resolved++;
            
            // Look up the class in the symbol table
            // This needs to handle namespaces (e.g., std::vector)
            {
                SymbolEntry *class_entry = symbol_table_lookup(symbol_table, name);
                if (class_entry && 
                   (class_entry->node->type == NODE_CLASS || 
                    class_entry->node->type == NODE_STRUCT)) {
                    
                    // Add reference to the class
                    if (node->num_references < node->references_capacity) {
                        node->references[node->num_references++] = class_entry->node;
                        cpp_resolver_stats.resolved_count++;
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
                            node->references[node->num_references++] = class_entry->node;
                            cpp_resolver_stats.resolved_count++;
                            return RESOLUTION_SUCCESS;
                        }
                    }
                }
                
                // Handle namespaced classes (Namespace::Class)
                char *double_colon = strstr(name, "::");
                if (double_colon) {
                    size_t namespace_len = double_colon - name;
                    char namespace[256]; // Reasonable limit
                    
                    if (namespace_len < sizeof(namespace)) {
                        strncpy(namespace, name, namespace_len);
                        namespace[namespace_len] = '\0';
                        
                        // Look up the namespace
                        SymbolEntry *ns_entry = symbol_table_lookup(symbol_table, namespace);
                        if (ns_entry && ns_entry->node->type == NODE_NAMESPACE) {
                            // Now look up the fully qualified class
                            SymbolEntry *class_entry = symbol_table_lookup(symbol_table, name);
                            if (class_entry) {
                                // Add reference to the class
                                if (node->num_references < node->references_capacity) {
                                    node->references[node->num_references++] = class_entry->node;
                                    cpp_resolver_stats.resolved_count++;
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
                                        node->references[node->num_references++] = class_entry->node;
                                        cpp_resolver_stats.resolved_count++;
                                        return RESOLUTION_SUCCESS;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
            
        case REFERENCE_METHOD:
            // Handle method resolution, including overloaded methods
            cpp_resolver_stats.method_resolved++;
            
            // Method resolution in C++ is complex due to overloading
            // For now, we'll just do a simple lookup without considering overloads
            {
                SymbolEntry *method_entry = symbol_table_lookup(symbol_table, name);
                if (method_entry && method_entry->node->type == NODE_METHOD) {
                    // Add reference to the method
                    if (node->num_references < node->references_capacity) {
                        node->references[node->num_references++] = method_entry->node;
                        cpp_resolver_stats.resolved_count++;
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
                            node->references[node->num_references++] = method_entry->node;
                            cpp_resolver_stats.resolved_count++;
                            return RESOLUTION_SUCCESS;
                        }
                    }
                }
                
                // Handle class methods (Class::method)
                char *double_colon = strstr(name, "::");
                if (double_colon) {
                    size_t class_name_len = double_colon - name;
                    char class_name[256]; // Reasonable limit
                    
                    if (class_name_len < sizeof(class_name)) {
                        strncpy(class_name, name, class_name_len);
                        class_name[class_name_len] = '\0';
                        
                        // Look up the class
                        SymbolEntry *class_entry = symbol_table_lookup(symbol_table, class_name);
                        if (class_entry) {
                            // Now look up the fully qualified method
                            SymbolEntry *method_entry = symbol_table_lookup(symbol_table, name);
                            if (method_entry) {
                                // Add reference to the method
                                if (node->num_references < node->references_capacity) {
                                    node->references[node->num_references++] = method_entry->node;
                                    cpp_resolver_stats.resolved_count++;
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
                                        node->references[node->num_references++] = method_entry->node;
                                        cpp_resolver_stats.resolved_count++;
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
            // For standard symbol resolution, try C++ specific rules first,
            // then fall back to C resolver for common functionality
            break;
    }
    
    // C++ scope resolution with namespaces:
    // 1. Try exact name match first
    SymbolEntry *entry = symbol_table_lookup(symbol_table, name);
    if (entry) {
        cpp_resolver_stats.resolved_count++;
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
    
    // 2. Try in current scope chain
    char *current_scope = NULL;
    if (node->parent && node->parent->qualified_name) {
        current_scope = node->parent->qualified_name;
    }
    entry = symbol_table_scope_lookup(symbol_table, name, current_scope, LANG_CPP);
    
    if (entry) {
        cpp_resolver_stats.resolved_count++;
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
    
    // 3. Check for the symbol in the global namespace (::symbol)
    char global_name[256];
    snprintf(global_name, sizeof(global_name), "::%s", name);
    entry = symbol_table_lookup(symbol_table, global_name);
    
    if (entry) {
        cpp_resolver_stats.resolved_count++;
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
    
    // 4. If not found so far, fall back to C resolver
    // Many C constructs are valid in C++ as well
    ResolutionResult result = reference_resolver_c(node, ref_type, name, symbol_table, NULL);
    
    if (result == RESOLUTION_SUCCESS) {
        cpp_resolver_stats.resolved_count++;
    }
    
    return result;
}

/**
 * Get C++ resolver statistics
 */
void cpp_resolver_get_stats(size_t *total, size_t *resolved,
                          size_t *namespace_resolved, size_t *template_resolved,
                          size_t *method_resolved, size_t *class_resolved) {
    if (total) *total = cpp_resolver_stats.total_lookups;
    if (resolved) *resolved = cpp_resolver_stats.resolved_count;
    if (namespace_resolved) *namespace_resolved = cpp_resolver_stats.namespace_resolved;
    if (template_resolved) *template_resolved = cpp_resolver_stats.template_resolved;
    if (method_resolved) *method_resolved = cpp_resolver_stats.method_resolved;
    if (class_resolved) *class_resolved = cpp_resolver_stats.class_resolved;
}

/**
 * Reset C++ resolver statistics
 */
void cpp_resolver_reset_stats() {
    memset(&cpp_resolver_stats, 0, sizeof(cpp_resolver_stats));
}
