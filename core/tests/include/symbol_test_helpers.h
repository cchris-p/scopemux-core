#ifndef SCOPEMUX_SYMBOL_TEST_HELPERS_H
#define SCOPEMUX_SYMBOL_TEST_HELPERS_H

#include "scopemux/parser.h"
#include "scopemux/symbol_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Symbol information for reference resolution in tests
 */
typedef struct {
  char *name;           ///< Symbol name
  char *qualified_name; ///< Fully qualified name
  SymbolType type;      ///< Symbol type
  char *file_path;      ///< Source file path
  unsigned int line;    ///< Line number
  unsigned int column;  ///< Column number
  Language language;    ///< Language of the symbol
  void *data;           ///< Optional additional data
} TestSymbol;

/**
 * Create a new test symbol with the given name and type
 *
 * @param name The name of the symbol
 * @param type The type of the symbol
 * @return A new test symbol or NULL on allocation failure
 */
TestSymbol *test_symbol_new(const char *name, SymbolType type);

/**
 * Free a test symbol and all its resources
 *
 * @param symbol The test symbol to free
 */
void test_symbol_free(TestSymbol *symbol);

/**
 * Add a symbol to a symbol table
 *
 * @param table The symbol table to add to
 * @param entry The symbol entry to add
 * @return true if added successfully, false if duplicate or error
 */
bool symbol_table_add(GlobalSymbolTable *table, SymbolEntry *entry);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_SYMBOL_TEST_HELPERS_H */