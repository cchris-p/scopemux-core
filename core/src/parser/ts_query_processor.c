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

#define SAFE_STR(x) ((x) ? (x) : "(null)")

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
  log_debug("Mapping query type: '%s'", SAFE_STR(query_type));
  node_type = get_node_type_for_query(query_type);
  log_debug("Query type '%s' mapped to node type %u", SAFE_STR(query_type), node_type);

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
  log_debug("Calling ast_node_new with node_type=%u, node_name=%s", node_type, SAFE_STR(node_name));
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
  if (!query_type || !ctx || !ast_root || !node_map) {
    log_error("[QUERY_PROCESSOR] Invalid parameters to process_query");
    return;
  }

  // Defensive check for NULL root_node
  if (ts_node_is_null(root_node)) {
    log_error("[QUERY_PROCESSOR] Cannot process query with NULL root node");
    return;
  }

  log_error("[QUERY_DEBUG] Processing query type: %s", query_type);

  // Get the query for this language and query type
  const TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, query_type);
  if (!query) {
    log_error("[QUERY_PROCESSOR] Failed to get query for %s", query_type);
    return;
  }

  // DEBUG: Log query details
  uint32_t pattern_count = ts_query_pattern_count(query);
  uint32_t capture_count = ts_query_capture_count(query);
  log_error("[QUERY_DEBUG] Query '%s' has %u patterns and %u possible captures",
            query_type, pattern_count, capture_count);

  // DEBUG: Tree-sitter does not provide pattern string introspection in the C API.
  // This block is a placeholder for future Tree-sitter API upgrades or custom debug info.
  // log_error("[QUERY_DEBUG] Query pattern introspection not available in C API");

  // Create a query cursor for executing the query
  TSQueryCursor *cursor = ts_query_cursor_new();
  if (!cursor) {
    log_error("[QUERY_PROCESSOR] Failed to create query cursor");
    return;
  }

  // Set the query cursor to use the root node
  ts_query_cursor_exec(cursor, query, root_node);

  // DEBUG: Track matches count
  int match_count = 0;
  int successful_captures = 0;

  // Map the query type to the appropriate AST node type
  uint32_t node_type = map_query_type_to_node_type(query_type);
  log_error("[QUERY_DEBUG] Mapped '%s' query to AST node type %u", query_type, node_type);

  // Process all matches
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    match_count++;
    // Get the capture count for this match
    uint32_t capture_count = match.capture_count;

    // Skip if no captures
    if (capture_count == 0) {
      continue;
    }

    log_error("[QUERY_DEBUG] Found match %d with %u captures", match_count, capture_count);

    // Track main node and its name for this match
    TSNode main_node = {0};
    char *node_name = NULL;
    bool node_name_is_debug_alloc = false; // Track if node_name was memory_debug_malloc'd

    // First pass: find the main capture and name (if any)
    for (uint32_t i = 0; i < capture_count; i++) {
      TSQueryCapture capture = match.captures[i];
      // Get capture name
      uint32_t capture_name_length;
      const char *capture_name = ts_query_capture_name_for_id(query, capture.index, &capture_name_length);
      if (!capture_name) continue;
      TSNode node = capture.node;
      if (ts_node_is_null(node)) continue;
      const char *node_type_str = ts_node_type(node);
      log_error("[QUERY_DEBUG] Capture %u: name='%.*s', node_type='%s'", 
                i, capture_name_length, capture_name, node_type_str);
      // Special case: For docstring queries, always use the first comment node as main_node
      if (strcmp(query_type, "docstrings") == 0 && main_node.id == 0) {
        if (strcmp(node_type_str, "comment") == 0) {
          main_node = node;
          log_error("[QUERY_DEBUG] For docstrings, set main_node to comment node at match %d, capture %u", match_count, i);
        }
      }
      // Find the main capture (function, struct, class, etc.)
      if (strncmp(capture_name, query_type, strlen(query_type)) == 0 ||
          strncmp(capture_name, "function", 8) == 0 ||
          strncmp(capture_name, "struct", 6) == 0 ||
          strncmp(capture_name, "class", 5) == 0 ||
          strncmp(capture_name, "variable", 8) == 0 ||
          strncmp(capture_name, "method", 6) == 0) {
        main_node = node;
      }
      // Find name capture
      if (strncmp(capture_name, "name", capture_name_length) == 0) {
        // Extract name text
        uint32_t start_byte = ts_node_start_byte(node);
        uint32_t end_byte = ts_node_end_byte(node);
        if (ctx->source_code && start_byte < ctx->source_code_length && end_byte <= ctx->source_code_length && end_byte > start_byte) {
          uint32_t length = end_byte - start_byte;
          node_name = (char *)memory_debug_malloc(length + 1, __FILE__, __LINE__, "name_capture");
          if (node_name) {
            strncpy(node_name, ctx->source_code + start_byte, length);
            node_name[length] = '\0';
            log_error("[QUERY_DEBUG] Extracted name: '%s'", node_name);
            node_name_is_debug_alloc = true;
          }
        } else {
          log_error("[QUERY_DEBUG] Skipped extracting name: invalid source_code or out-of-bounds [%u,%u] (len %u)", start_byte, end_byte, ctx->source_code_length);
        }
      }
    }

    // If we found a main node, create an AST node for it
    if (!ts_node_is_null(main_node)) {
      // Use default name if none found
      if (!node_name) {
        // For docstring queries, use the text of the main_node (comment) as the name if possible
        if (strcmp(query_type, "docstrings") == 0 && !ts_node_is_null(main_node) && ctx->source_code) {
          uint32_t start_byte = ts_node_start_byte(main_node);
          uint32_t end_byte = ts_node_end_byte(main_node);
          if (start_byte < ctx->source_code_length && end_byte <= ctx->source_code_length && end_byte > start_byte) {
            uint32_t length = end_byte - start_byte;
            node_name = (char *)memory_debug_malloc(length + 1, __FILE__, __LINE__, "docstring_comment");
            if (node_name) {
              strncpy(node_name, ctx->source_code + start_byte, length);
              node_name[length] = '\0';
              node_name_is_debug_alloc = true;
              log_error("[QUERY_DEBUG] Used comment text as docstring name: '%s'", node_name);
            }
          } else {
            log_error("[QUERY_DEBUG] Could not extract docstring comment text: invalid bounds [%u,%u] (len %u)", start_byte, end_byte, ctx->source_code_length);
          }
        }
        if (!node_name) {
          const char *default_name = "unnamed";
          node_name = (char *)default_name; // Do not free this
          log_error("[QUERY_DEBUG] Using default name: '%s'", default_name);
        }
      }

      // Create AST node with appropriate type
      ASTNode *ast_node = ast_node_create(
          node_type, node_name, node_name,
          (SourceRange){.start = {.line = ts_node_start_point(main_node).row,
                                  .column = ts_node_start_point(main_node).column},
                        .end = {.line = ts_node_end_point(main_node).row,
                                .column = ts_node_end_point(main_node).column}});

      if (ast_node) {
        // Successfully created node
        successful_captures++;
        // Add the node to the AST
        ast_node_add_child(ast_root, ast_node);
        log_error("[QUERY_DEBUG] Added %s node '%s' to AST", 
                  ast_node_type_to_string(node_type), node_name);
      } else {
        log_error("[QUERY_DEBUG] Failed to create AST node for %s", node_name);
      }

      // Free the name (it's been copied in ast_node_create)
      if (node_name_is_debug_alloc) {
        memory_debug_free(node_name, __FILE__, __LINE__);
      } // else do not free string literal or strdup'd fallback
    } else if (node_name) {
      // We found a name but no main node
      log_error("[QUERY_DEBUG] Found name '%s' but no main node for query '%s'", 
                node_name, query_type);
      memory_debug_free(node_name, __FILE__, __LINE__);
    } else {
      log_error("[QUERY_DEBUG] No main node or name found for match %d", match_count);
    }
  }

  // Final statistics
  log_error("[QUERY_DEBUG] Query '%s' finished processing: %d matches, %d nodes added",
            query_type, match_count, successful_captures);

  // Free the query cursor
  ts_query_cursor_delete(cursor);
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

  // ENHANCED LOGGING: Log source code preview
  if (ctx->source_code && ctx->source_code_length > 0) {
    size_t preview_len = ctx->source_code_length < 100 ? ctx->source_code_length : 100;
    char preview[101] = {0};
    strncpy(preview, ctx->source_code, preview_len);
    log_error("[QUERY_DEBUG] Source code preview (first %zu bytes): '%s%s'", preview_len, preview,
              ctx->source_code_length > 100 ? "..." : "");
  } else {
    log_error("[QUERY_DEBUG] Source code is NULL or empty!");
  }

  // ENHANCED LOGGING: Log Tree-sitter node structure
  log_error("[QUERY_DEBUG] Tree-sitter root node: type='%s', named=%d, child_count=%u",
            ts_node_type(root_node), ts_node_is_named(root_node), ts_node_child_count(root_node));

  // Log a few children for context
  for (uint32_t i = 0; i < ts_node_child_count(root_node) && i < 5; i++) {
    TSNode child = ts_node_child(root_node, i);
    log_error("[QUERY_DEBUG] Child %u: type='%s', named=%d", i, ts_node_type(child),
              ts_node_is_named(child));
  }

  // Allocate node map for tracking parents
  // (Size would depend on implementation details)
  size_t node_map_size = 1024; // Arbitrary size for example
  ASTNode **node_map = calloc(node_map_size, sizeof(ASTNode *));
  if (!node_map) {
    log_error("Failed to allocate node map for AST processing");
    return false;
  }

  // Process queries in semantic order
  for (size_t i = 0; i < num_query_types; i++) {
    // ENHANCED LOGGING: Log query processing start with more detail
    log_error("[QUERY_DEBUG] Processing query type: %s (%zu of %zu)", SAFE_STR(query_types[i]),
              i + 1, num_query_types);

    // Track AST node count before processing this query
    size_t prev_child_count = ast_root->num_children;

    // Process the query
    process_query(query_types[i], root_node, ctx, ast_root, node_map);

    // Check if new nodes were added
    if (ast_root->num_children > prev_child_count) {
      successful_queries++;
      // ENHANCED LOGGING: Log successful query with more detail
      log_error("[QUERY_DEBUG] Query '%s' SUCCEEDED: Added %zu node(s)", SAFE_STR(query_types[i]),
                ast_root->num_children - prev_child_count);
    } else {
      failed_queries++;
      // ENHANCED LOGGING: Log failed query with more detail
      log_error("[QUERY_DEBUG] Query '%s' FAILED: No nodes added", SAFE_STR(query_types[i]));
    }
  }

  // Final diagnostics
  if (ctx->log_level <= LOG_INFO) {
    log_info("AST query processing complete: %d successful, %d failed, total AST nodes: %zu",
             successful_queries, failed_queries, ast_root->num_children);
  }

  // ENHANCED LOGGING: More detailed summary
  log_error(
      "[QUERY_DEBUG] AST query processing summary: %d successful, %d failed, total AST nodes: %zu",
      successful_queries, failed_queries, ast_root->num_children);

  // If no queries succeeded, log a warning
  if (successful_queries == 0 && ctx->log_level <= LOG_WARNING) {
    log_warning(
        "All AST queries failed to add nodes - check query patterns and grammar compatibility");
  }

  // Clean up
  free(node_map);

  return successful_queries > 0;
}
