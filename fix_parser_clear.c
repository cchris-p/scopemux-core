/**
 * @brief Clear and free all resources associated with a parser context
 * 
 * This function safely cleans up all resources owned by the parser context,
 * with thorough validation to prevent double frees and detect memory corruption.
 * 
 * @param ctx Parser context to clear
 */
void parser_clear(ParserContext *ctx) {
  if (!ctx) {
    log_debug("Exit: context is NULL");
    return;
  }

  int encountered_error = 0;
  size_t freed_nodes = 0;
  
  log_info("Starting cleanup of parser context at %p", (void *)ctx);

  // First, clear the CST tree if present
  if (ctx->cst_root) {
    log_debug("Freeing CST root at %p", (void *)ctx->cst_root);
    cst_free(ctx->cst_root);
    ctx->cst_root = NULL;
  }

  // Free filename with memory tracking and validation
  if (ctx->filename) {
    log_debug("Freeing filename at %p", (void *)ctx->filename);

#ifdef _MSC_VER
    __try {
#endif
      if (memory_debug_is_valid_ptr(ctx->filename)) {
        memory_debug_untrack(ctx->filename, __FILE__, __LINE__);
        memory_debug_free(ctx->filename, __FILE__, __LINE__);
      } else {
        log_warning("Invalid filename pointer detected");
        encountered_error = 1;
      }
#ifdef _MSC_VER
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      log_error("Exception while freeing filename");
      encountered_error = 1;
    }
#endif
  }
  ctx->filename = NULL;

  // Free source code with memory tracking and validation
  if (ctx->source_code) {
    log_debug("Freeing source code at %p (length=%zu)", (void *)ctx->source_code,
              ctx->source_code_length);

#ifdef _MSC_VER
    __try {
#endif
      if (memory_debug_is_valid_ptr(ctx->source_code)) {
        memory_debug_untrack(ctx->source_code, __FILE__, __LINE__);
        memory_debug_free(ctx->source_code, __FILE__, __LINE__);
      } else {
        log_warning("Invalid source code pointer detected");
        encountered_error = 1;
      }
#ifdef _MSC_VER
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      log_error("Exception while freeing source code");
      encountered_error = 1;
    }
#endif
  }
  ctx->source_code = NULL;

  // Free last error with memory tracking and validation
  if (ctx->last_error) {
    log_debug("Freeing last error at %p", (void *)ctx->last_error);

#ifdef _MSC_VER
    __try {
#endif
      if (memory_debug_is_valid_ptr(ctx->last_error)) {
        memory_debug_untrack(ctx->last_error, __FILE__, __LINE__);
        memory_debug_free(ctx->last_error, __FILE__, __LINE__);
      } else {
        log_warning("Invalid last_error pointer detected");
        encountered_error = 1;
      }
#ifdef _MSC_VER
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      log_error("Exception while freeing last_error");
      encountered_error = 1;
    }
#endif
  }
  ctx->last_error = NULL;

  // Clear AST root pointer (no need to free separately as it's part of all_ast_nodes)
  if (ctx->ast_root) {
    log_debug("Clearing AST root pointer (no separate free needed)");
    ctx->ast_root = NULL;
  }

  // Free all AST nodes with thorough validation and memory tracking
  if (ctx->all_ast_nodes) {
    log_debug("Freeing %zu AST nodes", ctx->num_ast_nodes);

    // Validate num_ast_nodes to prevent potential buffer overrun
    size_t safe_num_nodes = ctx->num_ast_nodes;
    if (safe_num_nodes > 1000000) { // Sanity check against corrupted count
      log_error("Unreasonable num_ast_nodes (%zu), limiting to 1000000", safe_num_nodes);
      safe_num_nodes = 1000000;
      encountered_error = 1;
    }

    // Two-phase deletion to prevent corruption during cleanup
    // Phase 1: Mark nodes that should be skipped (corrupted/invalid)
    char *skip_nodes = NULL;
    if (safe_num_nodes > 0) {
      skip_nodes = (char *)memory_debug_calloc(safe_num_nodes, sizeof(char), __FILE__, __LINE__, "AST skip array");
      if (!skip_nodes) {
        log_error("Failed to allocate skip_nodes array, proceeding with caution");
        encountered_error = 1;
      } else {
        // Pre-validate all nodes and mark any corrupted ones
        for (size_t i = 0; i < safe_num_nodes; i++) {
          ASTNode *node = ctx->all_ast_nodes[i];
          if (!node) {
            skip_nodes[i] = 1; // Skip NULL nodes
            continue;
          }
          
          // Skip invalid pointers
          if (!memory_debug_is_valid_ptr(node)) {
            log_error("Invalid AST node pointer at index %zu: %p", i, (void *)node);
            skip_nodes[i] = 1;
            ctx->all_ast_nodes[i] = NULL; // Clear invalid reference
            encountered_error = 1;
            continue;
          }
          
          // Check memory canary to detect buffer overflows
          if (!memory_debug_check_canary(node, sizeof(ASTNode))) {
            log_error("Memory corruption detected in AST node %zu (buffer overflow)", i);
            skip_nodes[i] = 1;
            ctx->all_ast_nodes[i] = NULL; // Clear corrupted reference
            encountered_error = 1;
            continue;
          }
          
          // Check magic number
          if (node->magic != ASTNODE_MAGIC) {
            log_error("AST node at %p has invalid magic number 0x%x (expected 0x%x)", 
                     (void *)node, node->magic, ASTNODE_MAGIC);
            skip_nodes[i] = 1;
            ctx->all_ast_nodes[i] = NULL; // Clear invalid reference
            encountered_error = 1;
            continue;
          }
        }
      }
    }

    // Phase 2: Free valid nodes
    for (size_t i = 0; i < safe_num_nodes; i++) {
      ASTNode *node = ctx->all_ast_nodes[i];
      
      // Skip NULL nodes and those marked for skipping
      if (!node || (skip_nodes && skip_nodes[i])) {
        continue;
      }
      
      log_debug("Processing AST node %zu at %p", i, (void *)node);
      
      // Mark node as being freed
      node->magic = 0xDEADBEEF;
      
      // Free all node fields safely
      if (node->name) {
        if (memory_debug_is_valid_ptr(node->name)) {
          memory_debug_untrack(node->name, __FILE__, __LINE__);
          memory_debug_free(node->name, __FILE__, __LINE__);
        } else {
          log_warning("Invalid node->name pointer: %p", (void *)node->name);
          encountered_error = 1;
        }
        node->name = NULL;
      }
      
      if (node->qualified_name) {
        if (memory_debug_is_valid_ptr(node->qualified_name)) {
          memory_debug_untrack(node->qualified_name, __FILE__, __LINE__);
          memory_debug_free(node->qualified_name, __FILE__, __LINE__);
        } else {
          log_warning("Invalid node->qualified_name pointer: %p", (void *)node->qualified_name);
          encountered_error = 1;
        }
        node->qualified_name = NULL;
      }
      
      if (node->signature) {
        if (memory_debug_is_valid_ptr(node->signature)) {
          memory_debug_untrack(node->signature, __FILE__, __LINE__);
          memory_debug_free(node->signature, __FILE__, __LINE__);
        } else {
          log_warning("Invalid node->signature pointer: %p", (void *)node->signature);
          encountered_error = 1;
        }
        node->signature = NULL;
      }
      
      if (node->docstring) {
        if (memory_debug_is_valid_ptr(node->docstring)) {
          memory_debug_untrack(node->docstring, __FILE__, __LINE__);
          memory_debug_free(node->docstring, __FILE__, __LINE__);
        } else {
          log_warning("Invalid node->docstring pointer: %p", (void *)node->docstring);
          encountered_error = 1;
        }
        node->docstring = NULL;
      }
      
      if (node->raw_content) {
        if (memory_debug_is_valid_ptr(node->raw_content)) {
          memory_debug_untrack(node->raw_content, __FILE__, __LINE__);
          memory_debug_free(node->raw_content, __FILE__, __LINE__);
        } else {
          log_warning("Invalid node->raw_content pointer: %p", (void *)node->raw_content);
          encountered_error = 1;
        }
        node->raw_content = NULL;
      }
      
      // Free children array with validation
      if (node->children) {
        // Check if num_children is reasonable
        if (node->num_children > 1000) {
          log_error("Unreasonable num_children %zu, limiting to 1000", node->num_children);
          encountered_error = 1;
          node->num_children = 0; // Prevent buffer overrun
        }
        
        if (memory_debug_is_valid_ptr(node->children)) {
          log_debug("Freeing children array at %p (count=%zu)", 
                   (void *)node->children, node->num_children);
          memory_debug_untrack(node->children, __FILE__, __LINE__);
          memory_debug_free(node->children, __FILE__, __LINE__);
        } else {
          log_warning("Invalid children array pointer: %p", (void *)node->children);
          encountered_error = 1;
        }
        node->children = NULL;
      }
      
      // Free references array with validation
      if (node->references) {
        // Check if num_references is reasonable
        if (node->num_references > 1000) {
          log_error("Unreasonable num_references %zu, limiting to 1000", node->num_references);
          encountered_error = 1;
          node->num_references = 0; // Prevent buffer overrun
        }
        
        if (memory_debug_is_valid_ptr(node->references)) {
          log_debug("Freeing references array at %p (count=%zu)", 
                   (void *)node->references, node->num_references);
          memory_debug_untrack(node->references, __FILE__, __LINE__);
          memory_debug_free(node->references, __FILE__, __LINE__);
        } else {
          log_warning("Invalid references array pointer: %p", (void *)node->references);
          encountered_error = 1;
        }
        node->references = NULL;
      }
      
      // Finally free the node itself
#ifdef _MSC_VER
      __try {
#endif
        memory_debug_untrack(node, __FILE__, __LINE__);
        memory_debug_free(node, __FILE__, __LINE__);
        freed_nodes++;
#ifdef _MSC_VER
      } __except(EXCEPTION_EXECUTE_HANDLER) {
        log_error("Exception while freeing AST node");
        encountered_error = 1;
      }
#endif
      
      // Clear entry in array to prevent double-free
      ctx->all_ast_nodes[i] = NULL;
    }
    
    // Free the skip_nodes array if allocated
    if (skip_nodes) {
      memory_debug_untrack(skip_nodes, __FILE__, __LINE__);
      memory_debug_free(skip_nodes, __FILE__, __LINE__);
    }
    
    // Log cleanup summary
    log_info("AST node cleanup summary: freed %zu of %zu nodes, errors: %s", 
             freed_nodes, safe_num_nodes, encountered_error ? "YES" : "NO");
    
    // Free the all_ast_nodes array itself
#ifdef _MSC_VER
    __try {
#endif
      if (memory_debug_is_valid_ptr(ctx->all_ast_nodes)) {
        log_debug("Freeing all_ast_nodes array at %p", (void *)ctx->all_ast_nodes);
        memory_debug_untrack(ctx->all_ast_nodes, __FILE__, __LINE__);
        memory_debug_free(ctx->all_ast_nodes, __FILE__, __LINE__);
      } else {
        log_error("Invalid all_ast_nodes array pointer: %p", (void *)ctx->all_ast_nodes);
        encountered_error = 1;
      }
#ifdef _MSC_VER
    } __except(EXCEPTION_EXECUTE_HANDLER) {
      log_error("Exception while freeing all_ast_nodes array");
      encountered_error = 1;
    }
#endif
  }
  
  // Always reset all pointers to NULL regardless of whether they were freed
  ctx->all_ast_nodes = NULL;
  ctx->num_ast_nodes = 0;
  ctx->capacity_ast_nodes = 0;
  ctx->ast_root = NULL;
  
  // Reset all other context values to safe defaults
  ctx->source_code_length = 0;
  ctx->language = LANG_UNKNOWN;
  ctx->error_code = 0;
  
  // Report any errors encountered during cleanup
  if (encountered_error) {
    log_warning("Encountered errors during cleanup, but continued safely");
  }
  
  log_info("Successfully cleared parser context at %p", (void *)ctx);
}
