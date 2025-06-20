#define DEBUG_MODE true

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/tree_sitter_integration.h"
#include "../include/test_helpers.h"

//=================================
// Edge Case Tests
//=================================

/**
 * Test parsing of empty files.
 * This ensures the parser handles empty input gracefully.
 */
Test(edge_cases, empty_file, .description = "Test AST extraction with empty file") {

  if (DEBUG_MODE) {
    cr_log_info("Testing AST extraction with empty file");
  }

  // Create an empty source string
  const char *source_code = "";

  // Initialize parser context
  ParserContext *ctx = parser_init();
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
    // Accept both true (success) and false (graceful no-op) as long as the error is either NULL or
    // a known "empty input" message
    const char *err = parser_get_last_error(ctx);
    // Accept "Empty input: nothing to parse" as a non-fatal error for empty input
    cr_assert(err == NULL || strstr(err, "empty") != NULL || strstr(err, "no input") != NULL ||
                  strstr(err, "Invalid arguments") != NULL ||
                  strstr(err, "Empty input: nothing to parse") != NULL,
              "Parsing empty file as %s should not produce a fatal error: %s", file_extensions[i],
              err ? err : "No error");

    // Only check AST root if parse_result is true
    if (parse_result) {
      // For empty files, we either have a valid root with no children, or no root at all
      // Both cases should be handled gracefully without crashing
      if (ctx->ast_root) {
        cr_assert_eq(ctx->ast_root->num_children, 0,
                     "AST root should have no children for empty %s file", file_extensions[i]);
      }
    }

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

  if (DEBUG_MODE) {
    cr_log_info("Testing AST extraction with invalid syntax");
  }

  // Create some clearly invalid source code for different languages
  struct {
    const char *code;
    LanguageType lang;
    const char *filename;
  } test_cases[] = {{"def missing_colon() print('hello')", LANG_PYTHON, "invalid.py"},
                    {"int main() { printf(\"Hello\") return 0; }", LANG_C, "invalid.c"}};

  // Initialize parser context
  ParserContext *ctx = parser_init();
  cr_assert_not_null(ctx, "Failed to create parser context");

  for (int i = 0; i < 2; i++) {
    // Parse the invalid code - it should still "succeed" in that we get a partial AST
    // but we expect parsing errors
    bool parse_result = parser_parse_string(ctx, test_cases[i].code, strlen(test_cases[i].code),
                                            test_cases[i].filename, test_cases[i].lang);

    // Ensure the parser provides error feedback without crashing
    const char *err2 = parser_get_last_error(ctx);
    // It's acceptable to return an error message for invalid code
    if (err2) {
      if (DEBUG_MODE) {
        cr_log_info("Parser error for invalid %s code: %s",
                    test_cases[i].lang == LANG_PYTHON ? "Python" : "C", err2);
      }
                  test_cases[i].lang == LANG_PYTHON ? "Python" : "C", err2);
    } else {
      // If no error message, the parse_result should be false
      cr_assert(!parse_result, "Expected error message or failed parse for invalid %s code",
                test_cases[i].lang == LANG_PYTHON ? "Python" : "C");
    }

    // Reset for next test case
    parser_clear(ctx);
  }

  // Clean up
  parser_free(ctx);
}

// Add more edge case tests as needed
