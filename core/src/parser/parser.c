/**
 * @file parser.c
 * @brief Main parser module implementation
 *
 * This file implements the core parsing functionality, now significantly reduced
 * as most functionality has been moved to specialized components.
 */

#include "parser.h"
#include "ast_node.h"
#include "cst_node.h"
#include "memory_tracking.h"
#include "parser_context.h"
#include "parser_internal.h"
#include "query_processing.h"
#include "scopemux/ast.h"
#include "scopemux/tree_sitter_integration.h"

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _POSIX_C_SOURCE 200809L // For strdup on some systems

// Include the proper header for Tree-sitter integration functions

#include "../../core/include/scopemux/ts_resource_manager.h"
#include "scopemux/logging.h"
#include "scopemux/ts_internal.h"

/**
 * Parse a file and generate the AST and/or CST.
 */
bool parser_parse_file(ParserContext *ctx, const char *filename, LanguageType language) {
  if (!ctx || !filename) {
    log_error("Cannot parse file: %s", !ctx ? "context is NULL" : "filename is NULL");
    return false;
  }

  log_info("Parsing file: %s", filename);

  // Open and read the file
  FILE *f = fopen(filename, "rb");
  if (!f) {
    char error_message[256];
    snprintf(error_message, sizeof(error_message), "Failed to open file: %s", filename);
    parser_set_error(ctx, 1, error_message);
    return false;
  }

  // Get file size
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (file_size <= 0) {
    fclose(f);
    parser_set_error(ctx, 2, "File is empty or invalid");
    return false;
  }

  // Allocate buffer for file content
  char *content = (char *)memory_debug_malloc(file_size + 1, __FILE__, __LINE__, "file_content");
  if (!content) {
    fclose(f);
    parser_set_error(ctx, 3, "Memory allocation failed for file content");
    return false;
  }

  // Read file content
  size_t read_size = fread(content, 1, file_size, f);
  fclose(f);

  if (read_size != (size_t)file_size) {
    memory_debug_free(content, __FILE__, __LINE__);
    parser_set_error(ctx, 4, "Failed to read entire file");
    return false;
  }

  // Null-terminate the content
  content[file_size] = '\0';

  // Parse the content
  bool result = parser_parse_string(ctx, content, file_size, filename, language);

  // Free the content buffer
  memory_debug_free(content, __FILE__, __LINE__);

  return result;
}

/**
 * Parse a string and generate the AST and/or CST.
 */
bool parser_parse_string(ParserContext *ctx, const char *content, size_t content_length,
                         const char *filename, LanguageType language) {
  if (!ctx || !content) {
    log_error("Cannot parse string: %s", !ctx ? "context is NULL" : "content is NULL");
    return false;
  }

  // Clear any existing parser state
  parser_clear(ctx);

  // Store the source code and filename
  ctx->source_code = strdup(content);
  if (!ctx->source_code) {
    parser_set_error(ctx, 5, "Memory allocation failed for source code");
    return false;
  }
  ctx->source_code_length = content_length;

  if (filename) {
    ctx->filename = strdup(filename);
    if (!ctx->filename) {
      parser_set_error(ctx, 6, "Memory allocation failed for filename");
      return false;
    }
  }

  // Detect language if not specified
  if (language == LANG_UNKNOWN) {
    language = parser_detect_language(filename, content, content_length);
    if (language == LANG_UNKNOWN) {
      parser_set_error(ctx, 7, "Failed to detect language");
      return false;
    }
  }
  ctx->language = language;

  // Set up signal handling for crash recovery
  signal(SIGSEGV, segfault_handler); // Use the handler declared in parser_internal.h

  // Use setjmp/longjmp to recover from parser crashes
  crash_occurred = 0;
  if (setjmp(parse_crash_recovery) != 0) {
    // Restore original signal handler
    signal(SIGSEGV, SIG_DFL); // Restore default handler

    parser_set_error(ctx, 8, "Parser crashed during parsing");
    log_error("Recovered from parser crash");
    return false;
  }

  log_error("===== PARSER_PARSE_STRING: STARTING PARSER INITIALIZATION =====");
  log_error("Language type: %d (1=C, 2=CPP, 3=Python, 4=JavaScript, 5=TypeScript)", language);

  // Initialize the Tree-sitter parser for the specified language
  bool init_result = ts_init_parser(ctx, language);
  log_error("ts_init_parser returned: %s", init_result ? "TRUE" : "FALSE");

  if (!init_result) {
    log_error("Failed to initialize Tree-sitter parser");
    return false;
  }

  // Verify the parser language was actually set
  TSParser *ts_parser = ctx->ts_parser;
  const TSLanguage *current_lang = ts_parser_language(ts_parser);
  log_error("After initialization: Tree-sitter parser=%p, language=%p", (void *)ts_parser,
            (void *)current_lang);

  if (!current_lang) {
    parser_set_error(ctx, 8, "Tree-sitter parser language not set");
    return false;
  }

  log_error("===== PARSER_PARSE_STRING: PARSER INITIALIZATION COMPLETED =====");

  log_debug("Successfully initialized Tree-sitter parser for language %d", language);

  // Use Tree-sitter to generate a parse tree
  if (!ts_parser) {
    parser_set_error(ctx, 8, "Tree-sitter parser not initialized");
    return false;
  }

  // Log debugging information
  if (ctx->log_level <= LOG_DEBUG) {
    log_debug("Parsing %s with Tree-sitter, content length: %zu, language: %d", filename,
              content_length, language);
    log_debug("Tree-sitter parser at %p, language object at %p", (void *)ts_parser,
              (void *)ts_parser_language(ts_parser));
  }

  // Verify we've already checked that the language is properly set above, no need to check again

  // Parse the content
  TSTree *ts_tree = ts_parser_parse_string(ts_parser, NULL, content, (uint32_t)content_length);
  if (!ts_tree) {
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "Tree-sitter parsing failed for language %d", language);
    parser_set_error(ctx, 9, error_msg);
    log_error("Tree-sitter parsing failed for %s (language %d)", filename, language);
    return false;
  }

  // Restore original signal handler
  signal(SIGSEGV, SIG_DFL);

  // Generate CST if requested
  if (ctx->mode == PARSE_CST || ctx->mode == PARSE_BOTH) {
    TSNode root_node = ts_tree_root_node(ts_tree);
    CSTNode *cst_root = ts_tree_to_cst(root_node, ctx);
    parser_set_cst_root(ctx, cst_root);

    if (!cst_root) {
      parser_set_error(ctx, 10, "CST generation failed");
      return false;
    }
  }

  // Generate AST if requested
  if (ctx->mode == PARSE_AST || ctx->mode == PARSE_BOTH) {
    // Create root AST node
    ASTNode *root = ast_node_create(NODE_ROOT, filename ? filename : "unknown", NULL,
                                    (SourceRange){{0, 0, 0}, {0, 0, 0}});
    if (!root) {
      parser_set_error(ctx, 11, "Failed to create AST root node");
      return false;
    }

    // Add to tracking
    parser_add_ast_node(ctx, root);

    // Execute queries to build the AST using Tree-sitter root node
    TSNode root_node = ts_tree_root_node(ts_tree);
    ASTNode *ast_root = ts_tree_to_ast(root_node, ctx);

    // Add the AST root to the context if it was created successfully
    if (ast_root) {
      parser_add_ast_node(ctx, ast_root);

      // CRITICAL: Set the AST root node in the parser context
      // This ensures the root is accessible via ctx->ast_root in tests
      ctx->ast_root = ast_root;

      if (ctx->log_level <= LOG_DEBUG) {
        log_debug("AST root set in parser context, node count: %zu", ast_root->num_children);
      }
    } else {
      log_error("AST generation failed - falling back to initial root node");
      // Use the basic root node as fallback
      ctx->ast_root = root;
      if (ctx->log_level <= LOG_WARNING) {
        log_warning("Using fallback AST root with %zu children", root->num_children);
      }
    }
  }

  // Free the Tree-sitter tree now that we've processed it
  ts_tree_delete(ts_tree);

  ctx->error_code = 0; // No error
  return true;
}

/**
 * Get the AST node for a specific entity.
 */
const ASTNode *parser_get_ast_node(const ParserContext *ctx, const char *qualified_name) {
  if (!ctx || !qualified_name || ctx->num_ast_nodes == 0) {
    return NULL;
  }

  // Simple linear search through all AST nodes
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    ASTNode *node = ctx->all_ast_nodes[i];
    if (node && node->qualified_name && strcmp(node->qualified_name, qualified_name) == 0) {
      return node;
    }
  }

  return NULL;
}

/**
 * Get all AST nodes of a specific type.
 */
size_t parser_get_ast_nodes_by_type(const ParserContext *ctx, ASTNodeType type,
                                    const ASTNode **out_nodes, size_t max_nodes) {
  if (!ctx || ctx->num_ast_nodes == 0) {
    return 0;
  }

  size_t found = 0;

  // Simple linear search through all AST nodes
  for (size_t i = 0; i < ctx->num_ast_nodes && (out_nodes == NULL || found < max_nodes); i++) {
    ASTNode *node = ctx->all_ast_nodes[i];
    if (node && node->type == type) {
      if (out_nodes != NULL) {
        out_nodes[found] = node;
      }
      found++;
    }
  }

  return found;
}

/**
 * Get the root node of the Abstract Syntax Tree (AST).
 */
const ASTNode *parser_get_ast_root(const ParserContext *ctx) {
  if (!ctx) {
    return NULL;
  }

  // Find the root node by checking for NODE_ROOT type or parent == NULL
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    if (ctx->all_ast_nodes[i] && ctx->all_ast_nodes[i]->type == NODE_ROOT) {
      return ctx->all_ast_nodes[i];
    }
  }

  // Fallback to find a node without a parent
  for (size_t i = 0; i < ctx->num_ast_nodes; i++) {
    if (ctx->all_ast_nodes[i] && ctx->all_ast_nodes[i]->parent == NULL) {
      return ctx->all_ast_nodes[i];
    }
  }

  return NULL; // No root node found
}

/**
 * Get the root node of the Concrete Syntax Tree (CST).
 */
const CSTNode *parser_get_cst_root(const ParserContext *ctx) {
  if (!ctx) {
    return NULL;
  }
  return ctx->cst_root;
}

// === Tree-sitter integration public API ===
bool ts_init_parser(ParserContext *ctx, LanguageType language) {
  if (!ctx) {
    log_error("NULL context passed to ts_init_parser");
    return false;
  }
  log_debug("ts_init_parser facade called, delegating to ts_init_parser_impl for language %d",
            language);
  return ts_init_parser_impl(ctx, language);
}

ASTNode *ts_tree_to_ast(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx) {
    if (ctx) {
      parser_set_error(ctx, -1, "Invalid arguments to ts_tree_to_ast");
    }
    return NULL;
  }
  return ts_tree_to_ast_impl(root_node, ctx);
}

CSTNode *ts_tree_to_cst(TSNode root_node, ParserContext *ctx) {
  if (ts_node_is_null(root_node) || !ctx || !ctx->source_code) {
    parser_set_error(ctx, -1, "Invalid context for CST generation");
    return NULL;
  }
  return ts_tree_to_cst_impl(root_node, ctx);
}
