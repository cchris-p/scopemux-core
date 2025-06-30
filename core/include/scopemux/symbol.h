#ifndef SCOPMUX_SYMBOL_H
#define SCOPMUX_SYMBOL_H

#include "scopemux/ast.h"
#include "scopemux/language.h"

typedef struct Symbol {
  char *qualified_name; ///< Fully qualified name (e.g., namespace::class::method)
  ASTNode *node;        ///< Pointer to the corresponding AST node
  Language language;    ///< Language of the symbol
  bool is_definition;   ///< Whether it's a definition or declaration
} Symbol;

#endif // SCOPMUX_SYMBOL_H