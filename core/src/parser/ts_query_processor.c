/**
 * @file ts_query_processor.c
 * @brief Implementation of Tree-sitter query processing
 *
 * This module handles the execution and processing of Tree-sitter queries,
 * following the Strategy Pattern to support different query types with
 * standardized processing logic.
 */

#include "scopemux/ast.h"
#include "../../core/include/scopemux/logging.h"
#include "scopemux/parser.h"
#include "../../core/include/scopemux/query_manager.h"

#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  if (!query_type) {
    return NODE_UNKNOWN;
  }

  // Use the node type mapping system
  uint32_t node_type = get_node_type_for_query(query_type);

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
  *ast_node = ast_node_new(node_type, node_name);
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
  if (ts_node_is_null(node) || !source_code) {
    return NULL;
  }

  uint32_t start_byte = ts_node_start_byte(node);
  uint32_t end_byte = ts_node_end_byte(node);
  uint32_t length = end_byte - start_byte;

  char *result = (char *)malloc(length + 1);
  if (!result) {
    return NULL;
  }

  memcpy(result, source_code + start_byte, length);
  result[length] = '\0';

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
  if (!query_type || ts_node_is_null(root_node) || !ctx || !ast_root) {
    if (ctx && ctx->log_level <= LOG_DEBUG) {
      log_debug("Invalid arguments to process_query: %s%s%s%s",
              !query_type ? "query_type is null, " : "",
              ts_node_is_null(root_node) ? "root_node is null, " : "",
              !ctx ? "ctx is null, " : "",
              !ast_root ? "ast_root is null" : "");
    }
    return;
  }

  // Get the compiled query from the query manager
  const TSQuery *query = query_manager_get_query(ctx->q_manager, ctx->language, query_type);
  if (!query) {
    if (ctx->log_level <= LOG_DEBUG) {
      log_debug("No query found for type '%s' and language %d - check query file exists and is valid", 
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

  // Process all matches
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
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
        // Set node metadata
        ast_node->raw_content = extract_raw_content(captured_node, ctx->source_code);

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
  
  // Report query match results
  if (ctx->log_level <= LOG_DEBUG) {
    if (match_count > 0) {
      log_debug("Query '%s' found %d matches in the syntax tree", query_type, match_count);
    } else {
      log_debug("Query '%s' did not find any matches in the syntax tree - check query correctness", query_type);
    }
  }
}

/**
 * @brief Process all AST queries for a syntax tree
 *
 * @param root_node Root Tree-sitter node
 * @param ctx Parser context
 * @param ast_root AST root node
 */
void process_all_ast_queries(TSNode root_node, ParserContext *ctx, ASTNode *ast_root) {
  if (ts_node_is_null(root_node) || !ctx || !ast_root) {
    if (ctx && ctx->log_level <= LOG_ERROR) {
      log_error("Invalid arguments to process_all_ast_queries: %s%s%s",
               ts_node_is_null(root_node) ? "root_node is null, " : "",
               !ctx ? "ctx is null, " : "",
               !ast_root ? "ast_root is null" : "");
    }
    return;
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
      log_debug("Processing query type: %s (%zu of %zu)", query_types[i], i+1, num_query_types);
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
    log_warning("All AST queries failed to add nodes - check query patterns and grammar compatibility");
  }

  // Clean up
  free(node_map);
}
