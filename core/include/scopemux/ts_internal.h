#ifndef TS_INTERNAL_H
#define TS_INTERNAL_H

#include "scopemux/parser.h"
#include "tree_sitter/api.h"

// Parser initialization 
bool ts_init_parser_impl(ParserContext *ctx, LanguageType language);

// AST generation
ASTNode *ts_tree_to_ast_impl(TSNode root_node, ParserContext *ctx);

// CST generation
CSTNode *ts_tree_to_cst_impl(TSNode root_node, ParserContext *ctx);

// Query processing
int process_query(ParserContext *ctx, const char *query_path, TSNode root_node);

#endif /* TS_INTERNAL_H */
