/**
 * @file parser_context.c
 * @brief Implementation of parser context lifecycle management functions
 *
 * This file contains implementation for initializing, clearing, and freeing
 * parser contexts, with special focus on secure memory management.
 */

#include "parser_context.h"
#include <stdlib.h>
#include "../../core/include/scopemux/query_manager.h"
#include "ast_node.h"
#include "cst_node.h"
#include "memory_tracking.h"
#include "parser_internal.h"

/**
 * Initialize a new parser context.
 */
ParserContext *parser_init(void) {
  ParserContext *ctx = (ParserContext *)memory_debug_malloc(sizeof(ParserContext), __FILE__,
                                                            __LINE__, "parser_context");
  if (!ctx) {
    log_error("Failed to allocate memory for parser context");
    return NULL;
  }

  // Initialize with default values
  memset(ctx, 0, sizeof(ParserContext));

  // Initialize Tree-sitter parser through the integration layer
  ctx->ts_parser = ts_parser_new();
  if (!ctx->ts_parser) {
    log_error("Failed to initialize Tree-sitter parser");
    memory_debug_free(ctx, __FILE__, __LINE__);
    return NULL;
  }

  // Initialize the query manager
  ctx->q_manager = NULL;

  // Default to parse both AST and CST
  ctx->mode = PARSE_BOTH;

  // Set default log level
  ctx->log_level = LOG_INFO;

  log_info("Successfully initialized parser context at %p", (void *)ctx);
  return ctx;
}

/**
 * Clear all resources associated with a parser context.
 * This function is designed to be robust against partially freed or corrupted contexts.
 */
void parser_clear(ParserContext *ctx) {
  if (!ctx) {
    log_error("Cannot clear NULL parser context");
    return;
  }

  log_info("Clearing parser context at %p", (void *)ctx);

  // Flag to track if any errors were encountered during cleanup
  int encountered_error = 0;

  // First free the CST root as it's often the source of memory issues
  if (ctx->cst_root) {
    CSTNode *old_cst_root = ctx->cst_root;
    ctx->cst_root = NULL; // Clear reference first to prevent potential double-free

    log_debug("Freeing CST root at %p", (void *)old_cst_root);
    cst_node_free(old_cst_root);
    log_debug("CST root freed successfully");
  }

  // Free the source code
  if (ctx->source_code) {
    safe_free_field((void **)&ctx->source_code, "source_code", &encountered_error);
  }

  // Free the filename
  if (ctx->filename) {
    safe_free_field((void **)&ctx->filename, "filename", &encountered_error);
  }

  // Free the error message
  if (ctx->last_error) {
    safe_free_field((void **)&ctx->last_error, "last_error", &encountered_error);
  }

  // Note: Tree-sitter tree is not stored in the context anymore
  // It should be freed immediately after use in parser.c

  // Free the AST nodes - extra care needed here as this is where issues often occur
  log_debug("Freeing %zu AST nodes", ctx->num_ast_nodes);
  size_t freed_nodes = 0;

  // First mark all nodes as to-be-freed to prevent cyclic reference issues
  if (ctx->all_ast_nodes) {
    for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
      ASTNode *node = ctx->all_ast_nodes[i];
      if (!node)
        continue;

      // Check memory canary
      if (!memory_debug_check_canary(node, sizeof(ASTNode))) {
        log_error("Memory corruption detected in AST node %zu (buffer overflow)", i);
        ctx->all_ast_nodes[i] = NULL; // Clear corrupt reference
        encountered_error = 1;
        continue;
      }

      // Validate magic number
      if (node->magic != ASTNODE_MAGIC) {
        log_error("Invalid magic number in AST node %zu: expected 0x%X, found 0x%X", i,
                  ASTNODE_MAGIC, node->magic);
        ctx->all_ast_nodes[i] = NULL; // Clear invalid reference
        encountered_error = 1;
        continue;
      }

      // Free each node with careful error handling
#ifdef _MSC_VER
      __try {
#endif
        ast_node_free(node);
        freed_nodes++;
        ctx->all_ast_nodes[i] = NULL; // Clear reference to prevent double-free
#ifdef _MSC_VER
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        log_error("Exception while freeing AST node %zu", i);
        encountered_error = 1;
        ctx->all_ast_nodes[i] = NULL;
      }
#endif
    }
  }

  log_info("AST node cleanup summary: freed %zu of %zu nodes, errors: %s", freed_nodes,
           ctx->num_ast_nodes, encountered_error ? "YES" : "NO");

  // Free the array of AST nodes with extra validation
#ifdef _MSC_VER
  __try {
#endif
    if (ctx->all_ast_nodes && memory_debug_is_valid_ptr(ctx->all_ast_nodes)) {
      memory_debug_free(ctx->all_ast_nodes, __FILE__, __LINE__);
    }
    ctx->all_ast_nodes = NULL;
    ctx->num_ast_nodes = 0;
    ctx->ast_nodes_capacity = 0;
#ifdef _MSC_VER
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    log_error("Exception while freeing all_ast_nodes array");
    encountered_error = 1;

    // Still reset to safe values
    ctx->all_ast_nodes = NULL;
    ctx->num_ast_nodes = 0;
    ctx->ast_nodes_capacity = 0;
  }
#endif

  // Reset all context values to safe defaults to prevent issues if reused
  ctx->source_code_length = 0;
  ctx->language = LANG_UNKNOWN;
  ctx->error_code = 0;
  // Reset error state completely

  if (encountered_error) {
    log_warning("Encountered errors during cleanup, but continued safely");
  }

  log_info("Successfully cleared parser context at %p", (void *)ctx);
}

/**
 * Free a parser context and all its resources.
 */
void parser_free(ParserContext *ctx) {
  if (!ctx) {
    log_error("Cannot free NULL parser context");
    return;
  }

  log_info("Freeing parser context at %p", (void *)ctx);
  int encountered_error = 0;

  // Clear all resources
  parser_clear(ctx);

  // Free the query manager if it exists with defensive checks
  if (ctx->q_manager) {
    log_debug("Freeing query manager at %p", (void *)ctx->q_manager);

#ifdef _MSC_VER
    __try {
#endif
      query_manager_free(ctx->q_manager);
      ctx->q_manager = NULL;
      log_debug("Query manager freed successfully");
#ifdef _MSC_VER
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      log_error("Exception while freeing query manager");
      encountered_error = 1;
      ctx->q_manager = NULL;
    }
#endif
  }

  // Free the Tree-sitter parser with defensive checks
  if (ctx->ts_parser) {
    log_debug("Freeing Tree-sitter parser at %p", (void *)ctx->ts_parser);

#ifdef _MSC_VER
    __try {
#endif
      ts_parser_delete(ctx->ts_parser);
      ctx->ts_parser = NULL;
      log_debug("Tree-sitter parser freed successfully");
#ifdef _MSC_VER
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      log_error("Exception while freeing Tree-sitter parser");
      encountered_error = 1;
      ctx->ts_parser = NULL;
    }
#endif
  }

  // Finally, free the context itself
  memory_debug_free(ctx, __FILE__, __LINE__);

  if (encountered_error) {
    log_warning("Encountered errors during cleanup, but continued safely");
  }

  log_info("Parser context cleanup complete");
}

/**
 * Set the parsing mode.
 */
void parser_set_mode(ParserContext *ctx, ParseMode mode) {
  if (!ctx) {
    log_error("Cannot set mode for NULL parser context");
    return;
  }

  ctx->mode = mode;
}

/**
 * Add an AST node to the parser context's tracking array.
 */
bool parser_add_ast_node(ParserContext *ctx, ASTNode *node) {
  if (!ctx || !node) {
    log_error("Cannot add AST node: %s", !ctx ? "context is NULL" : "node is NULL");
    return false;
  }

  // Check if we need to allocate or resize the tracking array
  if (!ctx->all_ast_nodes) {
    // Initial allocation
    ctx->ast_nodes_capacity = 64; // Start with space for 64 nodes
    ctx->all_ast_nodes = (ASTNode **)memory_debug_malloc(
        ctx->ast_nodes_capacity * sizeof(ASTNode *), __FILE__, __LINE__, "ast_node_array");
    if (!ctx->all_ast_nodes) {
      log_error("Failed to allocate memory for AST node tracking array");
      return false;
    }
  } else if (ctx->num_ast_nodes >= ctx->ast_nodes_capacity) {
    // Need to resize
    size_t new_capacity = ctx->ast_nodes_capacity * 2;
    ASTNode **new_nodes = (ASTNode **)memory_debug_realloc(
        ctx->all_ast_nodes, new_capacity * sizeof(ASTNode *), __FILE__, __LINE__, "ast_node_array");
    if (!new_nodes) {
      log_error("Failed to resize AST node tracking array");
      return false;
    }
    ctx->all_ast_nodes = new_nodes;
    ctx->ast_nodes_capacity = new_capacity;
  }

  // Add the node to the tracking array
  ctx->all_ast_nodes[ctx->num_ast_nodes++] = node;
  return true;
}

/**
 * Set an error message and code in the parser context.
 */
void parser_set_error(ParserContext *ctx, int code, const char *message) {
  if (!ctx) {
    log_error("Cannot set error for NULL parser context");
    return;
  }

  // Free any existing error message
  if (ctx->last_error) {
    memory_debug_free(ctx->last_error, __FILE__, __LINE__);
    ctx->last_error = NULL;
  }

  // Set the new error information
  ctx->error_code = code;

  if (message) {
    ctx->last_error = strdup(message);
    if (!ctx->last_error) {
      log_error("Failed to duplicate error message");
      // Continue without the message rather than failing completely
    }
  }

  log_error("Parser error set: [%d] %s", code, message ? message : "(no message)");
}

/**
 * Get the last error message from the parser context.
 */
const char *parser_get_last_error(const ParserContext *ctx) {
  if (!ctx || !ctx->last_error) {
    return NULL;
  }

  return ctx->last_error;
}

/**
 * Set the CST root node in the parser context.
 */
void parser_set_cst_root(ParserContext *ctx, CSTNode *cst_root) {
  if (!ctx) {
    log_error("Cannot set CST root: parser context is NULL");
    return;
  }

  // Handle case where new root is same as existing root
  if (ctx->cst_root == cst_root) {
    log_debug("CST root unchanged (same pointer: %p)", (void *)cst_root);
    return;
  }

  // Store the old root temporarily
  CSTNode *old_root = ctx->cst_root;

  // CRITICAL: Set the new root immediately to prevent any potential double free
  // if cst_node_free somehow triggers another cleanup operation
  ctx->cst_root = cst_root;

  // Free the old root if it exists
  if (old_root) {
    log_debug("Freeing old CST root at %p (type=%s)", (void *)old_root,
              old_root->type ? old_root->type : "unknown");

    cst_node_free(old_root);
    log_debug("Old CST root freed successfully");
  }

  // Log the new state
  if (cst_root) {
    log_debug("CST root set to %p (type=%s)", (void *)cst_root,
              cst_root->type ? cst_root->type : "unknown");
  } else {
    log_debug("CST root cleared (set to NULL)");
  }
}

/**
 * Compatibility alias for parser_free. Use parser_free in new code.
 */
void parser_context_free(ParserContext *ctx) {
    parser_free(ctx);
}
