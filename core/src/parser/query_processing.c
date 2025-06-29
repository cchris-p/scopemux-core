/**
 * @file query_processing.c
 * @brief Implementation of Tree-sitter query execution and processing
 *
 * This file contains implementation for executing Tree-sitter queries
 * against parsed syntax trees and processing the results to build AST nodes.
 */

#include "query_processing.h"
#include "ast_node.h"
#include "parser_internal.h"

// Include tree-sitter API for query functions
#include "../../core/include/tree_sitter/api.h"

// Define function signatures for Tree-sitter query API to avoid compilation warnings
// These should match the actual Tree-sitter API signatures
extern TSQueryCursor *ts_query_cursor_new(void);
extern void ts_query_cursor_exec(TSQueryCursor *, const TSQuery *, TSNode);
extern bool ts_query_cursor_next_match(TSQueryCursor *, TSQueryMatch *);

// Forward declarations for internal helper functions
static bool process_function_match(ParserContext *ctx, TSQueryMatch *match);
static bool process_class_match(ParserContext *ctx, TSQueryMatch *match);
static bool process_variable_match(ParserContext *ctx, TSQueryMatch *match);
static size_t ts_query_results_match_count(TSQueryCursor *cursor);
static bool ts_query_results_get_match(void *results, size_t index, TSQueryMatch *match);

/**
 * Detect programming language from file extension and content.
 */
Language parser_detect_language(const char *filename, const char *content, size_t content_length) {
  if (!filename && !content) {
    log_error("Cannot detect language: no filename or content provided");
    return LANG_UNKNOWN;
  }

  // Try to detect by filename extension first
  if (filename) {
    const char *ext = strrchr(filename, '.');
    if (ext) {
      // Move past the dot
      ext++;

      // Check known extensions
      if (strcasecmp(ext, "c") == 0) {
        return LANG_C;
      } else if (strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "cc") == 0 ||
                 strcasecmp(ext, "cxx") == 0 || strcasecmp(ext, "h") == 0 ||
                 strcasecmp(ext, "hpp") == 0) {
        return LANG_CPP;
      } else if (strcasecmp(ext, "py") == 0) {
        return LANG_PYTHON;
      } else if (strcasecmp(ext, "js") == 0) {
        return LANG_JAVASCRIPT;
      } else if (strcasecmp(ext, "ts") == 0) {
        return LANG_TYPESCRIPT;
      }
    }
  }

  // If extension detection failed and we have content, try content-based detection
  if (content && content_length > 0) {
    // Basic content-based heuristics

    // Python typically starts with imports, shebangs, or docstrings
    if (strstr(content, "import ") || strstr(content, "from ") || strstr(content, "def ") ||
        strstr(content, "class ") || strstr(content, "#!/usr/bin/env python") ||
        strstr(content, "\"\"\"")) {
      return LANG_PYTHON;
    }

    // JavaScript/TypeScript specific features
    if (strstr(content, "function ") || strstr(content, "var ") || strstr(content, "let ") ||
        strstr(content, "const ") || strstr(content, "=>") || strstr(content, "export ")) {

      // Distinguish between JS and TS
      if (strstr(content, "interface ") || strstr(content, ": string") ||
          strstr(content, ": number") || strstr(content, ": boolean")) {
        return LANG_TYPESCRIPT;
      }
      return LANG_JAVASCRIPT;
    }

    // C/C++ specific features
    if (strstr(content, "#include") || strstr(content, "int main(")) {
      // Distinguish between C and C++
      if (strstr(content, "class ") || strstr(content, "template") ||
          strstr(content, "namespace") || strstr(content, "std::")) {
        return LANG_CPP;
      }
      return LANG_C;
    }
  }

  // If all detection methods fail
  return LANG_UNKNOWN;
}

/**
 * Execute a Tree-sitter query and process results for AST generation.
 */
bool parser_execute_query(ParserContext *ctx, const char *query_name) {
  if (!ctx || !query_name) {
    log_error("Cannot execute query: %s", !ctx ? "context is NULL" : "query name is NULL");
    return false;
  }

  if (!ctx->q_manager) {
    log_error("Cannot execute query: no query manager available");
    return false;
  }

  // Note: Tree-sitter tree is not stored in the context
  // It should be passed as a parameter or this function should be called
  // right after parsing when the tree is still available
  void *ts_tree = NULL; // This needs to be obtained from the appropriate source
  if (!ts_tree) {
    log_error("Cannot execute query: no parsed tree available");
    return false;
  }

  // Get the query from the query manager
  const struct TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, query_name);
  if (!query) {
    log_error("Failed to get query '%s' for language %d", query_name, ctx->language);
    return false;
  }

  // Tree-sitter API expects a TSNode root and a query cursor
  TSNode root_node;
  // In a real implementation, this would be obtained from the tree
  // For now, we'll just use a placeholder and return an error
  log_error("Tree-sitter integration not fully implemented yet");
  return false;

  /* The following is a template for the actual implementation

  // Create a query cursor
  TSQueryCursor *cursor = ts_query_cursor_new();
  if (!cursor) {
    log_error("Failed to create query cursor for query '%s'", query_name);
    return false;
  }

  // Execute the query using the cursor
  ts_query_cursor_exec(cursor, query, root_node);

  // Process the query results to build AST nodes
  bool success = process_query_results(ctx, cursor, query_name);

  // Free the query cursor
  ts_query_cursor_delete(cursor);
  */

  return false;
}

/**
 * Process query results and add nodes to the AST.
 *
 * Note: This is a simplified placeholder implementation. The actual implementation
 * would depend on the specific Tree-sitter query structure and parsing requirements.
 */
bool process_query_results(ParserContext *ctx, void *results, const char *query_name) {
  if (!ctx || !results || !query_name) {
    log_error("Cannot process query results: invalid parameters");
    return false;
  }

  // Get the number of matches
  size_t match_count = ts_query_results_match_count(results);
  log_debug("Query '%s' returned %zu matches", query_name, match_count);

  // Process each match
  for (size_t i = 0; i < match_count; i++) {
    // Get the current match
    TSQueryMatch match;
    if (!ts_query_results_get_match(results, i, &match)) {
      log_error("Failed to get match %zu from query results", i);
      continue;
    }

    // Process the match based on the query name
    if (strcmp(query_name, "functions") == 0) {
      // Extract function information and create AST nodes
      process_function_match(ctx, &match);
    } else if (strcmp(query_name, "classes") == 0) {
      // Extract class information and create AST nodes
      process_class_match(ctx, &match);
    } else if (strcmp(query_name, "variables") == 0) {
      // Extract variable declarations and create AST nodes
      process_variable_match(ctx, &match);
    }
    // Add more query types as needed
  }

  log_debug("Processed %zu matches for query '%s'", match_count, query_name);
  return true;
}

/**
 * Retrieve the number of matched query results
 * Iterate through all matches and count them.
 * WARNING: This will consume the cursor, so you cannot reuse it for further iteration.
 * Use this only if you do not need to access matches after counting, or if you can re-execute the
 * query.
 */

size_t ts_query_results_match_count(TSQueryCursor *cursor) {
  if (!cursor)
    return 0;
  size_t count = 0;
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    count++;
  }
  return count;
}

/**
 * Retrieve a match from query results by index.
 * WARNING: This will consume the cursor up to the requested index.
 * Use only if you do not need to access earlier matches after this call,
 * or if you can re-execute the query.
 *
 * @param results Pointer to TSQueryCursor.
 * @param index   Zero-based index of the match to retrieve.
 * @param match   Output pointer for the match.
 * @return true if match was found, false otherwise.
 */
static bool ts_query_results_get_match(void *results, size_t index, TSQueryMatch *match) {
  if (!results || !match)
    return false;
  TSQueryCursor *cursor = (TSQueryCursor *)results;
  size_t current = 0;
  TSQueryMatch temp;
  while (ts_query_cursor_next_match(cursor, &temp)) {
    if (current == index) {
      *match = temp; // Copy the match
      return true;
    }
    current++;
  }
  // Index out of range
  return false;
}

// Internal helper functions for processing different types of matches.
// These functions are responsible for extracting relevant information from Tree-sitter query
// matches and constructing the corresponding AST nodes in the ScopeMux IR.
//
// The implementation of each function depends on:
//   - The structure of the Tree-sitter queries (capture names, patterns, etc.)
//   - The language being parsed (Python, C, etc.)
//   - The expected AST node types and their fields
//
// Typical processing steps:
//   1. Iterate over match->captures and identify relevant captures by their names or indices.
//   2. Extract text spans from the source using the capture's node information.
//   3. Allocate and initialize the appropriate AST node type (e.g., AST_NODE_FUNCTION).
//   4. Attach extracted information (name, signature, body, etc.) to the AST node.
//   5. Insert the new AST node into the AST tree or context as appropriate.
//
// Caveats and TODOs:
//   - Ensure all capture indices and names are validated against the query definition.
//   - Handle language-specific constructs and differences in query structure.
//   - Add error handling for missing or malformed captures.
//   - Consider memory management for created AST nodes and extracted strings.
//   - Extend or refactor as new query patterns or AST node types are added.

static bool process_function_match(ParserContext *ctx, TSQueryMatch *match) {
  // TODO: Extract function name, parameters, and body from match->captures.
  // Example (pseudo-code):
  //   for (uint32_t i = 0; i < match->capture_count; i++) {
  //     const char *capture_name = ...; // Get capture name from query
  //     if (strcmp(capture_name, "name") == 0) { ... }
  //     else if (strcmp(capture_name, "parameters") == 0) { ... }
  //     else if (strcmp(capture_name, "body") == 0) { ... }
  //   }
  //   Create AST_NODE_FUNCTION and populate fields.
  //   Insert node into AST.
  //
  // NOTE: Actual implementation depends on Tree-sitter query definitions.
  return true;
}

static bool process_class_match(ParserContext *ctx, TSQueryMatch *match) {
  // TODO: Extract class name, base classes, and methods from match->captures.
  // Example (pseudo-code):
  //   for (uint32_t i = 0; i < match->capture_count; i++) {
  //     const char *capture_name = ...;
  //     if (strcmp(capture_name, "name") == 0) { ... }
  //     else if (strcmp(capture_name, "base") == 0) { ... }
  //     else if (strcmp(capture_name, "method") == 0) { ... }
  //   }
  //   Create AST_NODE_CLASS and populate fields.
  //   Insert node into AST.
  //
  // NOTE: Actual implementation depends on Tree-sitter query definitions.
  return true;
}

static bool process_variable_match(ParserContext *ctx, TSQueryMatch *match) {
  // TODO: Extract variable name, type, and initializer from match->captures.
  // Example (pseudo-code):
  //   for (uint32_t i = 0; i < match->capture_count; i++) {
  //     const char *capture_name = ...;
  //     if (strcmp(capture_name, "name") == 0) { ... }
  //     else if (strcmp(capture_name, "type") == 0) { ... }
  //     else if (strcmp(capture_name, "value") == 0) { ... }
  //   }
  //   Create AST_NODE_VARIABLE and populate fields.
  //   Insert node into AST.
  //
  // NOTE: Actual implementation depends on Tree-sitter query definitions.
  return true;
}
