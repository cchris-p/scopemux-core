/**
 * @file symbol_table_internal.h
 * @brief Internal coordination header for symbol table implementation modules
 * 
 * This header provides internal function declarations for the symbol table
 * implementation modules to coordinate between specialized components.
 * It is not part of the public API and should only be included by
 * symbol table implementation files.
 */

#ifndef SYMBOL_TABLE_INTERNAL_H
#define SYMBOL_TABLE_INTERNAL_H

#include "scopemux/symbol_table.h"
#include <stdbool.h>
#include <stddef.h>

// Forward declarations from symbol_core.c
GlobalSymbolTable *symbol_table_create_impl(size_t initial_capacity);
void symbol_table_free_impl(GlobalSymbolTable *table);
void symbol_table_get_stats_impl(const GlobalSymbolTable *table, size_t *out_capacity,
                                size_t *out_size, size_t *out_collisions);
bool symbol_table_should_rehash_impl(const GlobalSymbolTable *table);
bool symbol_table_rehash_impl(GlobalSymbolTable *table, size_t new_capacity);
bool symbol_table_add_scope_impl(GlobalSymbolTable *table, const char *scope_prefix);

// Forward declarations from symbol_entry.c
SymbolEntry *symbol_table_entry_create_impl(const char *qualified_name, ASTNode *node,
                                          const char *file_path, SymbolScope scope, Language language);
void symbol_table_entry_free_impl(SymbolEntry *entry);
char *extract_simple_name(const char *qualified_name);

// Forward declarations from symbol_lookup.c
SymbolEntry *symbol_table_lookup_impl(const GlobalSymbolTable *table, const char *qualified_name);
SymbolEntry *symbol_table_scope_lookup_impl(const GlobalSymbolTable *table, const char *name,
                                          const char *current_scope, Language language);
size_t symbol_table_get_by_type_impl(const GlobalSymbolTable *table, ASTNodeType type,
                                   SymbolEntry **out_entries, size_t max_entries);
size_t symbol_table_get_by_file_impl(const GlobalSymbolTable *table, const char *file_path,
                                   SymbolEntry **out_entries, size_t max_entries);

// Forward declarations from symbol_registration.c
SymbolEntry *symbol_table_register_impl(GlobalSymbolTable *table, const char *qualified_name,
                                      ASTNode *node, const char *file_path, SymbolScope scope, Language language);
size_t symbol_table_register_from_ast_impl(GlobalSymbolTable *table, ASTNode *node,
                                         const char *current_scope, const char *file_path, Language language);
bool symbol_table_add_impl(GlobalSymbolTable *table, SymbolEntry *entry);

#endif /* SYMBOL_TABLE_INTERNAL_H */
