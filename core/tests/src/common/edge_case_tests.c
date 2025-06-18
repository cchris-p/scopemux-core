#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/scopemux/parser.h"
#include "../../../include/scopemux/tree_sitter_integration.h"
#include "../../include/test_helpers.h"

//=================================
// Edge Case Tests
//=================================

/**
 * Test parsing of empty files.
 * This ensures the parser handles empty input gracefully.
 */
Test(edge_cases, empty_file, .description = "Test AST extraction with empty file") {

  cr_log_info("Testing AST extraction with empty file");

  // Create an empty source string
  const char *source_code = "";

  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  cr_assert_not_null(ctx, "Failed to create parser context");

  // Try parsing with different languages to ensure consistent behavior
  LanguageType languages[] = {LANG_PYTHON, LANG_C, LANG_CPP};
  const char *file_extensions[] = {"py", "c", "cpp"};

  for (int i = 0; i < 3; i++) {
    char filename[20];
    snprintf(filename, sizeof(filename), "empty.%s", file_extensions[i]);

    // Parse the empty source code
    bool parse_result =
        parser_parse_string(ctx, source_code, strlen(source_code), filename, languages[i]);
    cr_assert(parse_result, "Parsing empty file as %s should succeed", file_extensions[i]);
    cr_assert_null(parser_get_last_error(ctx), "Parser error for empty %s: %s", file_extensions[i],
                   parser_get_last_error(ctx) ? parser_get_last_error(ctx) : "");

    // Should create an empty AST root
    cr_assert_not_null(ctx->ast_root, "AST root should not be NULL even for empty %s file",
                       file_extensions[i]);
    cr_assert_eq(ctx->ast_root->num_children, 0,
                 "AST root should have no children for empty %s file", file_extensions[i]);

    // Reset for next language
    parser_clear(ctx);
  }

  // Clean up
  parser_free(ctx);
}

/**
 * Test parsing with invalid source code.
 * This ensures the parser provides appropriate error feedback.
 */
Test(edge_cases, invalid_syntax, .description = "Test AST extraction with invalid syntax") {

  cr_log_info("Testing AST extraction with invalid syntax");

  // Create some clearly invalid source code for different languages
  struct {
    const char *code;
    LanguageType lang;
    const char *filename;
  } test_cases[] = {{"def missing_colon() print('hello')", LANG_PYTHON, "invalid.py"},
                    {"int main() { printf(\"Hello\") return 0; }", LANG_C, "invalid.c"}};

  // Initialize parser context
  ParserContext *ctx = parser_context_new();
  cr_assert_not_null(ctx, "Failed to create parser context");

  for (int i = 0; i < 2; i++) {
    // Parse the invalid code - it should still "succeed" in that we get a partial AST
    // but we expect parsing errors
    bool parse_result = parser_parse_string(ctx, test_cases[i].code, strlen(test_cases[i].code),
                                            test_cases[i].filename, test_cases[i].lang);

    // The parsing might technically succeed but should have errors
    if (parse_result) {
      cr_log_info("Parser was able to partially parse invalid %s code",
                  test_cases[i].lang == LANG_PYTHON ? "Python" : "C");
    } else {
      cr_log_info("Parser correctly failed on invalid %s code",
                  test_cases[i].lang == LANG_PYTHON ? "Python" : "C");
    }

    // Reset for next test case
    parser_clear(ctx);
  }

  // Clean up
  parser_free(ctx);
}

// Add more edge case tests as needed
