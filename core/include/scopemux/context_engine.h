/**
 * @file context_engine.h
 * @brief Tiered Context Engine (Compressor/Expander) interface for ScopeMux
 *
 * This module is responsible for managing a pool of InfoBlocks (functions, classes, doc chunks),
 * estimating token costs, ranking blocks by relevance, and applying compression
 * techniques to fit within token budgets for efficient context management.
 */

#ifndef SCOPEMUX_CONTEXT_ENGINE_H
#define SCOPEMUX_CONTEXT_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "parser.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Compression level for InfoBlocks
 */
typedef enum {
  COMPRESSION_NONE = 0,      // No compression, full text
  COMPRESSION_LIGHT,         // Basic compression, remove unnecessary whitespace
  COMPRESSION_MEDIUM,        // Medium compression, shorten variable names
  COMPRESSION_HEAVY,         // Heavy compression, remove comments and simplify
  COMPRESSION_SIGNATURE_ONLY // Only keep function/class signatures
} CompressionLevel;

/**
 * @brief Relevance metrics for ranking InfoBlocks
 */
typedef struct {
  float recency;             // How recently was this block edited/viewed
  float cursor_proximity;    // How close is this to the current cursor position
  float semantic_similarity; // Semantic similarity to the query or current context
  float reference_count;     // How many times is this referenced by other blocks
  float user_focus;          // User explicitly focused on this block
} RelevanceMetrics;

/**
 * @brief Information Block representing a unit of code or documentation
 */
struct InfoBlock {
  struct ASTNode *ast_node;   // Reference to the AST node
  char *compressed_content;   // Compressed content
  size_t original_tokens;     // Original token count
  size_t compressed_tokens;   // Compressed token count
  CompressionLevel level;     // Current compression level
  RelevanceMetrics relevance; // Relevance metrics for ranking
  float rank_score;           // Overall ranking score
  struct InfoBlock *next;     // Next block in a linked list
};

typedef struct InfoBlock InfoBlock;

/**
 * @brief Context management options
 */
typedef struct {
  size_t max_tokens;         // Maximum token budget
  float recency_weight;      // Weight for recency in ranking
  float proximity_weight;    // Weight for cursor proximity in ranking
  float similarity_weight;   // Weight for semantic similarity in ranking
  float reference_weight;    // Weight for reference count in ranking
  float user_focus_weight;   // Weight for user focus in ranking
  bool preserve_structure;   // Try to preserve code structure during compression
  bool prioritize_functions; // Prioritize functions over other types
} ContextOptions;

/**
 * @brief Context Engine state
 */
struct ContextEngine {
  struct InfoBlock *blocks; // List of information blocks
  size_t num_blocks;        // Number of blocks
  size_t total_tokens;      // Total tokens across all blocks
  size_t compressed_tokens; // Total tokens after compression
  ContextOptions options;   // Context options

  // Token estimation
  void *token_estimator; // Opaque pointer to token estimation implementation

  // Error handling
  char *last_error; // Last error message
  int error_code;   // Error code
};

typedef struct ContextEngine ContextEngine;

/**
 * @brief Initialize the context engine
 *
 * @param options Context options or NULL for defaults
 * @return ContextEngine* Initialized context engine or NULL on failure
 */
ContextEngine *context_engine_init(const ContextOptions *options);

/**
 * @brief Clean up and free the context engine
 *
 * @param engine Context engine to free
 */
void context_engine_free(ContextEngine *engine);

/**
 * @brief Add an AST node to the context engine
 *
 * @param engine Context engine
 * @param node AST node to add
 * @return InfoBlock* Created info block or NULL on failure
 */
InfoBlock *context_engine_add_node(ContextEngine *engine, const ASTNode *node);

/**
 * @brief Add all nodes from a parser context to the context engine
 *
 * @param engine Context engine
 * @param parser_ctx Parser context
 * @return size_t Number of blocks added
 */
size_t context_engine_add_parser_context(ContextEngine *engine, const ParserContext *parser_ctx);

/**
 * @brief Rank blocks by relevance
 *
 * @param engine Context engine
 * @param cursor_file Current file path
 * @param cursor_line Current cursor line
 * @param cursor_column Current cursor column
 * @param query Optional query for semantic similarity (can be NULL)
 * @return bool True on success, false on failure
 */
bool context_engine_rank_blocks(ContextEngine *engine, const char *cursor_file,
                                uint32_t cursor_line, uint32_t cursor_column, const char *query);

/**
 * @brief Apply compression to fit within token budget
 *
 * @param engine Context engine
 * @return bool True on success, false on failure
 */
bool context_engine_compress(ContextEngine *engine);

/**
 * @brief Get compressed context as a string
 *
 * @param engine Context engine
 * @param out_buffer Output buffer (can be NULL to get required size)
 * @param buffer_size Size of output buffer
 * @return size_t Size of the context (excluding null terminator)
 */
size_t context_engine_get_context(const ContextEngine *engine, char *out_buffer,
                                  size_t buffer_size);

/**
 * @brief Estimate the number of tokens in a string
 *
 * @param engine Context engine
 * @param text Text to estimate
 * @param text_length Length of text
 * @return size_t Estimated token count
 */
size_t context_engine_estimate_tokens(const ContextEngine *engine, const char *text,
                                      size_t text_length);

/**
 * @brief Apply a specific compression level to a block
 *
 * @param engine Context engine
 * @param block Info block to compress
 * @param level Compression level
 * @return bool True on success, false on failure
 */
bool context_engine_compress_block(ContextEngine *engine, InfoBlock *block, CompressionLevel level);

/**
 * @brief Get the last error message
 *
 * @param engine Context engine
 * @return const char* Error message or NULL if no error
 */
const char *context_engine_get_last_error(const ContextEngine *engine);

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
                                   size_t num_nodes, float focus_value);

/**
 * @brief Reset all compression to COMPRESSION_NONE
 *
 * @param engine Context engine
 */
void context_engine_reset_compression(ContextEngine *engine);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_CONTEXT_ENGINE_H */
