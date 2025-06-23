/**
 * @file ts_init.c
 * @brief Implementation of Tree-sitter parser initialization
 * 
 * This module handles the initialization of Tree-sitter parsers for
 * different language types. It follows the Single Responsibility Principle
 * by focusing only on parser initialization and cleanup.
 * 
 * NOTE: The public interface is provided by tree_sitter_integration.c,
 * which calls into this module via ts_init_parser_impl.
 */

#include "../../core/include/scopemux/tree_sitter_integration.h"
#include "../../core/include/scopemux/adapters/adapter_registry.h"
#include "../../core/include/scopemux/adapters/language_adapter.h"
#include "../../core/include/scopemux/logging.h"
#include "../../core/include/scopemux/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for Tree-sitter language functions from vendor library
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

/**
 * @brief Builds a queries directory path for the given language
 * 
 * @param language Language type
 * @return char* Heap-allocated path string (caller must free)
 */
char *build_queries_dir(LanguageType language) {
    const char *base = "/home/matrillo/apps/scopemux/queries";
    const char *subdir = "unknown";
    switch (language) {
        case LANG_C: subdir = "c"; break;
        case LANG_CPP: subdir = "cpp"; break;
        case LANG_PYTHON: subdir = "python"; break;
        case LANG_JAVASCRIPT: subdir = "javascript"; break;
        case LANG_TYPESCRIPT: subdir = "typescript"; break;
        default: break;
    }
    size_t len = strlen(base) + 1 + strlen(subdir) + 1;
    char *result = (char *)malloc(len);
    if (!result) return NULL;
    snprintf(result, len, "%s/%s", base, subdir);
    return result;
}

/**
 * @brief Implementation of Tree-sitter parser initialization
 * 
 * This function is called by the facade ts_init_parser function in
 * tree_sitter_integration.c. It handles the actual initialization logic
 * for the Tree-sitter parser.
 * 
 * @param ctx The parser context to initialize
 * @param language The language to initialize the parser for
 * @return bool True on success, false on failure
 */
bool ts_init_parser_impl(ParserContext *ctx, LanguageType language) {
    if (!ctx) {
        log_error("NULL context passed to ts_init_parser");
        return false;
    }

    // Check if parser already exists
    if (ctx->ts_parser) {
        // Parser exists, but check if language matches
        if (ctx->language_type == language) {
            // Parser already initialized for this language
            if (ctx->log_level <= LOG_DEBUG) {
                log_debug("Parser already initialized for language %d", language);
            }
            return true;
        } else {
            // Different language, clean up old parser
            ts_parser_delete(ctx->ts_parser);
            ctx->ts_parser = NULL;
        }
    }

    // Create new parser
    ctx->ts_parser = ts_parser_new();
    if (!ctx->ts_parser) {
        parser_set_error(ctx, -1, "Failed to create Tree-sitter parser");
        return false;
    }

    // Set language
    const TSLanguage *ts_language = NULL;
    switch (language) {
        case LANG_C:
            ts_language = tree_sitter_c();
            break;
        case LANG_CPP:
            ts_language = tree_sitter_cpp();
            break;
        case LANG_PYTHON:
            ts_language = tree_sitter_python();
            break;
        case LANG_JAVASCRIPT:
            ts_language = tree_sitter_javascript();
            break;
        case LANG_TYPESCRIPT:
            ts_language = tree_sitter_typescript();
            break;
        default:
            parser_set_error(ctx, -1, "Unsupported language type");
            ts_parser_delete(ctx->ts_parser);
            ctx->ts_parser = NULL;
            return false;
    }

    if (!ts_parser_set_language(ctx->ts_parser, ts_language)) {
        parser_set_error(ctx, -1, "Failed to set Tree-sitter language");
        ts_parser_delete(ctx->ts_parser);
        ctx->ts_parser = NULL;
        return false;
    }

    // Initialize query manager if needed
    if (!ctx->query_manager) {
        ctx->query_manager = query_manager_new();
        if (!ctx->query_manager) {
            parser_set_error(ctx, -1, "Failed to create query manager");
            ts_parser_delete(ctx->ts_parser);
            ctx->ts_parser = NULL;
            return false;
        }

        // Load queries directory for this language
        char *queries_dir = build_queries_dir(language);
        if (queries_dir) {
            query_manager_set_queries_dir(ctx->query_manager, queries_dir);
            free(queries_dir);
        }
    }

    // Store language type
    ctx->language_type = language;

    if (ctx->log_level <= LOG_DEBUG) {
        log_debug("Successfully initialized Tree-sitter parser for language %d", language);
    }

    return true;
}
