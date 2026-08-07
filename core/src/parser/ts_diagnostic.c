#include <stdio.h>
#include <stdlib.h>

// Include Tree-sitter required headers directly from system paths
#include "scopemux/adapters/language_adapter.h"
#include <tree_sitter/api.h>

// Extern declarations for all tree_sitter_* parser functions
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

/**
 * Diagnostic program to verify Tree-sitter language function bindings
 *
 * This program tests the ability to load Tree-sitter language objects
 * directly from the compiled language libraries.
 */
int main() {
  printf("\n===== TREE-SITTER LANGUAGE DIAGNOSTIC =====\n\n");

  printf("Function addresses:\n");
  printf("  tree_sitter_c:          %p\n", (void *)&tree_sitter_c);
  printf("  tree_sitter_cpp:        %p\n", (void *)&tree_sitter_cpp);
  printf("  tree_sitter_python:     %p\n", (void *)&tree_sitter_python);
  printf("  tree_sitter_javascript: %p\n", (void *)&tree_sitter_javascript);
  printf("  tree_sitter_typescript: %p\n", (void *)&tree_sitter_typescript);

  printf("\nFunction call results:\n");
  const TSLanguage *c_lang = tree_sitter_c();
  const TSLanguage *cpp_lang = tree_sitter_cpp();
  const TSLanguage *python_lang = tree_sitter_python();
  const TSLanguage *js_lang = tree_sitter_javascript();
  const TSLanguage *ts_lang = tree_sitter_typescript();

  printf("  tree_sitter_c():          %p\n", (void *)c_lang);
  printf("  tree_sitter_cpp():        %p\n", (void *)cpp_lang);
  printf("  tree_sitter_python():     %p\n", (void *)python_lang);
  printf("  tree_sitter_javascript(): %p\n", (void *)js_lang);
  printf("  tree_sitter_typescript(): %p\n", (void *)ts_lang);

  printf("\nTesting parser initialization with C++:\n");
  if (cpp_lang) {
    printf("  C++ language object available, initializing parser...\n");
    TSParser *parser = ts_parser_new();
    if (parser) {
      bool success = ts_parser_set_language(parser, cpp_lang);
      printf("  ts_parser_set_language result: %s\n", success ? "SUCCESS" : "FAILURE");

      const TSLanguage *set_lang = ts_parser_language(parser);
      printf("  ts_parser_language result: %p (should match cpp_lang)\n", (void *)set_lang);

      ts_parser_delete(parser);
      printf("  Parser cleanup complete\n");
    } else {
      printf("  ERROR: Failed to create parser\n");
    }
  } else {
    printf("  ERROR: C++ language object is NULL, cannot initialize parser\n");
  }

  // For diagnostics, iterate over all_adapters and print their get_ts_language pointer addresses
  // and results
  for (int i = 0; all_adapters[i]; ++i) {
    LanguageAdapter *adapter = all_adapters[i];
    printf("  %s: %p\n", adapter->language_name, (void *)adapter->get_ts_language);
    if (adapter->get_ts_language) {
      const TSLanguage *lang_ptr = adapter->get_ts_language();
      printf("  %s(): %p\n", adapter->language_name, (void *)lang_ptr);
    }
  }

  printf("\n===== DIAGNOSTIC COMPLETE =====\n");
  return 0;
}
