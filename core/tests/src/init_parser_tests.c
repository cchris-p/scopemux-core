#define DEBUG_MODE true

#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the necessary header files for tree-sitter integration
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/tree_sitter_integration.h"
#include "../../include/scopemux/tree_sitter_parser.h"

// --- Helper function (example) to read a file ---
char *read_file_to_string(const char *filepath) {
  FILE *f = fopen(filepath, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buffer = (char *)malloc(length + 1);
  if (buffer) {
    fread(buffer, 1, length, f);
    buffer[length] = '\0';
  }
  fclose(f);
  return buffer;
}

Test(cpp_parsing, tree_sitter_initialization,
     .description = "Test Tree-sitter parser initialization for C++") {
  if (DEBUG_MODE) {
    cr_log_info("Testing Tree-sitter parser initialization for C++");
  }

  // Initialize a Tree-sitter parser for C++
  TreeSitterParser *parser = ts_parser_init(LANG_CPP);
  cr_assert_not_null(parser, "Failed to initialize Tree-sitter parser for C++");

  // Verify the language type
  cr_assert_eq(parser->language, LANG_CPP, "Parser language type does not match C++");

  // Ensure the parser instance was created
  cr_assert_not_null(parser->ts_parser, "Tree-sitter parser instance is NULL");

  // Ensure the language was loaded
  cr_assert_not_null(parser->ts_language, "Tree-sitter language is NULL");

  // Clean up
  ts_parser_free(parser);

  if (DEBUG_MODE) {
    cr_log_info("Tree-sitter C++ parser test passed");
  }
}

Test(python_parsing, tree_sitter_initialization,
     .description = "Test Tree-sitter parser initialization for Python") {
  if (DEBUG_MODE) {
    cr_log_info("Testing Tree-sitter parser initialization for Python");
  }

  // Initialize a Tree-sitter parser for Python
  TreeSitterParser *parser = ts_parser_init(LANG_PYTHON);
  cr_assert_not_null(parser, "Failed to initialize Tree-sitter parser for Python");

  // Verify the language type
  cr_assert_eq(parser->language, LANG_PYTHON, "Parser language type does not match Python");

  // Ensure the parser instance was created
  cr_assert_not_null(parser->ts_parser, "Tree-sitter parser instance is NULL");

  // Ensure the language was loaded
  cr_assert_not_null(parser->ts_language, "Tree-sitter language is NULL");

  // Clean up
  ts_parser_free(parser);

  if (DEBUG_MODE) {
    cr_log_info("Tree-sitter Python parser test passed");
  }
}

// Add more Test(...) blocks for other C-level functionalities.
