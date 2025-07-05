#pragma once

#include "../../../../vendor/tree-sitter/lib/include/tree_sitter/api.h" // For TSNode, TSQuery, TSQueryMatch
#include "../parser.h" // For ASTNode, ParserContext
#include <stdint.h>    // For uint32_t

typedef struct LanguageAdapter {
  Language language_type;
  const char *language_name;

  // Core processing functions
  char *(*extract_signature)(TSNode node, const char *source_code);
  char *(*generate_qualified_name)(const char *name, ASTNode *parent);
  void (*process_special_cases)(ASTNode *node, ParserContext *ctx);

  // Query processing
  void (*pre_process_query)(const char *query_type, TSQuery *query);
  void (*post_process_match)(ASTNode *node, TSQueryMatch *match);

  const TSLanguage *(*get_ts_language)(void);
} LanguageAdapter;

// NOTE: This array is the single source of truth (SSOT) for all supported languages in ScopeMux.
// To add a new language, create a LanguageAdapter instance and add it to this array.
extern struct LanguageAdapter *all_adapters[];
struct LanguageAdapter *get_adapter_by_language(Language lang);
