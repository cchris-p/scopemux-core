/**
 * @file ts_query_processor.c
 * @brief Implementation of Tree-sitter query processing
 *
 * This module handles the execution and processing of Tree-sitter queries,
 * following the Strategy Pattern to support different query types with
 * standardized processing logic.
 */

#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"

#include "config/node_type_mapping_loader.h"
#include "parser_internal.h"
#include "scopemux/ast.h"
#include "scopemux/logging.h"
#include "scopemux/memory_debug.h"
#include "scopemux/parser.h"

// Forward declarations and externs
extern ASTNodeType get_node_type_for_query(const char *query_type);
extern void segfault_handler(int sig);

// Ensure strdup is properly declared to avoid implicit declaration warnings
#ifndef _GNU_SOURCE
char *strdup(const char *s);
#endif

/**
 * @brief Safely extract the text and length of a Tree-sitter node from the source buffer.
 *
 * @param node The Tree-sitter node
 * @param source_code The full source buffer
 * @param source_length The length of the source buffer
 * @param out_len Output: length of the extracted text
 * @return Pointer to the start of the text in the buffer, or NULL on error
 *
 * Note: The returned pointer is NOT null-terminated and must be used with care.
 */
static const char *ts_node_text(TSNode node, const char *source_code, size_t source_length,
                                uint32_t *out_len) {
  uint32_t start = ts_node_start_byte(node);
  uint32_t end = ts_node_end_byte(node);
  if (!source_code || end > source_length || end <= start) {
    if (out_len)
      *out_len = 0;
    return NULL;
  }
  if (out_len)
    *out_len = end - start;
  return source_code + start;
}

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
  uint32_t node_type = NODE_UNKNOWN; // Initialize to avoid undefined behavior

  // Strip @ prefix if present for mapping lookup
  const char *clean_query_type = query_type;
  if (query_type[0] == '@') {
    clean_query_type++;
  }

  // Map query types to node types
  if (clean_query_type) {
    if (strcmp(clean_query_type, "function") == 0) {
      log_error("[QUERY_DEBUG] Mapped function query to NODE_FUNCTION");
      return NODE_FUNCTION;
    } else if (strcmp(clean_query_type, "functions") == 0) {
      log_error("[QUERY_DEBUG] Mapped functions query to NODE_FUNCTION");
      return NODE_FUNCTION;
    } else if (strcmp(clean_query_type, "class") == 0) {
      return NODE_CLASS;
    } else if (strcmp(clean_query_type, "classes") == 0) {
      return NODE_CLASS;
    } else if (strcmp(clean_query_type, "method") == 0) {
      return NODE_METHOD;
    } else if (strcmp(clean_query_type, "methods") == 0) {
      return NODE_METHOD;
    } else if (strcmp(clean_query_type, "variable") == 0) {
      return NODE_VARIABLE;
    } else if (strcmp(clean_query_type, "variables") == 0) {
      return NODE_VARIABLE;
    } else if (strcmp(clean_query_type, "import") == 0 ||
               strcmp(clean_query_type, "include") == 0) {
      return NODE_INCLUDE;
    } else if (strcmp(clean_query_type, "imports") == 0 ||
               strcmp(clean_query_type, "includes") == 0) {
      return NODE_INCLUDE;
    } else if (strcmp(clean_query_type, "docstring") == 0) {
      log_error("[QUERY_DEBUG] Mapped docstring query to NODE_DOCSTRING");
      return NODE_DOCSTRING;
    } else if (strcmp(clean_query_type, "docstrings") == 0) {
      log_error("[QUERY_DEBUG] Mapped docstrings query to NODE_DOCSTRING");
      return NODE_DOCSTRING;
    } else {
      log_error("[QUERY_DEBUG] Unknown query type: %s", clean_query_type);
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
    return "@name";
  }

  if (strstr(node_type, "body") || strcmp(node_type, "body") == 0) {
    return "@body";
  }

  if (strstr(node_type, "parameter") || strstr(node_type, "param") ||
      strcmp(node_type, "parameters") == 0) {
    return "@parameters";
  }

  if (strstr(node_type, "comment") || strstr(node_type, "docstring") ||
      strcmp(node_type, "doc_comment") == 0) {
    return "@docstring";
  }

  // Strip @ prefix from query type if present
  const char *clean_query_type = query_type;
  if (query_type[0] == '@') {
    clean_query_type++;
  }

  // Main node types
  if (strcmp(clean_query_type, "functions") == 0 || strcmp(clean_query_type, "function") == 0) {
    if (strstr(node_type, "function") || strstr(node_type, "method") ||
        strcmp(node_type, "function_definition") == 0) {
      return "@function";
    }
  } else if (strcmp(clean_query_type, "classes") == 0 || strcmp(clean_query_type, "class") == 0) {
    if (strstr(node_type, "class") || strstr(node_type, "struct") ||
        strcmp(node_type, "class_definition") == 0) {
      return "@class";
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
    size_t name_len = strlen(name);
    node_name = (char *)memory_debug_malloc(name_len + 1, __FILE__, __LINE__, "node_name");
    if (node_name) {
      strncpy(node_name, name, name_len);
      node_name[name_len] = '\0';
    }
  } else {
    // Try to extract name from node type
    const char *node_type_str = ts_node_type(ts_node);
    if (node_type_str) {
      size_t name_len = strlen(node_type_str);
      node_name = (char *)memory_debug_malloc(name_len + 1, __FILE__, __LINE__, "node_type_name");
      if (node_name) {
        strncpy(node_name, node_type_str, name_len);
        node_name[name_len] = '\0';
      }
    } else {
      const char *default_name = "unnamed";
      size_t name_len = strlen(default_name);
      node_name = (char *)memory_debug_malloc(name_len + 1, __FILE__, __LINE__, "default_name");
      if (node_name) {
        strncpy(node_name, default_name, name_len);
        node_name[name_len] = '\0';
      }
    }
  }

  if (!node_name) {
    return 2; // Memory allocation error
  }

  // Create the AST node
  log_debug("Calling ast_node_new with node_type=%u, node_name=%s", node_type, SAFE_STR(node_name));
  *ast_node = ast_node_new(node_type, node_name, AST_SOURCE_DEBUG_ALLOC);
  log_debug("ast_node_new returned %p", (void *)(*ast_node));
  assert(*ast_node != NULL && "ast_node_new must not return NULL");
  if (!*ast_node) {
    memory_debug_free(node_name, __FILE__, __LINE__);
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
  log_error("[QUERY_DEBUG] Query '%s' has %u patterns and %u possible captures", query_type,
            pattern_count, capture_count);

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
  const char *clean_query_type = query_type;
  if (query_type[0] == '@') {
    clean_query_type++;
  }
  uint32_t node_type = map_query_type_to_node_type(clean_query_type);
  if (node_type == NODE_UNKNOWN) {
    // Try singular form if plural form not found
    size_t len = strlen(clean_query_type);
    if (len > 1 && clean_query_type[len - 1] == 's') {
      char *singular = (char *)memory_debug_malloc(len, __FILE__, __LINE__, "singular_query_type");
      if (singular) {
        strncpy(singular, clean_query_type, len - 1);
        singular[len - 1] = '\0';
        node_type = map_query_type_to_node_type(singular);
        // 06-15-2025 - This is a library-allocated pointer
        // memory_debug_free(singular, __FILE__, __LINE__);
      }
    }
  }
  log_error("[QUERY_DEBUG] Mapped '%s' query to AST node type %u", clean_query_type, node_type);

  // Process all matches
  TSQueryMatch match;
  while (ts_query_cursor_next_match(cursor, &match)) {
    match_count++;
    uint32_t capture_count = match.capture_count;
    if (capture_count == 0) {
      continue;
    }

    log_error("[QUERY_DEBUG] Found match %d with %u captures", match_count, capture_count);

    TSNode main_node = {0};
    char *node_name = NULL;
    char *signature = NULL;
    char *docstring = NULL;
    bool docstring_is_name = false;

    ASTStringSource name_source = AST_SOURCE_NONE;
    ASTStringSource signature_source = AST_SOURCE_NONE;
    ASTStringSource docstring_source = AST_SOURCE_NONE;

    for (uint32_t i = 0; i < capture_count; i++) {
      TSQueryCapture capture = match.captures[i];
      uint32_t capture_name_length;
      const char *capture_name =
          ts_query_capture_name_for_id(query, capture.index, &capture_name_length);
      if (!capture_name)
        continue;

      TSNode node = capture.node;
      if (ts_node_is_null(node))
        continue;

      const char *node_type_str = ts_node_type(node);
      uint32_t text_len = 0;
      const char *text = ts_node_text(node, ctx->source_code, ctx->source_code_length, &text_len);
      char *text_preview = NULL;
      if (text && text_len > 0) {
        size_t preview_len = text_len < 40 ? text_len : 40;
        text_preview =
            memory_debug_strndup(text, preview_len, __FILE__, __LINE__, "capture_text_preview");
      }
      log_error("[QUERY_DEBUG] Capture %u: name='%.*s', node_type='%s', text='%.40s'", i,
                capture_name_length, capture_name, node_type_str,
                text_preview ? text_preview : "<null>");
      // 06-15-2025 - This is a library-allocated pointer
      // if (text_preview)
      //   memory_debug_free(text_preview, __FILE__, __LINE__);

      // Robust main node and capture logic
      // Identify main node (robust plural/singular and @ handling)
      bool is_main_node = false;
      // Strip @ prefix if present
      const char *clean_capture_name = capture_name;
      size_t clean_length = capture_name_length;
      if (capture_name[0] == '@') {
        clean_capture_name++;
        clean_length--;
      }
      const char *clean_query_type = query_type;
      size_t clean_query_length = strlen(query_type);
      if (query_type[0] == '@') {
        clean_query_type++;
        clean_query_length--;
      }
      // Direct match (with or without @)
      if (strncmp(clean_capture_name, clean_query_type, clean_length) == 0) {
        is_main_node = true;
      } else {
        // Handle singular/plural forms - enhanced to be more robust with partial matches
        const char *singular_forms[] = {"function", "struct", "class",
                                        "variable", "method", "docstring"};
        const char *plural_forms[] = {"functions", "structs", "classes",
                                      "variables", "methods", "docstrings"};
        for (size_t j = 0; j < sizeof(singular_forms) / sizeof(singular_forms[0]); j++) {
          // Check if clean_capture_name contains the singular form (for @function in a functions
          // query)
          if (strcmp(clean_query_type, plural_forms[j]) == 0) {
            size_t sing_len = strlen(singular_forms[j]);
            if (clean_length >= sing_len &&
                strncmp(clean_capture_name, singular_forms[j], sing_len) == 0) {
              is_main_node = true;
              fprintf(stderr, "[SINGPLUR_DEBUG] Found singular form '%s' in query type '%s'\n",
                      singular_forms[j], clean_query_type);
              break;
            }
          }
          // Check for plural form in a singular query type (less common)
          else if (strcmp(clean_query_type, singular_forms[j]) == 0) {
            size_t plur_len = strlen(plural_forms[j]);
            if (clean_length >= plur_len &&
                strncmp(clean_capture_name, plural_forms[j], plur_len) == 0) {
              is_main_node = true;
              break;
            }
          }
        }
      }
      log_error("[QUERY_DEBUG] Main node detection: query_type='%s', capture_name='%.*s', "
                "clean_capture_name='%.*s', clean_query_type='%s', is_main_node=%d",
                query_type, (int)capture_name_length, capture_name, (int)clean_length,
                clean_capture_name, clean_query_type, is_main_node);
      if (is_main_node) {
        main_node = node;
      }
      // Name capture
      if (strncmp(clean_capture_name, "name", clean_length) == 0) {
        uint32_t len = 0;
        const char *text = ts_node_text(node, ctx->source_code, ctx->source_code_length, &len);
        if (text && len > 0) {
          node_name = memory_debug_strndup(text, len, __FILE__, __LINE__, "name_capture");
          name_source = AST_SOURCE_DEBUG_ALLOC;
        }
      } else if (strncmp(clean_capture_name, "signature", clean_length) == 0) {
        uint32_t len = 0;
        const char *text = ts_node_text(node, ctx->source_code, ctx->source_code_length, &len);
        if (text && len > 0) {
          signature = memory_debug_strndup(text, len, __FILE__, __LINE__, "signature_capture");
          signature_source = AST_SOURCE_DEBUG_ALLOC;
        }
      } else if (strncmp(clean_capture_name, "docstring", clean_length) == 0) {
        uint32_t len = 0;
        const char *text = ts_node_text(node, ctx->source_code, ctx->source_code_length, &len);
        if (text && len > 0) {
          docstring = memory_debug_strndup(text, len, __FILE__, __LINE__, "docstring_capture");
          docstring_source = AST_SOURCE_DEBUG_ALLOC;
        }
      }
    }

    if (!ts_node_is_null(main_node)) {
      if (!node_name) {
        if (strcmp(query_type, "docstrings") == 0 && docstring) {
          node_name = docstring;
          name_source = AST_SOURCE_ALIAS; // Prevent double free: node_name is an alias of docstring
          docstring_is_name = true;
        } else {
          const char *type_str = ts_node_type(main_node);
          const char *fallback_format = "unnamed_%s";
          size_t len = snprintf(NULL, 0, fallback_format, type_str);
          node_name = memory_debug_malloc(len + 1, __FILE__, __LINE__, "fallback_name");
          if (node_name) {
            snprintf(node_name, len + 1, fallback_format, type_str);
            name_source = AST_SOURCE_DEBUG_ALLOC;
          }
        }
      }

      SourceRange range = {.start = {.line = ts_node_start_point(main_node).row,
                                     .column = ts_node_start_point(main_node).column},
                           .end = {.line = ts_node_end_point(main_node).row,
                                   .column = ts_node_end_point(main_node).column}};

      ASTNode *ast_node =
          ast_node_create(node_type, node_name, name_source, NULL, AST_SOURCE_NONE, range);

      if (!ast_node) {
        log_error("Failed to create AST node for query '%s'", query_type);
        if (name_source == AST_SOURCE_DEBUG_ALLOC)
          memory_debug_free(node_name, __FILE__, __LINE__);
        if (signature_source == AST_SOURCE_DEBUG_ALLOC)
          memory_debug_free(signature, __FILE__, __LINE__);
        if (docstring_source == AST_SOURCE_DEBUG_ALLOC && !docstring_is_name)
          memory_debug_free(docstring, __FILE__, __LINE__);
        continue;
      }

      if (name_source == AST_SOURCE_DEBUG_ALLOC)
        node_name = NULL;

      if (signature) {
        ast_node_set_signature(ast_node, signature, signature_source);
        if (signature_source == AST_SOURCE_DEBUG_ALLOC)
          signature = NULL;
      }

      if (docstring) {
        if (docstring_is_name) {
          ast_node_set_docstring(ast_node, ast_node->name, ast_node->name_source);
        } else {
          ast_node_set_docstring(ast_node, docstring, docstring_source);
          if (docstring_source == AST_SOURCE_DEBUG_ALLOC)
            docstring = NULL;
        }
      }

      ast_node_add_child(ast_root, ast_node);
      fprintf(stderr, "[AST_CREATE] Created ASTNode at %p, type=%u\n", (void *)ast_node, node_type);
      bool reg_result = parser_add_ast_node(ctx, ast_node);
      fprintf(stderr, "[AST_REGISTER_CALL] parser_add_ast_node(ctx, %p) returned %d\n",
              (void *)ast_node, reg_result);
      // If this is a function node, extract and assign raw content
      if (node_type == NODE_FUNCTION) {
        char *raw_content = extract_raw_content(main_node, ctx->source_code);
        if (raw_content) {
          ast_node->raw_content = raw_content;
          ast_node->owned_fields |= FIELD_RAW_CONTENT; // Mark ownership for freeing
          log_error("[QUERY_DEBUG] Set content for function node '%s'", ast_node->name);
        } else {
          log_error("[QUERY_DEBUG] Failed to extract content for function node '%s'",
                    ast_node->name);
        }
      }
      successful_captures++;
      log_error("[QUERY_DEBUG] Added %s node '%s' to AST (and registered in context)",
                ast_node_type_to_string(node_type), ast_node->name);

    } else {
      log_error("[QUERY_DEBUG] No main node found for match %d", match_count);
      // Free node_name and docstring safely (avoid double-free)
      bool freed_docstring = false;
      if (node_name && name_source == AST_SOURCE_DEBUG_ALLOC) {
        if (node_name == docstring && docstring_source == AST_SOURCE_DEBUG_ALLOC) {
          memory_debug_free(node_name, __FILE__, __LINE__);
          freed_docstring = true;
          log_error("[QUERY_DEBUG] Freed shared node_name/docstring pointer");
        } else {
          memory_debug_free(node_name, __FILE__, __LINE__);
          log_error("[QUERY_DEBUG] Freed node_name pointer");
        }
      }
      if (signature && signature_source == AST_SOURCE_DEBUG_ALLOC) {
        memory_debug_free(signature, __FILE__, __LINE__);
        log_error("[QUERY_DEBUG] Freed signature pointer");
      }
      if (docstring && docstring_source == AST_SOURCE_DEBUG_ALLOC && !freed_docstring) {
        memory_debug_free(docstring, __FILE__, __LINE__);
        log_error("[QUERY_DEBUG] Freed docstring pointer");
      }
    }
  }

  // Final statistics
  log_error("[QUERY_DEBUG] Query '%s' finished processing: %d matches, %d nodes added", query_type,
            match_count, successful_captures);

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
    // Special debug for functions query
    if (query_types[i] && strcmp(query_types[i], "functions") == 0) {
      fprintf(stderr, "[FUNCTIONS_DEBUG] About to process FUNCTIONS query at index %zu\n", i);
    }

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
