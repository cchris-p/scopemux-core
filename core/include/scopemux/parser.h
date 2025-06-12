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
  // Add more node types as needed
} NodeType;

typedef enum { PARSE_AST, PARSE_CST } ParseMode;

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

/**
 * @brief IR node representing a parsed entity
 *
 * This is the core data structure for representing parsed code entities.
 * It contains metadata about the entity, its location, and references to
 * related entities.
 */
typedef struct IRNode {
  NodeType type;        // Type of the node
  char *name;           // Name of the entity
  char *qualified_name; // Fully qualified name (e.g., namespace::class::method)
  SourceRange range;    // Source code range
  char *signature;      // Function/method signature if applicable
  char *docstring;      // Associated documentation
  char *raw_content;    // Raw source code content

  // Parent-child relationships
  struct IRNode *parent;    // Parent node (e.g., class for a method)
  struct IRNode **children; // Child nodes (e.g., methods for a class)
  size_t num_children;      // Number of children
  size_t children_capacity; // Capacity of children array

  // References and dependencies
  struct IRNode **references; // Nodes referenced by this node
  size_t num_references;      // Number of references
  size_t references_capacity; // Capacity of references array

  // For future expansion
  void *additional_data; // Language-specific or analysis data
} IRNode;

/**
 * @brief Context for the parser
 *
 * This structure holds the state of the parser, including the
 * Tree-sitter parser, parsed file information, and the resulting IR.
 */
typedef struct {
  void *ts_parser;           // Tree-sitter parser (void* to avoid dependency)
  ParseMode mode;            // Type of parse mode (e.g., AST or CST)
  char *filename;            // Current file being parsed
  char *source_code;         // Source code content
  size_t source_code_length; // Length of source code
  LanguageType language;     // Detected language

  IRNode *root_node;     // Root node of the IR
  IRNode **all_nodes;    // Flat array of all nodes for easy access
  size_t num_nodes;      // Number of nodes
  size_t nodes_capacity; // Capacity of nodes array

  // Error handling
  char *last_error; // Last error message
  int error_code;   // Error code
} ParserContext;

/**
 * @brief Initialize the parser
 *
 * @return ParserContext* Initialized parser context or NULL on failure
 */
ParserContext *parser_init(void);

/**
 * @brief Clean up and free the parser context
 *
 * @param ctx Parser context to free
 */
void parser_free(ParserContext *ctx);

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
bool parser_parse_string(ParserContext *ctx, const char *content, size_t content_length,
                         const char *filename, LanguageType language);

/**
 * @brief Get the last error message
 *
 * @param ctx Parser context
 * @return const char* Error message or NULL if no error
 */
const char *parser_get_last_error(const ParserContext *ctx);

/**
 * @brief Get the IR node for a specific entity
 *
 * @param ctx Parser context
 * @param qualified_name Fully qualified name of the entity
 * @return const IRNode* Found node or NULL if not found
 */
const IRNode *parser_get_node(const ParserContext *ctx, const char *qualified_name);

/**
 * @brief Get all nodes of a specific type
 *
 * @param ctx Parser context
 * @param type Node type to filter by
 * @param out_nodes Output array of nodes (can be NULL to just get the count)
 * @param max_nodes Maximum number of nodes to return
 * @return size_t Number of nodes found
 */
size_t parser_get_nodes_by_type(const ParserContext *ctx, NodeType type, const IRNode **out_nodes,
                                size_t max_nodes);

/**
 * @brief Create a new IR node
 *
 * @param type Node type
 * @param name Node name
 * @param qualified_name Fully qualified name
 * @param range Source range
 * @return IRNode* Created node or NULL on failure
 */
IRNode *ir_node_create(NodeType type, const char *name, const char *qualified_name,
                       SourceRange range);

/**
 * @brief Free an IR node and all its resources
 *
 * @param node Node to free
 */
void ir_node_free(IRNode *node);

/**
 * @brief Add a child node to a parent node
 *
 * @param parent Parent node
 * @param child Child node
 * @return bool True on success, false on failure
 */
bool ir_node_add_child(IRNode *parent, IRNode *child);

/**
 * @brief Add a reference from one node to another
 *
 * @param from Source node
 * @param to Target node
 * @return bool True on success, false on failure
 */
bool ir_node_add_reference(IRNode *from, IRNode *to);

#endif /* SCOPEMUX_PARSER_H */
