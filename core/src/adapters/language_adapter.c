// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L

#include "scopemux/adapters/language_adapter.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include <stdio.h> // For snprintf
#include <stdlib.h>
#include <string.h>

extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

/**
 * Extract a full signature including return type for C functions
 * @param node The function definition node
 * @param source_code The source code string
 * @return A dynamically allocated string containing the full signature
 */
static char *c_extract_signature(TSNode node, const char *source_code) {
  if (ts_node_is_null(node) || !source_code) {
    return strdup("()"); // Fallback
  }

  // Look for the return type (primitive_type node)
  TSNode return_type_node = ts_node_named_child(node, 0);
  if (ts_node_is_null(return_type_node)) {
    return strdup("()"); // Fallback
  }

  // Get return type text
  const char *return_type = NULL;
  if (strcmp(ts_node_type(return_type_node), "primitive_type") == 0) {
    uint32_t start = ts_node_start_byte(return_type_node);
    uint32_t end = ts_node_end_byte(return_type_node);
    size_t length = end - start;
    char *str = malloc(length + 1);
    if (str) {
      strncpy(str, source_code + start, length);
      str[length] = '\0';
      return_type = str;
    }
  }

  // Look for function declarator which contains the function name and parameters
  TSNode declarator_node = ts_node_named_child(node, 1);
  if (ts_node_is_null(declarator_node)) {
    free((void *)return_type);
    return strdup("()"); // Fallback
  }

  // Extract parameter list
  const char *params = NULL;
  for (uint32_t i = 0; i < ts_node_named_child_count(declarator_node); i++) {
    TSNode child = ts_node_named_child(declarator_node, i);
    if (strcmp(ts_node_type(child), "parameter_list") == 0) {
      uint32_t start = ts_node_start_byte(child);
      uint32_t end = ts_node_end_byte(child);
      size_t length = end - start;
      char *str = malloc(length + 1);
      if (str) {
        strncpy(str, source_code + start, length);
        str[length] = '\0';
        params = str;
        break;
      }
    }
  }

  // Get function name
  const char *func_name = NULL;
  for (uint32_t i = 0; i < ts_node_named_child_count(declarator_node); i++) {
    TSNode child = ts_node_named_child(declarator_node, i);
    if (strcmp(ts_node_type(child), "identifier") == 0) {
      uint32_t start = ts_node_start_byte(child);
      uint32_t end = ts_node_end_byte(child);
      size_t length = end - start;
      char *str = malloc(length + 1);
      if (str) {
        strncpy(str, source_code + start, length);
        str[length] = '\0';
        func_name = str;
        break;
      }
    }
  }

  // Create the full signature
  char *signature = NULL;
  if (return_type && func_name && params) {
    size_t sig_len =
        strlen(return_type) + strlen(func_name) + strlen(params) + 3; // Space + () + null
    signature = malloc(sig_len);
    if (signature) {
      snprintf(signature, sig_len, "%s %s%s", return_type, func_name, params);
    }
  } else if (return_type && func_name) {
    size_t sig_len = strlen(return_type) + strlen(func_name) + 4; // Space + () + null
    signature = malloc(sig_len);
    if (signature) {
      snprintf(signature, sig_len, "%s %s()", return_type, func_name);
    }
  } else {
    signature = strdup("()"); // Fallback
  }

  // Free intermediate allocations
  free((void *)return_type);
  free((void *)func_name);
  free((void *)params);

  return signature;
}

/**
 * Generate a qualified name for a C node
 * @param name The base name
 * @param parent The parent node
 * @return A dynamically allocated qualified name string
 */
static char *c_generate_qualified_name(const char *name, ASTNode *parent) {
  if (!name) {
    return NULL;
  }

  // If parent is missing or has no valid qualified_name, just use name as is
  if (!parent || parent->type == NODE_UNKNOWN || !parent->qualified_name) {
    return strdup(name);
  }

  // For C, we use dot notation: parent.name
  size_t len = strlen(parent->qualified_name) + strlen(name) + 2; // +2 for '.' and null terminator
  char *qualified = malloc(len);
  if (!qualified) {
    return strdup(name);
  }

  snprintf(qualified, len, "%s.%s", parent->qualified_name, name);
  return qualified;
}

/**
 * Process special cases for C nodes
 * @param node The AST node to process
 * @param ctx The parser context
 */
static void c_process_special_cases(ASTNode *node, ParserContext *ctx) {
  // Currently no special cases needed for C
  (void)node;
  (void)ctx;
}

/**
 * Pre-process a Tree-sitter query for C
 * @param query_type The type of query being processed
 * @param query The Tree-sitter query
 */
static void c_pre_process_query(const char *query_type, TSQuery *query) {
  // Currently no pre-processing needed for C queries
  (void)query_type;
  (void)query;
}

/**
 * Post-process a Tree-sitter query match for C
 * @param node The AST node created from the match
 * @param match The Tree-sitter query match
 */
static void c_post_process_match(ASTNode *node, TSQueryMatch *match) {
  // Currently no post-processing needed for C matches
  (void)node;
  (void)match;
}

// The C language adapter instance
LanguageAdapter c_adapter = {.language_type = LANG_C,
                             .language_name = "C",
                             .get_ts_language = tree_sitter_c,
                             .extract_signature = c_extract_signature,
                             .generate_qualified_name = c_generate_qualified_name,
                             .process_special_cases = c_process_special_cases,
                             .pre_process_query = c_pre_process_query,
                             .post_process_match = c_post_process_match};

// Stub functions for unimplemented adapters
static char *stub_extract_signature(TSNode node, const char *source_code) { return strdup("()"); }
static char *stub_generate_qualified_name(const char *name, ASTNode *parent) {
  return strdup(name ? name : "");
}
static void stub_process_special_cases(ASTNode *node, ParserContext *ctx) {
  (void)node;
  (void)ctx;
}
static void stub_pre_process_query(const char *query_type, TSQuery *query) {
  (void)query_type;
  (void)query;
}
static void stub_post_process_match(ASTNode *node, TSQueryMatch *match) {
  (void)node;
  (void)match;
}

LanguageAdapter cpp_adapter = {.language_type = LANG_CPP,
                               .language_name = "C++",
                               .get_ts_language = tree_sitter_cpp,
                               .extract_signature = stub_extract_signature,
                               .generate_qualified_name = stub_generate_qualified_name,
                               .process_special_cases = stub_process_special_cases,
                               .pre_process_query = stub_pre_process_query,
                               .post_process_match = stub_post_process_match};

LanguageAdapter python_adapter = {.language_type = LANG_PYTHON,
                                  .language_name = "Python",
                                  .get_ts_language = tree_sitter_python,
                                  .extract_signature = stub_extract_signature,
                                  .generate_qualified_name = stub_generate_qualified_name,
                                  .process_special_cases = stub_process_special_cases,
                                  .pre_process_query = stub_pre_process_query,
                                  .post_process_match = stub_post_process_match};

LanguageAdapter javascript_adapter = {.language_type = LANG_JAVASCRIPT,
                                      .language_name = "JavaScript",
                                      .get_ts_language = tree_sitter_javascript,
                                      .extract_signature = stub_extract_signature,
                                      .generate_qualified_name = stub_generate_qualified_name,
                                      .process_special_cases = stub_process_special_cases,
                                      .pre_process_query = stub_pre_process_query,
                                      .post_process_match = stub_post_process_match};

LanguageAdapter typescript_adapter = {.language_type = LANG_TYPESCRIPT,
                                      .language_name = "TypeScript",
                                      .get_ts_language = tree_sitter_typescript,
                                      .extract_signature = stub_extract_signature,
                                      .generate_qualified_name = stub_generate_qualified_name,
                                      .process_special_cases = stub_process_special_cases,
                                      .pre_process_query = stub_pre_process_query,
                                      .post_process_match = stub_post_process_match};

// NOTE: This array is the single source of truth (SSOT) for all supported languages in ScopeMux.
// To add a new language, create a LanguageAdapter instance and add it to this array.
LanguageAdapter *all_adapters[] = {&c_adapter,          &cpp_adapter,        &python_adapter,
                                   &javascript_adapter, &typescript_adapter, NULL};

LanguageAdapter *get_adapter_by_language(Language lang) {
  for (int i = 0; all_adapters[i]; ++i) {
    if (all_adapters[i]->language_type == lang)
      return all_adapters[i];
  }
  return NULL;
}
