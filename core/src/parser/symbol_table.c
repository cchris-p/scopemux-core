/**
 * @file symbol_table.c
 * @brief Implementation of project-wide symbol management
 * 
 * This file serves as the main entry point for the symbol table infrastructure,
 * delegating to specialized components in the symbol_table/ directory:
 * 
 * - symbol_core.c: Core table management and lifecycle
 * - symbol_entry.c: Symbol entry creation and manipulation
 * - symbol_lookup.c: Symbol lookup and resolution
 * - symbol_registration.c: Symbol registration and hashing
 */

#include "scopemux/symbol_table.h"
#include "scopemux/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This file serves as the main entry point for the symbol table infrastructure.
// The actual implementations are in the symbol_table/ directory files.
// This approach allows for maintaining the public API while organizing the code
// into more manageable, specialized components.

// Helper macros for forward declarations
#define IMPORT_FUNCTION(returnType, name, ...) extern returnType symbol_table_##name##_impl(__VA_ARGS__)

// Import implementations from symbol_core.c
IMPORT_FUNCTION(GlobalSymbolTable*, create_impl, size_t initial_capacity);
IMPORT_FUNCTION(void, free_impl, GlobalSymbolTable *table);
IMPORT_FUNCTION(void, get_stats_impl, const GlobalSymbolTable *table, size_t *out_capacity,
              size_t *out_size, size_t *out_collisions);
IMPORT_FUNCTION(bool, should_rehash_impl, const GlobalSymbolTable *table);
IMPORT_FUNCTION(bool, rehash_impl, GlobalSymbolTable *table, size_t new_capacity);
IMPORT_FUNCTION(bool, add_scope_impl, GlobalSymbolTable *table, const char *scope_prefix);

// Import implementations from symbol_entry.c
IMPORT_FUNCTION(void, entry_free_impl, SymbolEntry *entry);
IMPORT_FUNCTION(SymbolEntry*, entry_create_impl, const char *qualified_name, ASTNode *node,
                        const char *file_path, SymbolScope scope, LanguageType language);

// Import implementations from symbol_lookup.c
IMPORT_FUNCTION(SymbolEntry*, lookup_impl, const GlobalSymbolTable *table, const char *qualified_name);
IMPORT_FUNCTION(SymbolEntry*, scope_lookup_impl, const GlobalSymbolTable *table, const char *name,
                          const char *current_scope, LanguageType language);
IMPORT_FUNCTION(size_t, get_by_type_impl, const GlobalSymbolTable *table, ASTNodeType type,
                    SymbolEntry **out_entries, size_t max_entries);
IMPORT_FUNCTION(size_t, get_by_file_impl, const GlobalSymbolTable *table, const char *file_path,
                    SymbolEntry **out_entries, size_t max_entries);

// Import implementations from symbol_registration.c
IMPORT_FUNCTION(SymbolEntry*, register_impl, GlobalSymbolTable *table, const char *qualified_name,
                      ASTNode *node, const char *file_path, SymbolScope scope, LanguageType language);
IMPORT_FUNCTION(size_t, register_from_ast_impl, GlobalSymbolTable *table, ASTNode *node,
                             const char *current_scope, const char *file_path, LanguageType language);

/**
 * Create a new global symbol table
 * 
 * Implemented in symbol_table/symbol_core.c
 */
GlobalSymbolTable *symbol_table_create(size_t initial_capacity) {
    return symbol_table_create_impl(initial_capacity);
}

/**
 * Free all resources associated with a symbol table
 * 
 * Implemented in symbol_table/symbol_core.c
 */
void symbol_table_free(GlobalSymbolTable *table) {
    symbol_table_free_impl(table);
}

/**
 * Register a symbol in the global table
 * 
 * Implemented in symbol_table/symbol_registration.c
 */
SymbolEntry *symbol_table_register(GlobalSymbolTable *table, const char *qualified_name,
                                 ASTNode *node, const char *file_path,
                                 SymbolScope scope, LanguageType language) {
    return symbol_table_register_impl(table, qualified_name, node, file_path, scope, language);
}

/**
 * Look up a symbol by its fully qualified name
 * 
 * Implemented in symbol_table/symbol_lookup.c
 */
SymbolEntry *symbol_table_lookup(const GlobalSymbolTable *table, const char *qualified_name) {
    return symbol_table_lookup_impl(table, qualified_name);
}

/**
 * Look up a symbol using scope-aware resolution
 * 
 * Implemented in symbol_table/symbol_lookup.c
 */
SymbolEntry *symbol_table_scope_lookup(const GlobalSymbolTable *table, const char *name,
                                    const char *current_scope, LanguageType language) {
    return symbol_table_scope_lookup_impl(table, name, current_scope, language);
}

/**
 * Add a scope prefix for resolution
 * 
 * Implemented in symbol_table/symbol_core.c
 */
bool symbol_table_add_scope(GlobalSymbolTable *table, const char *scope_prefix) {
    return symbol_table_add_scope_impl(table, scope_prefix);
}

/**
 * Get all symbols of a specific type
 * 
 * Implemented in symbol_table/symbol_lookup.c
 */
size_t symbol_table_get_by_type(const GlobalSymbolTable *table, ASTNodeType type,
                              SymbolEntry **out_entries, size_t max_entries) {
    return symbol_table_get_by_type_impl(table, type, out_entries, max_entries);
}

/**
 * Find all symbols in a specific file
 * 
 * Implemented in symbol_table/symbol_lookup.c
 */
size_t symbol_table_get_by_file(const GlobalSymbolTable *table, const char *file_path,
                              SymbolEntry **out_entries, size_t max_entries) {
    return symbol_table_get_by_file_impl(table, file_path, out_entries, max_entries);
}

/**
 * Get statistics about the symbol table
 * 
 * Implemented in symbol_table/symbol_core.c
 */
void symbol_table_get_stats(const GlobalSymbolTable *table, size_t *out_capacity,
                           size_t *out_size, size_t *out_collisions) {
    symbol_table_get_stats_impl(table, out_capacity, out_size, out_collisions);
}

/**
 * Analyze the symbol table for optimization opportunities
 * 
 * Implemented in symbol_table/symbol_core.c
 */
bool symbol_table_should_rehash(const GlobalSymbolTable *table) {
    return symbol_table_should_rehash_impl(table);
}

/**
 * Rehash the symbol table with a new capacity
 * 
 * Implemented in symbol_table/symbol_core.c
 */
bool symbol_table_rehash(GlobalSymbolTable *table, size_t new_capacity) {
    return symbol_table_rehash_impl(table, new_capacity);
}

//-----------------------------------------------------------------------------
// Note on Internal Helper Functions
//-----------------------------------------------------------------------------
// Internal helper functions have been moved to appropriate modular components:
// - hash_string          -> symbol_registration.c
// - extract_simple_name  -> symbol_entry.c
// - symbol_entry_create  -> symbol_entry.c
// - symbol_entry_free    -> symbol_entry.c
//
// These functions are now marked static in their respective implementation files
// to avoid symbol collision while maintaining the same functionality.
//-----------------------------------------------------------------------------

/**
 * Register all symbols from an AST into the table
 * 
 * Implemented in symbol_table/symbol_registration.c
 */
size_t symbol_table_register_from_ast(GlobalSymbolTable *table, ASTNode *node,
                                    const char *current_scope, const char *file_path,
                                    LanguageType language) {
    return symbol_table_register_from_ast_impl(table, node, current_scope, file_path, language);
}
