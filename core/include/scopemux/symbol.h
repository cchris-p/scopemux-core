#ifndef SCOPEMUX_SYMBOL_H
#define SCOPEMUX_SYMBOL_H

#include "scopemux/ast.h"
#include "scopemux/language.h"

/**
 * @brief Symbol type enumeration
 */
typedef enum {
  SYMBOL_UNKNOWN = 0,
  SYMBOL_FUNCTION,
  SYMBOL_METHOD,
  SYMBOL_CLASS,
  SYMBOL_VARIABLE,
  SYMBOL_NAMESPACE,
  SYMBOL_MODULE,
  SYMBOL_TYPE,
  SYMBOL_ENUM
} SymbolType;

typedef struct Symbol {
  char *qualified_name; ///< Fully qualified name (e.g., namespace::class::method)
  ASTNode *node;        ///< Pointer to the corresponding AST node
  Language language;    ///< Language of the symbol
  bool is_definition;   ///< Whether it's a definition or declaration
} Symbol;

#endif // SCOPMUX_SYMBOL_H