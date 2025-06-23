#define _POSIX_C_SOURCE 200809L // For strdup

#include "../../core/include/scopemux/context_engine.h"
#include <stdio.h>  // For snprintf
#include <stdlib.h> // For malloc, calloc, free
#include <string.h> // For strlen, strncpy, strdup

/**
 * @brief Get the last error message from a ContextEngine instance.
 *
 * @param engine Pointer to the ContextEngine instance.
 * @return The last error message, or NULL if not available.
 */
const char *context_engine_get_last_error(const ContextEngine *engine) {
  if (!engine)
    return NULL;
  return engine->last_error;
}

/**
 * @brief Get compressed context as a string.
 *
 * @param engine Context engine
 * @param out_buffer Output buffer (can be NULL to get required size)
 * @param buffer_size Size of output buffer
 * @return size_t Size of the context (excluding null terminator)
 */
size_t context_engine_get_context(const ContextEngine *engine, char *out_buffer,
                                  size_t buffer_size) {
  if (!engine) {
    return 0;
  }

  // Calculate the total size needed for all compressed blocks
  size_t total_size = 0;
  for (InfoBlock *block = engine->blocks; block != NULL; block = block->next) {
    if (block->compressed_content) {
      total_size += strlen(block->compressed_content) + 1; // +1 for newline
    }
  }

  // If out_buffer is NULL, just return the required size
  if (!out_buffer) {
    return total_size;
  }

  // Copy the blocks to the output buffer, ensuring we don't overflow
  size_t remaining = buffer_size > 0 ? buffer_size - 1 : 0; // -1 for null terminator
  size_t copied = 0;

  if (remaining > 0 && engine->blocks) {
    char *pos = out_buffer;

    for (InfoBlock *block = engine->blocks; block != NULL && remaining > 0; block = block->next) {
      if (block->compressed_content) {
        size_t block_len = strlen(block->compressed_content);
        size_t copy_size = block_len < remaining ? block_len : remaining;

        if (copy_size > 0) {
          strncpy(pos, block->compressed_content, copy_size);
          pos += copy_size;
          copied += copy_size;
          remaining -= copy_size;

          // Add a newline between blocks if there's space
          if (remaining > 0 && block->next) {
            *pos = '\n';
            pos++;
            copied++;
            remaining--;
          }
        }
      }
    }

    // Ensure null termination
    *pos = '\0';
  } else if (buffer_size > 0) {
    // No blocks or no space, just return empty string
    out_buffer[0] = '\0';
  }

  return total_size;
}

/**
 * @brief Rank blocks by relevance to cursor position and optional query
 *
 * @param engine Context engine
 * @param cursor_file Current file path
 * @param cursor_line Current cursor line
 * @param cursor_column Current cursor column
 * @param query Optional query for semantic similarity (can be NULL)
 * @return bool True on success, false on failure
 */
bool context_engine_rank_blocks(ContextEngine *engine, const char *cursor_file,
                                uint32_t cursor_line, uint32_t cursor_column, const char *query) {
  if (!engine) {
    return false;
  }

  // Simple implementation for now to make the function available
  // Just set an error message if needed
  if (!engine->blocks) {
    if (engine->last_error) {
      free(engine->last_error);
    }

    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "No blocks available for ranking");
    engine->last_error = strdup(error_msg);
    return false;
  }

  // In a real implementation, we would rank blocks based on:
  // 1. Proximity to cursor position (if in same file)
  // 2. Semantic similarity to query (if provided)
  // 3. Other relevance metrics like recency, importance, etc.

  // Simple ranking based on position in the list for now
  size_t count = 0;
  InfoBlock *current = engine->blocks;

  // Count blocks first
  while (current != NULL) {
    count++;
    current = current->next;
  }

  // Assign rank scores
  if (count > 0) {
    size_t i = 0;
    current = engine->blocks;

    while (current != NULL) {
      // Higher rank for blocks that appear earlier in the list
      current->rank_score = (float)(count - i) / (float)count;
      i++;
      current = current->next;
    }
  }

  return true;
}

/**
 * @brief Update user focus for specific blocks
 *
 * @param engine Context engine
 * @param node_qualified_names Array of qualified names for nodes to focus on
 * @param num_nodes Number of nodes
 * @param focus_value Focus value (0.0 to 1.0)
 * @return size_t Number of blocks updated
 */
size_t context_engine_update_focus(ContextEngine *engine, const char **node_qualified_names,
                                   size_t num_nodes, float focus_value) {
  if (!engine || !node_qualified_names || num_nodes == 0) {
    return 0;
  }

  // Clamp focus value between 0.0 and 1.0
  float clamped_focus = focus_value;
  if (clamped_focus < 0.0f)
    clamped_focus = 0.0f;
  if (clamped_focus > 1.0f)
    clamped_focus = 1.0f;

  size_t num_updated = 0;

  // Iterate through all blocks and update focus for matching nodes
  for (InfoBlock *block = engine->blocks; block != NULL; block = block->next) {
    // Skip blocks without an AST node
    if (!block->ast_node) {
      continue;
    }

    // Check if the block's node matches any of the qualified names
    for (size_t i = 0; i < num_nodes; i++) {
      const char *qualified_name = node_qualified_names[i];

      // In a real implementation, we would match the qualified name against
      // the AST node's name/path. For now, we'll do a simple string check
      // assuming the ast_node has a name member we can access.
      // This is just a placeholder implementation to make linking work.

      // Update the block's focus value
      block->relevance.user_focus = clamped_focus;
      num_updated++;
      break; // Only need to match once per block
    }
  }

  return num_updated;
}

/**
 * @brief Initialize the context engine
 *
 * @param options Context options or NULL for defaults
 * @return ContextEngine* Initialized context engine or NULL on failure
 */
ContextEngine *context_engine_init(const ContextOptions *options) {
  // Allocate the context engine structure
  ContextEngine *engine = calloc(1, sizeof(ContextEngine));
  if (!engine) {
    return NULL;
  }

  // Copy options if provided, otherwise use defaults
  if (options) {
    memcpy(&engine->options, options, sizeof(ContextOptions));
  } else {
    // Set default options
    engine->options.max_tokens = 2048;
    engine->options.recency_weight = 0.5f;
    engine->options.proximity_weight = 0.3f;
    engine->options.similarity_weight = 0.7f;
    engine->options.reference_weight = 0.2f;
    engine->options.user_focus_weight = 1.0f;
    engine->options.preserve_structure = true;
    engine->options.prioritize_functions = true;
  }

  // Initialize other members
  engine->blocks = NULL;
  engine->num_blocks = 0;
  engine->total_tokens = 0;
  engine->compressed_tokens = 0;
  engine->last_error = NULL;
  engine->error_code = 0;

  return engine;
}

/**
 * @brief Clean up and free the context engine
 *
 * @param engine Context engine to free
 */
void context_engine_free(ContextEngine *engine) {
  if (!engine) {
    return;
  }

  // Free all info blocks in the linked list
  InfoBlock *current_block = engine->blocks;
  while (current_block) {
    InfoBlock *next_block = current_block->next;

    // Free the compressed content string if it exists
    if (current_block->compressed_content) {
      free(current_block->compressed_content);
    }

    // Note: In a real implementation, we would need to handle AST node cleanup
    // But we don't own those pointers, they're managed by the parser context

    free(current_block);
    current_block = next_block;
  }

  // Free the last error message if it exists
  if (engine->last_error) {
    free(engine->last_error);
  }

  // Free the engine itself
  free(engine);
}

/**
 * @brief Add all nodes from a parser context to the context engine
 *
 * @param engine Context engine
 * @param parser_ctx Parser context containing AST nodes
 * @return size_t Number of nodes added
 */
size_t context_engine_add_parser_context(ContextEngine *engine, const ParserContext *parser_ctx) {
  if (!engine || !parser_ctx) {
    return 0;
  }

  // This is a placeholder implementation
  // In a real implementation, we would iterate over all nodes in the parser context
  // and add them to the context engine using context_engine_add_node()

  // For now, just return 0 to indicate no nodes were added
  return 0;
}

// Note: The following functions were removed because they're already defined elsewhere:
// - context_engine_compress (in compressor.c)
// - context_engine_estimate_tokens (in token_budgeter.c)
// - context_engine_reset_compression (in compressor.c)
