/**
 * @file ts_query_processor.c
 * @brief Implementation of Tree-sitter query processing
 *
 * This module handles the execution and processing of Tree-sitter queries,
 * following the Strategy Pattern to support different query types with
 * standardized processing logic.
 */

#include "config/node_type_mapping_loader.h"
#include "parser_internal.h"
#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/memory_debug.h"
#include "scopemux/parser.h"

// Forward declaration of functions from node_type_mapping_loader.h
extern ASTNodeType get_node_type_for_query(const char *query_type);

#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declaration for segfault handler
extern void segfault_handler(int sig);

// Ensure strdup is properly declared to avoid implicit declaration warnings
#ifndef _GNU_SOURCE
char *strdup(const char *s);
#endif

// Helper return codes for error handling
#define MATCH_OK 0
#define MATCH_SKIP 1
#define MATCH_ERROR 2

/**
 * @brief Maps a query type string to the corresponding ASTNodeType enum
 *
 * @param query_type The semantic query type (e.g., "functions", "classes")
 * @return uint32_t The corresponding ASTNodeType value
 */
static uint32_t map_query_type_to_node_type(const char *query_type) {
  // Validate input parameter
  if (!query_type) {
    log_error("NULL query_type passed to map_query_type_to_node_type");
    return NODE_UNKNOWN;
  }

  // Defensive check for empty string
  if (query_type[0] == '\0') {
    log_error("Empty query_type string passed to map_query_type_to_node_type");
    return NODE_UNKNOWN;
  }

  // Use the node type mapping system with error handling
  uint32_t node_type;

  // Safe call to get_node_type_for_query
  // This is a critical section that might cause segfaults
  // Use a direct call but with proper validation
  log_debug("Mapping query type: '%s'", query_type);
  node_type = get_node_type_for_query(query_type);
  log_debug("Query type '%s' mapped to node type %u", query_type, node_type);

  // Fall back to hardcoded mappings if needed
  if (node_type == NODE_UNKNOWN) {
    if (strcmp(query_type, "functions") == 0) {
      return NODE_FUNCTION;
    } else if (strcmp(query_type, "classes") == 0) {
      return NODE_CLASS;
    } else if (strcmp(query_type, "methods") == 0) {
      return NODE_METHOD;
    } else if (strcmp(query_type, "variables") == 0) {
      return NODE_VARIABLE;
    } else if (strcmp(query_type, "imports") == 0 || strcmp(query_type, "includes") == 0) {
      return NODE_INCLUDE;
    }
  }

  return node_type;
}

/**
 * @brief Determines the semantic capture name for a node type and query
 *
 * @param node_type Tree-sitter node type string
 * @param query_type Semantic query type
 * @return char* Capture name (statically allocated, do not free)
 */
static const char *determine_capture_name(const char *node_type, const char *query_type) {
  if (!node_type || !query_type) {
    return NULL;
  }

  // Common capture names across languages
  if (strstr(node_type, "name") || strstr(node_type, "identifier") ||
      strcmp(node_type, "name") == 0) {
    return "name";
  }

  if (strstr(node_type, "body") || strcmp(node_type, "body") == 0) {
    return "body";
  }

  if (strstr(node_type, "parameter") || strstr(node_type, "param") ||
      strcmp(node_type, "parameters") == 0) {
    return "parameters";
  }

  if (strstr(node_type, "comment") || strstr(node_type, "docstring") ||
      strcmp(node_type, "doc_comment") == 0) {
    return "docstring";
  }

  // Main node types
  if (strcmp(query_type, "functions") == 0) {
    if (strstr(node_type, "function") || strstr(node_type, "method") ||
        strcmp(node_type, "function_definition") == 0) {
      return "function";
    }
  } else if (strcmp(query_type, "classes") == 0) {
    if (strstr(node_type, "class") || strstr(node_type, "struct") ||
        strcmp(node_type, "class_definition") == 0) {
      return "class";
    }
  }

  return NULL;
}

/**
 * @brief Create an AST node from a Tree-sitter match
 *
 * @param node_type The type of AST node to create
 * @param name Optional name for the node (can be NULL)
 * @param ts_node The Tree-sitter node
 * @param ast_node Output parameter for the created node
 * @param ctx Parser context
 * @return int Status code (0 for success)
 */
static int create_node_from_match(uint32_t node_type, const char *name, TSNode ts_node,
                                  ASTNode **ast_node, ParserContext *ctx) {
  log_debug(
      "create_node_from_match: node_type=%u, name=%s, ts_node_is_null=%d, ast_node_ptr=%p, ctx=%p",
      node_type, name ? name : "(null)", ts_node_is_null(ts_node), (void *)ast_node, (void *)ctx);
  assert(ast_node != NULL && "ast_node output pointer must not be NULL");
  assert(ctx != NULL && "ParserContext must not be NULL");
  if (ts_node_is_null(ts_node) || !ast_node || !ctx) {
    return 2; // Error
  }

  // Extract node name if not provided
  char *node_name = NULL;
  if (name) {
    node_name = strdup(name);
  } else {
    // Try to extract name from node type
    const char *node_type_str = ts_node_type(ts_node);
    if (node_type_str) {
      node_name = strdup(node_type_str);
    } else {
      node_name = strdup("unnamed");
    }
  }

  if (!node_name) {
    return 2; // Memory allocation error
  }

  // Create the AST node
  log_debug("Calling ast_node_new with node_type=%u, node_name=%s", node_type,
            node_name ? node_name : "(null)");
  *ast_node = ast_node_new(node_type, node_name);
  log_debug("ast_node_new returned %p", (void *)(*ast_node));
  assert(*ast_node != NULL && "ast_node_new must not return NULL");
  if (!*ast_node) {
    free(node_name);
    return 2; // Failed to create node
  }

  // Set source range
  (*ast_node)->range.start.line = ts_node_start_point(ts_node).row;
  (*ast_node)->range.start.column = ts_node_start_point(ts_node).column;
  (*ast_node)->range.end.line = ts_node_end_point(ts_node).row;
  (*ast_node)->range.end.column = ts_node_end_point(ts_node).column;

  return 0; // Success
}

/**
 * @brief Helper to extract raw content from a node
 *
 * @param node Tree-sitter node
 * @param source_code Source code string
 * @return char* Heap-allocated content string (caller must free)
 */
static char *extract_raw_content(TSNode node, const char *source_code) {
  // Validate input parameters
  if (ts_node_is_null(node) || !source_code) {
    log_debug("extract_raw_content: Invalid parameters - node is null: %s, source_code is null: %s",
              ts_node_is_null(node) ? "yes" : "no", !source_code ? "yes" : "no");
    return NULL;
  }

  // Get node byte range
  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);

  // Validate byte range
  if (start_byte >= end_byte) {
    log_error("extract_raw_content: Invalid byte range - start: %u, end: %u", start_byte, end_byte);
    return NULL;
  }

  uint32_t length = end_byte - start_byte;

  // Check for reasonable length to prevent memory issues
  if (length > 1024 * 1024) { // 1MB limit
    log_warning("extract_raw_content: Content length exceeds limit (%u bytes)", length);
    return NULL;
  }

  // Validate source_code bounds more thoroughly
  // Check if we can safely access the first and last byte
  if (!source_code[0]) {
    log_error("extract_raw_content: Source code is empty");
    return NULL;
  }

  // Estimate source code length to validate bounds
  size_t estimated_source_length = 0;
  const char *p = source_code;
  while (*p && estimated_source_length <= end_byte + 1) {
    p++;
    estimated_source_length++;
  }

  if (estimated_source_length <= start_byte) {
    log_error("extract_raw_content: Start byte (%u) is beyond source code bounds (len ~%zu)",
              start_byte, estimated_source_length);
    return NULL;
  }

  if (estimated_source_length < end_byte) {
    log_warning("extract_raw_content: End byte (%u) exceeds source length (~%zu), truncating",
                end_byte, estimated_source_length);
    end_byte = (uint32_t)estimated_source_length;
    length = end_byte - start_byte;
  }

  // Allocate memory for content
  char *result = (char *)memory_debug_malloc(length + 1, __FILE__, __LINE__, "extract_raw_content");
  if (!result) {
    log_error("extract_raw_content: Failed to allocate memory for content");
    return NULL;
  }

  // Copy content with explicit bounds checking
  for (uint32_t i = 0; i < length; i++) {
    if (start_byte + i >= estimated_source_length) {
      // We've reached the end of the source code
      result[i] = '\0';
      log_warning("extract_raw_content: Truncated content at position %u", i);
      return result;
    }
    result[i] = source_code[start_byte + i];
  }

  result[length] = '\0';
  log_debug("extract_raw_content: Successfully extracted %u bytes", length);

  return result;
}

/**
 * @brief Processes a Tree-sitter query for a specific semantic type
 *
 * @param query_type Semantic query type (e.g., "functions", "classes")
 * @param root_node Root Tree-sitter node
 * @param ctx Parser context
 * @param ast_root AST root node
 * @param node_map Node mapping for parent relationships
 */
void process_query(const char *query_type, TSNode root_node, ParserContext *ctx, ASTNode *ast_root,
                   ASTNode **node_map) {
  log_debug("process_query: query_type=%s, root_node_is_null=%d, ctx=%p, ast_root=%p, node_map=%p",
            query_type ? query_type : "(null)", ts_node_is_null(root_node), (void *)ctx,
            (void *)ast_root, (void *)node_map);
  assert(query_type != NULL && "query_type must not be NULL");
  assert(ctx != NULL && "ParserContext must not be NULL");
  assert(ast_root != NULL && "ast_root must not be NULL");
  log_debug("ENTERING process_query with query_type: %s", query_type ? query_type : "NULL");

  // Validate all input parameters
  if (!query_type) {
    log_error("process_query: query_type is NULL");
    return;
  }

  if (ts_node_is_null(root_node)) {
    log_error("process_query: root_node is NULL for query type '%s'", query_type);
    return;
  }

  if (!ctx) {
    log_error("process_query: ctx is NULL for query type '%s'", query_type);
    return;
  }

  if (!ast_root) {
    log_error("process_query: ast_root is NULL for query type '%s'", query_type);
    return;
  }

  // Additional validation for node_map
  if (!node_map) {
    log_warning("process_query: node_map is NULL for query type '%s' - parent relationships will "
                "not be tracked",
                query_type);
    // We continue execution as node_map is optional
  }

  // Get the compiled query from the query manager
  const TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, query_type);
  if (!query) {
    if (ctx->log_level <= LOG_DEBUG) {
      log_debug(
          "No query found for type '%s' and language %d - check query file exists and is valid",
          query_type, ctx->language);
    }
    return;
  }

  int match_count = 0;

  // Create a query cursor
  TSQueryCursor *cursor = ts_query_cursor_new();
  if (!cursor) {
    log_error("Failed to create query cursor for '%s'", query_type);
    return;
  }

  // Set the query range to the entire syntax tree
  ts_query_cursor_exec(cursor, query, root_node);

  log_debug("Executing query '%s' on syntax tree", query_type);

  // Set up protection against segfaults during query matching
  jmp_buf query_recovery;
  if (setjmp(query_recovery) != 0) {
    log_error("Recovered from potential crash in query processing for '%s'", query_type);
    ts_query_cursor_delete(cursor);
    return;
  }

  // Install signal handler for this operation
  void (*prev_handler)(int) = signal(SIGSEGV, segfault_handler);

  // Process all matches with protection
  TSQueryMatch match;
  bool had_error = false;

  while (!had_error) {
    // Try to get the next match safely
    bool has_match = false;

    // Use setjmp/longjmp for error recovery
    if (setjmp(query_recovery) != 0) {
      log_error("Recovered from crash in ts_query_cursor_next_match for '%s'", query_type);
      had_error = true;
      break;
    }

    // Get the next match
    has_match = ts_query_cursor_next_match(cursor, &match);

    if (!has_match) {
      break; // No more matches
    }
    match_count++;

    // Process each match based on its pattern
    for (uint32_t i = 0; i < match.capture_count; i++) {
      TSNode captured_node = match.captures[i].node;
      uint32_t capture_index = match.captures[i].index;

      // Get the capture name
      uint32_t length;
      const char *capture_name = ts_query_capture_name_for_id(query, capture_index, &length);
      if (!capture_name) {
        if (ctx->log_level <= LOG_DEBUG) {
          log_debug("No capture name for index %d in query '%s'", capture_index, query_type);
        }
        continue;
      }

      // Map to a standard node type
      uint32_t node_type = map_query_type_to_node_type(query_type);
      if (node_type == NODE_UNKNOWN) {
        if (ctx->log_level <= LOG_DEBUG) {
          log_debug("Unknown node type for query '%s'", query_type);
        }
        continue;
      }

      // Create and populate a new AST node
      ASTNode *ast_node = NULL;
      ParseStatus status = create_node_from_match(node_type, NULL, captured_node, &ast_node, ctx);

      if (status == 0 && ast_node) {
        // Set node metadata with robust error handling
        if (ctx->source_code) {
          ast_node->raw_content = extract_raw_content(captured_node, ctx->source_code);
          if (!ast_node->raw_content && ctx->log_level <= LOG_DEBUG) {
            log_debug("Failed to extract raw content for node type %u", node_type);
          }
        } else {
          log_warning("Source code is NULL, cannot extract raw content");
          ast_node->raw_content = NULL;
        }

        // Add to the node hierarchy
        ASTNode *parent = NULL;
        // Find parent logic would be here

        if (parent) {
          ast_node_add_child(parent, ast_node);
        } else {
          ast_node_add_child(ast_root, ast_node);
        }

        // Add to node map
        if (node_map) {
          // Node map update logic would be here
        }
      }
    }
  }

  // Clean up
  ts_query_cursor_delete(cursor);

  // Restore the previous signal handler
  signal(SIGSEGV, prev_handler);

  // Report query match results
  if (ctx->log_level <= LOG_DEBUG) {
    if (match_count > 0) {
      log_debug("Query '%s' found %d matches in the syntax tree", query_type, match_count);
    } else {
      log_debug("Query '%s' did not find any matches in the syntax tree - check query correctness",
                query_type);
    }
  }
}

/**
 * @brief Process all semantic queries for a given syntax tree
 *
 * @param root_node Root Tree-sitter node
 * @param ctx Parser context
 * @param ast_root AST root node
 * @return bool True if at least one query succeeded, false otherwise
 */
bool process_all_ast_queries(TSNode root_node, ParserContext *ctx, ASTNode *ast_root) {
  log_debug("process_all_ast_queries: Starting with root_node_is_null=%d, ctx=%p, ast_root=%p",
            ts_node_is_null(root_node), (void *)ctx, (void *)ast_root);

  // Validate input parameters
  if (ts_node_is_null(root_node)) {
    log_error("process_all_ast_queries: Root node is null");
    return false;
  }

  if (!ctx) {
    log_error("process_all_ast_queries: Parser context is null");
    return false;
  }

  if (!ast_root) {
    log_error("process_all_ast_queries: AST root node is null");
    return false;
  }

  // Define query execution order for semantic hierarchy
  static const char *query_types[] = {
      "classes",   // Process classes first (for container hierarchy)
      "structs",   // C/C++ structs
      "functions", // Top-level functions
      "methods",   // Class methods
      "variables", // Variable declarations
      "imports",   // Imports/includes
      "docstrings" // Documentation strings
  };
  static const size_t num_query_types = sizeof(query_types) / sizeof(query_types[0]);

  // Count successful queries for diagnostics
  int successful_queries = 0;
  int failed_queries = 0;

  // Allocate node map for tracking parents
  // (Size would depend on implementation details)
  size_t node_map_size = 1024; // Arbitrary size for example
  ASTNode **node_map = calloc(node_map_size, sizeof(ASTNode *));
  if (!node_map) {
    log_error("Failed to allocate node map for AST processing");
    return;
  }

  // Process queries in semantic order
  for (size_t i = 0; i < num_query_types; i++) {
    if (ctx->log_level <= LOG_DEBUG) {
      log_debug("Processing query type: %s (%zu of %zu)", query_types[i], i + 1, num_query_types);
    }

    // Track AST node count before processing this query
    size_t prev_child_count = ast_root->num_children;

    // Process the query
    process_query(query_types[i], root_node, ctx, ast_root, node_map);

    // Check if new nodes were added
    if (ast_root->num_children > prev_child_count) {
      successful_queries++;
      if (ctx->log_level <= LOG_DEBUG) {
        log_debug("Query '%s' added %zu node(s)", query_types[i],
                  ast_root->num_children - prev_child_count);
      }
    } else {
      failed_queries++;
      if (ctx->log_level <= LOG_DEBUG) {
        log_debug("Query '%s' did not add any nodes", query_types[i]);
      }
    }
  }

  // Final diagnostics
  if (ctx->log_level <= LOG_INFO) {
    log_info("AST query processing complete: %d successful, %d failed, total AST nodes: %zu",
             successful_queries, failed_queries, ast_root->num_children);
  }

  // If no queries succeeded, log a warning
  if (successful_queries == 0 && ctx->log_level <= LOG_WARNING) {
    log_warning(
        "All AST queries failed to add nodes - check query patterns and grammar compatibility");
  }

  // Clean up
  free(node_map);
}
