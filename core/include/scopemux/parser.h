/**
 * @file parser.h
 * @brief Parser and IR Generator interface for ScopeMux
 *
 * This module is responsible for parsing source code using Tree-sitter
 * and generating a compact, binary Intermediate Representation (IR) for
 * each function/class with metadata such as function signatures, line/byte
 * ranges, control-flow primitives, docstrings, comments, and call expressions.
 */

/*
 * NOTE FOR MAINTAINERS:
 * This is the public API header for the parser module. It must only contain declarations
 * intended for use by external modules, tests, or users of the ScopeMux library.
 *
 * There is also an internal parser.h (or parser_internal.h) in src/parser/ which is for
 * private implementation details. Only this file (in include/) should be included by code
 * outside the parser module. Do not expose internal-only types or functions here.
 */
#ifndef SCOPEMUX_PARSER_H
#define SCOPEMUX_PARSER_H

#include "ast.h"
#include "language.h"
#include "logging.h"
#include "source_range.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct ASTNode;
typedef struct ASTNode ASTNode;

// Forward declaration of QueryManager
typedef struct QueryManager QueryManager;

/* Language enum is now defined in scopemux/language.h. */

typedef enum { PARSE_AST, PARSE_CST, PARSE_BOTH } ParseMode;

// Forward declaration of CSTNode
struct CSTNode;
typedef struct CSTNode CSTNode;

// CST Node lifecycle functions
CSTNode *cst_node_new(const char *type, char *content);
void cst_node_free(CSTNode *node);
bool cst_node_add_child(CSTNode *parent, CSTNode *child);

/**
 * @brief Creates a deep copy of a CST node and all its children
 *
 * @param node The node to copy
 * @return CSTNode* A new allocated copy or NULL on failure
 */
CSTNode *cst_node_copy_deep(const CSTNode *node);

/**
 * @brief AST node representing a parsed semantic entity in a language-agnostic way.
 * @see The ASTNode struct is defined in scopemux/ast.h
 */

/**
 * @brief Context for the parser
 *
 * This structure holds the state of the parser, including the
 * Tree-sitter parser, parsed file information, and the resulting IR.
 */
typedef struct ParserContext {
  void *ts_parser;           // Tree-sitter parser (void* to avoid dependency)
  QueryManager *q_manager;   // Query manager for .scm files
  ParseMode mode;            // Type of parse mode (e.g., AST or CST)
  char *filename;            // Current file being parsed
  char *source_code;         // Source code content
  size_t source_code_length; // Length of source code
  Language language;         // Detected language

  /**
   * @brief Root node of the Abstract Syntax Tree.
   * Populated when mode is PARSE_AST.
   */
  ASTNode *ast_root;         // Root node of the AST
  ASTNode **all_ast_nodes;   // Flat array of all AST nodes for easy access
  size_t num_ast_nodes;      // Number of nodes in the AST
  size_t ast_nodes_capacity; // Capacity of the AST nodes array

  /**
   * @brief Root of the Concrete Syntax Tree.
   * Populated when mode is PARSE_CST. May be NULL otherwise.
   */
  CSTNode *cst_root;

  /**
   * @brief Dependencies tracking.
   * Array of parser contexts that this context depends on.
   */
  struct ParserContext **dependencies; // Array of dependencies
  size_t num_dependencies;             // Number of dependencies
  size_t dependencies_capacity;        // Capacity of dependencies array

  // Error handling
  char *last_error; // Last error message
  int error_code;   // Error code

  /**
   * @brief Logging level for this parser context (see logging.h)
   * Set to LOG_ERROR, LOG_DEBUG, etc. to control output.
   */
  LogLevel log_level;
} ParserContext;

/**
 * @brief Adds an AST node to the parser context's tracking list
 *
 * @param ctx Parser context
 * @param node Node to add to tracking
 * @return bool True on success, false on failure
 */
bool parser_add_ast_node(ParserContext *ctx, ASTNode *node);

/**
 * @brief Initialize the parser
 *
 * @return ParserContext* Initialized parser context or NULL on failure
 */
ParserContext *parser_init(void);

/**
 * @brief Free a parser context and all associated resources
 *
 * @param ctx The parser context to free
 */
void parser_free(ParserContext *ctx);

/**
 * @brief Free a parser context and all associated resources (alias for parser_free)
 *
 * @param ctx The parser context to free
 */
void parser_context_free(ParserContext *ctx);

/**
 * @brief Add an AST node to the parser context's tracking list (alias for parser_add_ast_node)
 *
 * @param ctx Parser context
 * @param node Node to add to tracking
 * @return bool True on success, false on failure
 */
bool parser_context_add_ast(ParserContext *ctx, ASTNode *node);

/**
 * @brief Add an AST node to the parser context with associated filename
 *
 * @param ctx Parser context
 * @param node Node to add to tracking
 * @param filename The filename associated with the AST node
 * @return bool True on success, false on failure
 */
bool parser_context_add_ast_with_filename(ParserContext *ctx, ASTNode *node, const char *filename);

/**
 * @brief Add a dependency relationship between two parser contexts
 *
 * @param source The source parser context that depends on the target
 * @param target The target parser context that is depended upon
 * @return bool True on success, false on failure
 */
bool parser_context_add_dependency(ParserContext *source, ParserContext *target);

/**
 * @brief Sets the parsing mode for the context.
 *
 * The default mode is PARSE_AST.
 *
 * @param ctx The parser context.
 * @param mode The desired parse mode (PARSE_AST or PARSE_CST).
 */
void parser_set_mode(ParserContext *ctx, ParseMode mode);

/**
 * @brief Clean up and free the parser context
 *
 * @param ctx Parser context to free
 */
void parser_free(ParserContext *ctx);

/**
 * @brief Clears the results of the last parse (AST, source code, errors)
 * from the context, preparing it for a new parsing operation.
 *
 * @param ctx The parser context to clear.
 */
void parser_clear(ParserContext *ctx);

/**
 * @brief Detect the language of a file based on its extension and content
 *
 * @param filename Path to the file
 * @param content File content (can be NULL, in which case the file will be read)
 * @param content_length Length of the content (ignored if content is NULL)
 * @return Language Detected language
 */
Language parser_detect_language(const char *filename, const char *content, size_t content_length);

/**
 * @brief Parse a file and generate the IR
 *
 * @param ctx Parser context
 * @param filename Path to the file
 * @param language Optional language hint (LANG_UNKNOWN to auto-detect)
 * @return bool True on success, false on failure
 */
bool parser_parse_file(ParserContext *ctx, const char *filename, Language language);

/**
 * @brief Parse a string and generate the IR
 *
 * @param ctx Parser context
 * @param content Source code content
 * @param content_length Length of the content
 * @param filename Optional filename for error reporting and language detection
 * @param language Optional language hint (LANG_UNKNOWN to auto-detect)
 * @return bool True on success, false on failure
 */
bool parser_parse_string(ParserContext *ctx, const char *const content, size_t content_length,
                         const char *filename, Language language);

/**
 * @brief Get the AST root node from a parser context
 *
 * @param ctx Parser context
 * @return const ASTNode* Root AST node or NULL if not available
 */
const ASTNode *parser_context_get_ast(const ParserContext *ctx);

/**
 * @brief Get the last error message
 *
 * @param ctx Parser context
 * @return const char* Error message or NULL if no error
 */
const char *parser_get_last_error(const ParserContext *ctx);

/**
 * @brief Set an error message and code in the parser context
 *
 * @param ctx Parser context
 * @param code Error code
 * @param message Error message
 */
void parser_set_error(ParserContext *ctx, int code, const char *message);

/**
 * @brief Get the AST node for a specific entity
 *
 * @param ctx Parser context
 * @param qualified_name Fully qualified name of the entity
 * @return const ASTNode* Found node or NULL if not found
 */
const ASTNode *parser_get_ast_node(const ParserContext *ctx, const char *qualified_name);

/**
 * @brief Get the root of the Abstract Syntax Tree (AST).
 *
 * This function should only be called after a successful parse in PARSE_AST mode.
 *
 * @param ctx Parser context.
 * @return const ASTNode* Root of the AST or NULL if not available.
 */
const ASTNode *parser_get_ast_root(const ParserContext *ctx);

/**
 * @brief Get the root of the Concrete Syntax Tree (CST).
 *
 * This function should only be called after a successful parse in PARSE_CST mode.
 *
 * @param ctx Parser context.
 * @return const CSTNode* Root of the CST or NULL if not available.
 */
const CSTNode *parser_get_cst_root(const ParserContext *ctx);

/**
 * @brief Set the root of the Concrete Syntax Tree (CST).
 *
 * This function allows explicitly setting the CST root, primarily used
 * for memory management when freeing CST nodes.
 *
 * @param ctx Parser context.
 * @param root The new CST root node (can be NULL to clear).
 */
void parser_set_cst_root(ParserContext *ctx, CSTNode *root);

/**
 * @brief Get all AST nodes of a specific type
 *
 * @param ctx Parser context
 * @param type Node type to filter by
 * @param out_nodes Output array of nodes (can be NULL to just get the count)
 * @param max_nodes Maximum number of nodes to return
 * @return size_t Number of nodes found
 */
size_t parser_get_ast_nodes_by_type(const ParserContext *ctx, ASTNodeType type,
                                    const ASTNode **out_nodes, size_t max_nodes);

// AST Node property functions are declared in scopemux/ast.h

/**
 * @brief Create a new CST node.
 *
 * @param type Node type string (from Tree-sitter, not copied).
 * @param content Source code content of the node (ownership transferred).
 * @param range Source range of the node.
 * @return CSTNode* Created node or NULL on failure.
 */
CSTNode *cst_node_create(const char *type, char *content, SourceRange range);

/**
 * @brief Free a CST node and all its children recursively.
 *
 * @param node The CST node to free.
 */
void cst_node_free(CSTNode *node);

/**
 * @brief Add a child node to a parent CST node.
 *
 * @param parent Parent node.
 * @param child Child node.
 * @return bool True on success, false on failure.
 */
bool cst_node_add_child(CSTNode *parent, CSTNode *child);

/**
 * @brief Unified status codes for parser and processor helpers
 */
typedef enum {
  PARSE_OK = 0,   ///< Operation succeeded
  PARSE_SKIP = 1, ///< Skip this entity (not an error)
  PARSE_ERROR = 2 ///< Error occurred, check context for details
} ParseStatus;

/**
 * @brief Logging level for parser and processor modules
 *
 * This field allows unified control of logging output per context.
 * Set to LOG_ERROR, LOG_DEBUG, etc. (see logging.h)
 */
// Add this to ParserContext:
//   LogLevel log_level;
// Example usage:
//   ctx->log_level = LOG_DEBUG;

#endif /* SCOPEMUX_PARSER_H */
