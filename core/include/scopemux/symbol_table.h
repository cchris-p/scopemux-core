/**
 * @file symbol_table.h
 * @brief Project-wide symbol management for inter-file relationship resolution
 *
 * This module provides a global symbol table for tracking and resolving symbols
 * across multiple source files. It serves as the foundation for establishing
 * inter-file relationships such as cross-file function calls, imports, and
 * inheritance hierarchies.
 *
 * The symbol table supports efficient lookup by qualified name and scope-aware
 * resolution of partial names based on the current context.
 */

#ifndef SCOPEMUX_SYMBOL_TABLE_H
#define SCOPEMUX_SYMBOL_TABLE_H

#include "parser.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Symbol visibility scope
 */
typedef enum {
  SCOPE_UNKNOWN = 0,
  SCOPE_LOCAL,   ///< Local to a function/method/block
  SCOPE_FILE,    ///< File-level visibility (static in C/C++)
  SCOPE_MODULE,  ///< Module-level visibility (export in JS/TS)
  SCOPE_GLOBAL,  ///< Global visibility (public export)
  SCOPE_EXTERNAL ///< From external dependency/library
} SymbolScope;

/**
 * @brief Entry in the global symbol table
 *
 * Each entry represents a single symbol definition or declaration
 * with its metadata and a pointer to the corresponding ASTNode.
 */
typedef struct SymbolEntry {
  char *qualified_name;     ///< Fully qualified name (e.g., namespace::class::method)
  char *simple_name;        ///< Simple name without qualification (e.g., method)
  char *file_path;          ///< Path to the source file containing this symbol
  ASTNode *node;            ///< Pointer to the corresponding AST node
  SymbolScope scope;        ///< Symbol visibility scope
  Language language;        ///< Language of the source file
  struct SymbolEntry *next; ///< Next entry in case of hash collisions

  // Optional extended information
  char *module_path;          ///< Import/include path (if applicable)
  bool is_definition;         ///< Whether this is a definition (vs. declaration)
  struct SymbolEntry *parent; ///< Parent symbol (if applicable, e.g., class for a method)
} SymbolEntry;

/**
 * @brief Global symbol table with hash-based lookup
 *
 * The symbol table stores all symbols across the project and
 * enables efficient lookup by qualified name.
 */
typedef struct GlobalSymbolTable {
  SymbolEntry **buckets; ///< Hash table buckets
  size_t num_buckets;    ///< Number of buckets in the hash table
  size_t num_symbols;    ///< Total number of symbols in the table
  size_t collisions;     ///< Number of hash collisions (for statistics)

  // Compatibility fields for tests
  size_t capacity; ///< Alias for num_buckets (for test compatibility)
  size_t count;    ///< Alias for num_symbols (for test compatibility)

  // Scope and namespace tracking
  char **scope_prefixes; ///< Array of scope prefixes for resolution
  size_t num_scopes;     ///< Number of scope prefixes
} GlobalSymbolTable;

/**
 * @brief Create a new global symbol table
 *
 * @param initial_capacity Initial capacity (number of buckets)
 * @return GlobalSymbolTable* New symbol table or NULL on failure
 */
GlobalSymbolTable *symbol_table_create(size_t initial_capacity);

/**
 * @brief Free all resources associated with a symbol table
 *
 * Note: This does not free the ASTNodes referenced by the entries.
 *
 * @param table Symbol table to free
 */
void symbol_table_free(GlobalSymbolTable *table);

/**
 * @brief Register a symbol in the global table
 *
 * @param table Symbol table
 * @param qualified_name Fully qualified name (will be copied)
 * @param node AST node (not copied, ownership remains with caller)
 * @param file_path File path (will be copied)
 * @param scope Symbol visibility scope
 * @param language Language of the source file
 * @return SymbolEntry* The created entry or NULL on failure
 */
SymbolEntry *symbol_table_register(GlobalSymbolTable *table, const char *qualified_name,
                                   ASTNode *node, const char *file_path, SymbolScope scope,
                                   Language language);

/**
 * @brief Look up a symbol by its fully qualified name
 *
 * @param table Symbol table
 * @param qualified_name Fully qualified name to look up
 * @return SymbolEntry* Matching entry or NULL if not found
 */
SymbolEntry *symbol_table_lookup(const GlobalSymbolTable *table, const char *qualified_name);

/**
 * @brief Look up a symbol using scope-aware resolution
 *
 * This function attempts to resolve a possibly unqualified or partially qualified
 * name by searching through the current scope chain.
 *
 * @param table Symbol table
 * @param name Symbol name (may be partially qualified)
 * @param current_scope Current scope for resolution (e.g., namespace::class)
 * @param language Language context for resolution rules
 * @return SymbolEntry* Matching entry or NULL if not found
 */
SymbolEntry *symbol_table_scope_lookup(const GlobalSymbolTable *table, const char *name,
                                       const char *current_scope, Language language);

/**
 * @brief Add a scope prefix for resolution
 *
 * @param table Symbol table
 * @param scope_prefix Scope prefix to add (e.g., "std", "namespace1")
 * @return bool True on success, false on failure
 */
bool symbol_table_add_scope(GlobalSymbolTable *table, const char *scope_prefix);

/**
 * @brief Get all symbols of a specific type
 *
 * @param table Symbol table
 * @param type Node type to filter by
 * @param out_entries Output array of entries (can be NULL to just get the count)
 * @param max_entries Maximum number of entries to return
 * @return size_t Number of matching entries found
 */
size_t symbol_table_get_by_type(const GlobalSymbolTable *table, ASTNodeType type,
                                SymbolEntry **out_entries, size_t max_entries);

/**
 * @brief Find all symbols in a specific file
 *
 * @param table Symbol table
 * @param file_path File path to filter by
 * @param out_entries Output array of entries (can be NULL to just get the count)
 * @param max_entries Maximum number of entries to return
 * @return size_t Number of matching entries found
 */
size_t symbol_table_get_by_file(const GlobalSymbolTable *table, const char *file_path,
                                SymbolEntry **out_entries, size_t max_entries);

/**
 * @brief Get statistics about the symbol table
 *
 * @param table Symbol table
 * @param out_capacity Output parameter for table capacity
 * @param out_size Output parameter for number of symbols
 * @param out_collisions Output parameter for number of collisions
 */
void symbol_table_get_stats(const GlobalSymbolTable *table, size_t *out_capacity, size_t *out_size,
                            size_t *out_collisions);

/**
 * @brief Analyze the symbol table for optimization opportunities
 *
 * This function checks the load factor and collision rate to determine
 * if the table should be rehashed for better performance.
 *
 * @param table Symbol table
 * @return bool True if rehashing is recommended
 */
bool symbol_table_should_rehash(const GlobalSymbolTable *table);

/**
 * @brief Rehash the symbol table with a new capacity
 *
 * @param table Symbol table
 * @param new_capacity New capacity (number of buckets)
 * @return bool True on success, false on failure
 */
bool symbol_table_rehash(GlobalSymbolTable *table, size_t new_capacity);

#endif /* SCOPEMUX_SYMBOL_TABLE_H */
