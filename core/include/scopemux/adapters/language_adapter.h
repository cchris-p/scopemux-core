#pragma once

#include "../common.h"                  // For LanguageType
#include "../parser.h"                  // For ASTNode, ParserContext
#include "../tree_sitter_integration.h" // For TSNode, TSQuery, TSQueryMatch
#include <stdint.h>                     // For uint32_t

typedef struct LanguageAdapter {
  LanguageType language_type;
  const char *language_name;

  // Core processing functions
  char *(*extract_signature)(TSNode node, const char *source_code);
  char *(*generate_qualified_name)(const char *name, ASTNode *parent);
  void (*process_special_cases)(ASTNode *node, ParserContext *ctx);

  // Query processing
  void (*pre_process_query)(const char *query_type, TSQuery *query);
  void (*post_process_match)(ASTNode *node, TSQueryMatch *match);
} LanguageAdapter;
