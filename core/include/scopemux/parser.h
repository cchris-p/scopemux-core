/**
 * @file parser.h
 * @brief Parser and IR Generator interface for ScopeMux
 *
 * This module is responsible for parsing source code using Tree-sitter
 * and generating a compact, binary Intermediate Representation (IR) for
 * each function/class with metadata such as function signatures, line/byte
 * ranges, control-flow primitives, docstrings, comments, and call expressions.
 */

#ifndef SCOPEMUX_PARSER_H
#define SCOPEMUX_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Representation of a source location
 */
typedef struct {
  uint32_t line;   // 0-indexed line number
  uint32_t column; // 0-indexed column number
  uint32_t offset; // Byte offset from the start of the file
} SourceLocation;

/**
 * @brief Representation of a source range
 */
typedef struct {
  SourceLocation start;
  SourceLocation end;
} SourceRange;

// Forward declaration of QueryManager
typedef struct QueryManager QueryManager;

/**
 * @brief Supported programming languages
 */
typedef enum {
  LANG_UNKNOWN = 0,
  LANG_C,
  LANG_CPP,
  LANG_PYTHON,
  LANG_JAVASCRIPT,
  LANG_TYPESCRIPT,
  // Add more languages as needed
} LanguageType;

/**
 * @brief Types of AST nodes that we care about
 */
typedef enum {
  NODE_UNKNOWN = 0,
  NODE_FUNCTION,
  NODE_METHOD,
  NODE_CLASS,
  NODE_STRUCT,
  NODE_ENUM,
  NODE_INTERFACE,
  NODE_NAMESPACE,
  NODE_MODULE,
  NODE_COMMENT,
  NODE_DOCSTRING,
  // C-specific node types
  NODE_UNION,
  NODE_TYPEDEF,
  NODE_INCLUDE,
  NODE_MACRO,
  NODE_VARIABLE,
  // Add more node types as needed
} ASTNodeType;

typedef enum { PARSE_AST, PARSE_CST } ParseMode;

/**
 * @brief Represents a generic node in the Concrete Syntax Tree (CST).
 */
typedef struct CSTNode {
  const char *type; ///< The syntax type of the node (e.g., "function_definition", "identifier").
  char *content;    ///< The source code content of the node.
  SourceRange range;
  struct CSTNode **children; ///< Array of child nodes.
  unsigned int children_count;
} CSTNode;

// CST Node lifecycle functions
CSTNode *cst_node_new(const char *type, char *content);
void cst_node_free(CSTNode *node);
bool cst_node_add_child(CSTNode *parent, CSTNode *child);

/**
 * @brief AST node representing a parsed semantic entity.
 *
 * All string fields (name, signature, etc.) are owned by this struct
 * and will be freed when the AST is destroyed via parser_free().
 *
 * This is the core data structure for representing parsed code entities.
 * It contains metadata about the entity, its location, and references to
 * related entities.
 */
typedef struct ASTNode {
  ASTNodeType type;     // Type of the node
  char *name;           // Name of the entity
  char *qualified_name; // Fully qualified name (e.g., namespace::class::method)
  SourceRange range;    // Source code range
  char *signature;      // Function/method signature if applicable
  char *docstring;      // Associated documentation
  char *raw_content;    // Raw source code content

  // Parent-child relationships
  struct ASTNode *parent;    // Parent node (e.g., class for a method)
  struct ASTNode **children; // Child nodes (e.g., methods for a class)
  size_t num_children;       // Number of children
  size_t children_capacity;  // Capacity of children array

  // References and dependencies
  struct ASTNode **references; // Nodes referenced by this node
  size_t num_references;       // Number of references
  size_t references_capacity;  // Capacity of references array

  // For future expansion
  void *additional_data; // Language-specific or analysis data
} ASTNode;

// AST Node lifecycle functions
ASTNode *ast_node_new(ASTNodeType type, const char *name);
void ast_node_free(ASTNode *node);
bool ast_node_add_child(ASTNode *parent, ASTNode *child);

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
  LanguageType language;     // Detected language

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

  // Error handling
  char *last_error; // Last error message
  int error_code;   // Error code
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
 * @return LanguageType Detected language
 */
LanguageType parser_detect_language(const char *filename, const char *content,
                                    size_t content_length);

/**
 * @brief Parse a file and generate the IR
 *
 * @param ctx Parser context
 * @param filename Path to the file
 * @param language Optional language hint (LANG_UNKNOWN to auto-detect)
 * @return bool True on success, false on failure
 */
bool parser_parse_file(ParserContext *ctx, const char *filename, LanguageType language);

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
                         const char *filename, LanguageType language);

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
 * @brief Get the root of the Concrete Syntax Tree (CST).
 *
 * This function should only be called after a successful parse in PARSE_CST mode.
 *
 * @param ctx Parser context.
 * @return const CSTNode* Root of the CST or NULL if not available.
 */
const CSTNode *parser_get_cst_root(const ParserContext *ctx);

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

/**
 * @brief Create a new AST node
 *
 * @param type Node type
 * @param name Node name
 * @param qualified_name Fully qualified name
 * @param range Source range
 * @return ASTNode* Created node or NULL on failure
 */
ASTNode *ast_node_create(ASTNodeType type, const char *name, const char *qualified_name,
                         SourceRange range);

/**
 * @brief Free an AST node and all its resources
 *
 * @param node Node to free
 */
void ast_node_free(ASTNode *node);

/**
 * @brief Add a child node to a parent AST node
 *
 * @param parent Parent node
 * @param child Child node
 * @return bool True on success, false on failure
 */
bool ast_node_add_child(ASTNode *parent, ASTNode *child);

/**
 * @brief Add a reference from one AST node to another
 *
 * @param from Source node
 * @param to Target node
 * @return bool True on success, false on failure
 */
bool ast_node_add_reference(ASTNode *from, ASTNode *to);

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

#endif /* SCOPEMUX_PARSER_H */
