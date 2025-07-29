/**
 * @file parser_context.c
 * @brief Implementation of parser context lifecycle management functions
 *
 * This file contains implementation for initializing, clearing, and freeing
 * parser contexts, with special focus on secure memory management.
 */

#include "parser_context.h"
#include "../../core/include/scopemux/memory_management.h"
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
  // Allocate memory for the context
  fprintf(stderr, "DEBUG: About to allocate ParserContext of size %zu\n", sizeof(ParserContext));
  void *raw_ptr = safe_malloc(sizeof(ParserContext));
  fprintf(stderr, "DEBUG: safe_malloc returned raw_ptr=%p\n", raw_ptr);
  ParserContext *ctx = (ParserContext *)raw_ptr;
  fprintf(stderr, "DEBUG: after cast ctx=%p\n", (void *)ctx);
  if (!ctx) {
    log_error("Failed to allocate memory for parser context");
    return NULL;
  }

  // Initialize with default values
  fprintf(stderr, "DEBUG: About to memset ctx=%p with size %zu\n", (void *)ctx,
          sizeof(ParserContext));
  memset(ctx, 0, sizeof(ParserContext));
  fprintf(stderr, "DEBUG: memset completed successfully\n");

  // Initialize Tree-sitter parser through the integration layer
  ctx->ts_parser = ts_parser_new();
  if (!ctx->ts_parser) {
    log_error("Failed to initialize Tree-sitter parser");
    safe_free(ctx);
    return NULL;
  }

  // Initialize the query manager
  ctx->q_manager = query_manager_init("queries");
  if (!ctx->q_manager) {
    log_error("Failed to initialize query manager");
    ts_parser_delete(ctx->ts_parser);
    safe_free(ctx);
    return NULL;
  }

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

  // Initialize node type mappings
  load_node_type_mapping();
  log_info("Initialized node type mappings");

  log_info("Successfully initialized parser context at %p", (void *)ctx);
  return ctx;
}

/**
 * Clear all resources associated with a parser context.
 * This function is designed to be robust against partially freed or corrupted contexts.
 */
void parser_clear(ParserContext *ctx) {
  if (!ctx) {
    log_debug("[LIFECYCLE] parser_clear called with NULL context");
    return;
  }

  fprintf(stderr, "[DEBUG] parser_clear starting for ctx=%p\n", (void *)ctx);
  fprintf(stderr, "[DEBUG] ctx->num_ast_nodes=%zu\n", ctx->num_ast_nodes);
  
  // Check if this context has already been cleared
  static void *last_cleared_ctx = NULL;
  if (ctx == last_cleared_ctx) {
    fprintf(stderr, "[WARNING] Attempting to clear the same context twice: %p\n", (void *)ctx);
    log_debug("[LIFECYCLE] Context %p already cleared, skipping", (void *)ctx);
    return;
  }
  
  log_debug("[LIFECYCLE] Entering parser_clear for ctx=%p", (void *)ctx);

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

  // Free source_code
  if (ctx->source_code) {
    safe_free(ctx->source_code);
    ctx->source_code = NULL;
  }

  // Free filename
  if (ctx->filename) {
    safe_free(ctx->filename);
    ctx->filename = NULL;
  }

  // Free last_error
  if (ctx->last_error) {
    safe_free(ctx->last_error);
    ctx->last_error = NULL;
  }

  // Note: Tree-sitter tree is not stored in the context anymore
  // It should be freed immediately after use in parser.c

  // Free the query manager
  if (ctx->q_manager) {
    query_manager_free(ctx->q_manager);
    ctx->q_manager = NULL;
  }

  // Free the AST nodes - ONLY free root nodes to avoid double-free
  // Root nodes will recursively free their children
  log_debug("Freeing AST nodes (root nodes only to avoid double-free)");
  size_t freed_nodes = 0;
  size_t skipped_children = 0;
  if (ctx->all_ast_nodes) {
    for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
      ASTNode *node = ctx->all_ast_nodes[i];
      if (!node) {
        log_debug("[AST_FREE] Skipping NULL node at index %zu", i);
        continue;
      }

      fprintf(stderr, "[DEBUG] About to check magic for node at index %zu, ptr=%p\n", i, (void *)node);
      
      // Check magic number first to detect already-freed nodes
      // Use volatile to prevent compiler optimization that might cache the value
      volatile uint32_t magic = node->magic;
      
      fprintf(stderr, "[DEBUG] Successfully read magic=0x%X for node at index %zu, ptr=%p\n", magic, i, (void *)node);
      if (magic != ASTNODE_MAGIC) {
        if (magic == 0xDEADBEEF) {
          log_debug("[AST_FREE] Skipping already-freed node at index %zu, ptr=%p", i, (void *)node);
        } else {
          log_debug("[AST_FREE] Invalid magic number in AST node %zu: expected 0x%X, found 0x%X "
                    "(possibly freed)",
                    i, ASTNODE_MAGIC, magic);
        }
        ctx->all_ast_nodes[i] = NULL;
        continue;
      }

      if (!memory_debug_check_canary(node, sizeof(ASTNode))) {
        log_error("[AST_FREE] Memory corruption detected in AST node %zu (buffer overflow)", i);
        ctx->all_ast_nodes[i] = NULL;
        continue;
      }

      // CRITICAL: Only free root nodes (nodes without parents)
      // Child nodes will be freed recursively by their parents
      if (node->parent != NULL) {
        log_debug("[AST_FREE] Skipping child node at index %zu, ptr=%p (parent=%p)", i,
                  (void *)node, (void *)node->parent);
        skipped_children++;
        ctx->all_ast_nodes[i] = NULL;
        continue;
      }

      log_debug("[AST_FREE] About to free ROOT ASTNode at index %zu, ptr=%p, magic=0x%X", i,
                (void *)node, node->magic);
      ast_node_free(node); // This will recursively free all children
      log_debug("[AST_FREE] Freed ROOT ASTNode at index %zu, ptr=%p (and all its children)", i,
                (void *)node);
      freed_nodes++;
      ctx->all_ast_nodes[i] = NULL;
    }
  }
  log_info("AST node cleanup summary: freed %zu root nodes, skipped %zu child nodes (freed "
           "recursively), total tracked: %zu",
           freed_nodes, skipped_children, ctx->num_ast_nodes);

  // Free all_ast_nodes array
  if (ctx->all_ast_nodes) {
    safe_free(ctx->all_ast_nodes);
    ctx->all_ast_nodes = NULL;
  }
  ctx->num_ast_nodes = 0;
  ctx->ast_nodes_capacity = 0;

  // Free dependencies array
  if (ctx->dependencies) {
    safe_free(ctx->dependencies);
    ctx->dependencies = NULL;
  }
  ctx->num_dependencies = 0;
  ctx->dependencies_capacity = 0;

  // Reset all context values to safe defaults to prevent issues if reused
  ctx->source_code_length = 0;
  ctx->language = LANG_UNKNOWN;
  ctx->error_code = 0;
  // Reset error state completely

  // Mark this context as cleared to prevent double-clearing
  last_cleared_ctx = ctx;
  
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

  // First clear all resources
  parser_clear(ctx);

  // Free the Tree-sitter parser
  if (ctx->ts_parser) {
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
  }

  // Free the query manager
  if (ctx->q_manager) {
    query_manager_free(ctx->q_manager);
    ctx->q_manager = NULL;
  }

  // Free the dependencies array
  if (ctx->dependencies) {
    safe_free(ctx->dependencies);
    ctx->dependencies = NULL;
  }

  // Free the context itself
  safe_free(ctx);
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
  size_t idx = ctx->num_ast_nodes;
  ctx->all_ast_nodes[ctx->num_ast_nodes++] = node;
  log_debug("[AST_REGISTER] Registered ASTNode at idx=%zu, ptr=%p, total now=%zu", idx,
            (void *)node, ctx->num_ast_nodes);
  return true;
}

/**
 * Remove an AST node from the parser context's tracking array.
 */
bool parser_remove_ast_node(ParserContext *ctx, ASTNode *node) {
  if (!ctx || !node) {
    log_error("Cannot remove AST node: %s", !ctx ? "context is NULL" : "node is NULL");
    return false;
  }

  if (!ctx->all_ast_nodes || ctx->num_ast_nodes == 0) {
    log_debug("[AST_UNREGISTER] No nodes to remove from context");
    return false;
  }

  // Find the node in the tracking array
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    if (ctx->all_ast_nodes[i] == node) {
      log_debug("[AST_UNREGISTER] Found node at idx=%zu, ptr=%p", i, (void *)node);

      // Remove by setting to NULL (don't shift array to avoid invalidating indices)
      ctx->all_ast_nodes[i] = NULL;

      log_debug("[AST_UNREGISTER] Removed ASTNode at idx=%zu, ptr=%p", i, (void *)node);
      return true;
    }
  }

  log_debug("[AST_UNREGISTER] Node not found in tracking array: ptr=%p", (void *)node);
  return false;
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
