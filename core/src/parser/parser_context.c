/**
 * @file parser_context.c
 * @brief Implementation of parser context lifecycle management functions
 *
 * This file contains implementation for initializing, clearing, and freeing
 * parser contexts, with special focus on secure memory management.
 */

#include "parser_context.h"
#include "../../core/include/scopemux/query_manager.h"
#include "../../include/scopemux/ast_compliance.h"
#include "../../include/scopemux/lang_compliance.h"
#include "ast_node.h"
#include "cst_node.h"
#include "memory_tracking.h"
#include "parser_internal.h"
#include <stdlib.h>

#define SAFE_STR(x) ((x) ? (x) : "(null)")

// Defensive free macro
#define DEFENSIVE_FREE(ptr, label)                                                                 \
  do {                                                                                             \
    if (ptr) {                                                                                     \
      if (memory_debug_is_valid_ptr(ptr)) {                                                        \
        log_debug("Freeing tracked pointer (%s): %p", label, (void *)(ptr));                       \
        memory_debug_free(ptr, __FILE__, __LINE__);                                                \
      } else {                                                                                     \
        log_warning("Skipping untracked pointer (%s): %p", label, (void *)(ptr));                  \
      }                                                                                            \
      ptr = NULL;                                                                                  \
    }                                                                                              \
  } while (0)

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

  // Initialize dependency tracking
  ctx->dependencies = NULL;
  ctx->num_dependencies = 0;
  ctx->dependencies_capacity = 0;

  // Set default log level
  ctx->log_level = LOG_INFO;

  // Initialize language-specific schema compliance and post-processing callbacks
  register_all_language_compliance();
  log_info("Registered language-specific compliance callbacks");

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

  log_info("[LIFECYCLE] Entering parser_clear for ctx=%p", (void *)ctx);

  // Check for static assignment (simple heuristic: check if pointer is in static range)
  extern char __data_start, _edata, __bss_start, _end;
  if (ctx->filename) {
    if ((ctx->filename >= (char *)&__data_start && ctx->filename < (char *)&_edata) ||
        (ctx->filename >= (char *)&__bss_start && ctx->filename < (char *)&_end)) {
      log_warning("[LIFECYCLE] ctx->filename appears to point to static data: %p ('%s')",
                  (void *)ctx->filename, ctx->filename);
    }
  }
  if (ctx->source_code) {
    if ((ctx->source_code >= (char *)&__data_start && ctx->source_code < (char *)&_edata) ||
        (ctx->source_code >= (char *)&__bss_start && ctx->source_code < (char *)&_end)) {
      log_warning("[LIFECYCLE] ctx->source_code appears to point to static data: %p ('%.20s')",
                  (void *)ctx->source_code, ctx->source_code);
    }
  }

  // Free the CST root as before
  if (ctx->cst_root) {
    CSTNode *old_cst_root = ctx->cst_root;
    ctx->cst_root = NULL;
    log_debug("Freeing CST root at %p", (void *)old_cst_root);
    cst_node_free(old_cst_root);
    log_debug("CST root freed successfully");
  }

  // Defensive free for source_code
  DEFENSIVE_FREE(ctx->source_code, "source_code");
  // Defensive free for filename
  DEFENSIVE_FREE(ctx->filename, "filename");
  // Defensive free for last_error
  DEFENSIVE_FREE(ctx->last_error, "last_error");

  // Note: Tree-sitter tree is not stored in the context anymore
  // It should be freed immediately after use in parser.c

  // Free the AST nodes as before
  log_debug("Freeing %zu AST nodes", ctx->num_ast_nodes);
  size_t freed_nodes = 0;
  if (ctx->all_ast_nodes) {
    for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
      ASTNode *node = ctx->all_ast_nodes[i];
      if (!node)
        continue;
      if (!memory_debug_check_canary(node, sizeof(ASTNode))) {
        log_error("Memory corruption detected in AST node %zu (buffer overflow)", i);
        ctx->all_ast_nodes[i] = NULL;
        continue;
      }
      if (node->magic != ASTNODE_MAGIC) {
        log_error("Invalid magic number in AST node %zu: expected 0x%X, found 0x%X", i,
                  ASTNODE_MAGIC, node->magic);
        ctx->all_ast_nodes[i] = NULL;
        continue;
      }
      ast_node_free(node);
      freed_nodes++;
      ctx->all_ast_nodes[i] = NULL;
    }
  }
  log_info("AST node cleanup summary: freed %zu of %zu nodes, errors: NO", freed_nodes,
           ctx->num_ast_nodes);
  // Defensive free for all_ast_nodes array
  DEFENSIVE_FREE(ctx->all_ast_nodes, "all_ast_nodes");
  ctx->num_ast_nodes = 0;
  ctx->ast_nodes_capacity = 0;

  // Defensive free for dependencies array
  DEFENSIVE_FREE(ctx->dependencies, "dependencies");
  ctx->num_dependencies = 0;
  ctx->dependencies_capacity = 0;

  // Reset all context values to safe defaults to prevent issues if reused
  ctx->source_code_length = 0;
  ctx->language = LANG_UNKNOWN;
  ctx->error_code = 0;
  // Reset error state completely

  log_info("[LIFECYCLE] Exiting parser_clear for ctx=%p", (void *)ctx);
}

/**
 * Free a parser context and all its resources.
 */
void parser_free(ParserContext *ctx) {
  if (!ctx) {
    log_error("Cannot free NULL parser context");
    return;
  }

  log_info("[LIFECYCLE] Entering parser_free for ctx=%p", (void *)ctx);

  int encountered_error = 0;

  // Clear all resources
  parser_clear(ctx);

  // Defensive free for query manager
  if (ctx->q_manager) {
    log_debug("Freeing query manager at %p", (void *)ctx->q_manager);
    query_manager_free(ctx->q_manager);
    ctx->q_manager = NULL;
    log_debug("Query manager freed successfully");
  }

  // Defensive free for Tree-sitter parser
  if (ctx->ts_parser) {
    log_debug("Freeing Tree-sitter parser at %p", (void *)ctx->ts_parser);
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    log_debug("Tree-sitter parser freed successfully");
  }

  // Defensive free for the context itself
  if (memory_debug_is_valid_ptr(ctx)) {
    log_debug("Freeing tracked ParserContext struct: %p", (void *)ctx);
    memory_debug_free(ctx, __FILE__, __LINE__);
  } else {
    log_warning("Skipping untracked ParserContext struct: %p", (void *)ctx);
  }

  if (encountered_error) {
    log_warning("Encountered errors during cleanup, but continued safely");
  }

  log_info("[LIFECYCLE] Exiting parser_free for ctx=%p", (void *)ctx);
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

  log_error("Parser error set: [%d] %s", code, SAFE_STR(message));
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
    log_debug("Freeing old CST root at %p (type=%s)", (void *)old_root, SAFE_STR(old_root->type));

    cst_node_free(old_root);
    log_debug("Old CST root freed successfully");
  }

  // Log the new state
  if (cst_root) {
    log_debug("CST root set to %p (type=%s)", (void *)cst_root, SAFE_STR(cst_root->type));
  } else {
    log_debug("CST root cleared (set to NULL)");
  }
}

/**
 * Compatibility alias for parser_free. Use parser_free in new code.
 */
void parser_context_free(ParserContext *ctx) { parser_free(ctx); }

/**
 * Add an AST node to the parser context with an associated filename.
 */
bool parser_context_add_ast_with_filename(ParserContext *ctx, ASTNode *node, const char *filename) {
  if (!ctx || !node || !filename) {
    log_error("Cannot add AST node: invalid parameters");
    return false;
  }

  // Add the node to tracking array
  if (!parser_add_ast_node(ctx, node)) {
    return false;
  }

  // Set the filename in the node
  if (node->file_path) {
    memory_debug_free(node->file_path, __FILE__, __LINE__);
  }
  node->file_path = strdup(filename);
  if (!node->file_path) {
    log_error("Failed to duplicate filename");
    return false;
  }

  return true;
}

/**
 * Add a dependency to the parser context.
 */
bool parser_context_add_dependency(ParserContext *ctx, ParserContext *dependency) {
  if (!ctx || !dependency) {
    log_error("Cannot add dependency: invalid parameters");
    return false;
  }

  // Check if we need to allocate or resize the dependencies array
  if (!ctx->dependencies) {
    ctx->dependencies_capacity = 8;
    ctx->dependencies = (ParserContext **)memory_debug_malloc(
        ctx->dependencies_capacity * sizeof(ParserContext *), __FILE__, __LINE__, "dependencies");
    if (!ctx->dependencies) {
      log_error("Failed to allocate memory for dependencies array");
      return false;
    }
  } else if (ctx->num_dependencies >= ctx->dependencies_capacity) {
    size_t new_capacity = ctx->dependencies_capacity * 2;
    ParserContext **new_deps = (ParserContext **)memory_debug_realloc(
        ctx->dependencies, new_capacity * sizeof(ParserContext *), __FILE__, __LINE__,
        "dependencies");
    if (!new_deps) {
      log_error("Failed to resize dependencies array");
      return false;
    }
    ctx->dependencies = new_deps;
    ctx->dependencies_capacity = new_capacity;
  }

  // Add the dependency
  ctx->dependencies[ctx->num_dependencies++] = dependency;
  return true;
}
