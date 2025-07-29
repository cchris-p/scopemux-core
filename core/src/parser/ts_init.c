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

#include "../../include/scopemux/adapters/adapter_registry.h"
#include "../../include/scopemux/adapters/language_adapter.h"
#include "../../include/scopemux/logging.h"
#include "../../include/scopemux/memory_debug.h"
#include "../../include/scopemux/memory_management.h"
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/query_manager.h"
#include "config/node_type_mapping_loader.h"

#include "../../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include <dlfcn.h>
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *)0)
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
char *build_queries_dir_impl(Language language) {
  const char *subdir = "unknown";

  // Map language enum to directory name
  switch (language) {
  case LANG_C:
    subdir = "c";
    break;
  case LANG_CPP:
    subdir = "cpp";
    break;
  case LANG_PYTHON:
    subdir = "python";
    break;
  case LANG_JAVASCRIPT:
    subdir = "javascript";
    break;
  case LANG_TYPESCRIPT:
    subdir = "typescript";
    break;
  default:
    break;
  }

  // Check environment variable first
  const char *env_queries_dir = getenv("SCMU_QUERIES_DIR");
  if (env_queries_dir) {
    size_t len = strlen(env_queries_dir) + 1 + strlen(subdir) + 1;
    char *result = (char *)safe_malloc(len);
    if (result) {
      snprintf(result, len, "%s/%s", env_queries_dir, subdir);
      struct stat st;
      if (stat(result, &st) == 0 && S_ISDIR(st.st_mode)) {
        fprintf(stderr, "DEBUG: Using environment queries directory: %s\n", result);
        return result;
      }
      safe_free(result);
    }
  }

  // Try multiple fallback paths
  const char *paths[] = {
    "queries",           // From project root
    "../queries",       // One level up
    "../../queries",    // Two levels up (for build/core/ execution)
    "../../../queries"  // Three levels up (for build/core/tests/ execution)
  };
  size_t num_paths = sizeof(paths) / sizeof(paths[0]);

  for (size_t i = 0; i < num_paths; i++) {
    size_t len = strlen(paths[i]) + 1 + strlen(subdir) + 1;
    char *result = (char *)safe_malloc(len);
    if (!result) {
      fprintf(stderr, "ERROR: Failed to allocate memory for queries directory path\n");
      continue;
    }
    snprintf(result, len, "%s/%s", paths[i], subdir);

    // Verify that the directory exists
    struct stat st;
    if (stat(result, &st) == 0 && S_ISDIR(st.st_mode)) {
      fprintf(stderr, "DEBUG: Using queries directory: %s\n", result);
      return result;
    }
    
    fprintf(stderr, "DEBUG: Tried queries directory: %s (not found)\n", result);
    safe_free(result);
  }

  // If all paths failed, return the first fallback path anyway
  size_t len = strlen(paths[0]) + 1 + strlen(subdir) + 1;
  char *result = (char *)safe_malloc(len);
  if (result) {
    snprintf(result, len, "%s/%s", paths[0], subdir);
    fprintf(stderr, "WARNING: No queries directory found, using fallback: %s\n", result);
  }
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
// Ensure this is properly exported for linking
bool ts_init_parser_impl(ParserContext *ctx, Language language) {
  fprintf(stderr, "[DIAGNOSTIC-ENTRY] Entered ts_init_parser_impl: ctx=%p, language=%d\n",
          (void *)ctx, language);
  log_debug("ts_init_parser_impl called with language: %d, ctx: %p", language, (void *)ctx);
  if (!ctx) {
    log_error("NULL context passed to ts_init_parser");
    return false;
  }

  // Check if parser already exists
  if (ctx->ts_parser) {
    // Clean up existing parser if it exists
    ts_parser_delete(ctx->ts_parser);
  }

  // Create new parser
  ctx->ts_parser = ts_parser_new();
  if (!ctx->ts_parser) {
    fprintf(stderr, "Failed to create Tree-sitter parser\n");
    return false;
  }

  // Store language type in context for later reference
  ctx->language = language;

  fprintf(stderr, "\n***** PARSER INITIALIZATION DIAGNOSTIC *****\n");
  fprintf(stderr, "Test executable pid: %d\n", (int)getpid());

// We have static declarations for the Tree-sitter language functions
// Instead of trying to load shared libraries that don't exist, use the statically linked functions
#if defined(__GNUC__)
  // Log that we're using statically linked Tree-sitter libraries
  fprintf(stderr, "Using statically linked Tree-sitter libraries\n");

  // Get function pointers to the statically linked functions
  void *c_sym = (void *)&tree_sitter_c;
  void *cpp_sym = (void *)&tree_sitter_cpp;
  void *python_sym = (void *)&tree_sitter_python;
  void *js_sym = (void *)&tree_sitter_javascript;
  void *ts_sym = (void *)&tree_sitter_typescript;

  fprintf(stderr, "SYMBOL RESOLUTION CHECK:\n");
  fprintf(stderr, "  dlsym(tree_sitter_c): %p\n", c_sym);
  fprintf(stderr, "  dlsym(tree_sitter_cpp): %p\n", cpp_sym);
  fprintf(stderr, "  dlsym(tree_sitter_python): %p\n", python_sym);
  fprintf(stderr, "  dlsym(tree_sitter_javascript): %p\n", js_sym);
  fprintf(stderr, "  dlsym(tree_sitter_typescript): %p\n", ts_sym);
#endif

  // DIAGNOSTIC: Print language enum and adapter lookup
  fprintf(stderr, "[DIAGNOSTIC] ts_init_parser_impl: language enum: %d\n", language);
  const char *lang_name = NULL;
  switch (language) {
  case LANG_C:
    lang_name = "C";
    break;
  case LANG_CPP:
    lang_name = "C++";
    break;
  case LANG_PYTHON:
    lang_name = "Python";
    break;
  case LANG_JAVASCRIPT:
    lang_name = "JavaScript";
    break;
  case LANG_TYPESCRIPT:
    lang_name = "TypeScript";
    break;
  default:
    lang_name = "UNKNOWN";
    break;
  }
  fprintf(stderr, "[DIAGNOSTIC] Language name: %s\n", lang_name);

  LanguageAdapter *adapter = get_adapter_by_language(language);
  fprintf(stderr, "[DIAGNOSTIC] Adapter lookup result: %p\n", (void *)adapter);
  if (adapter) {
    fprintf(stderr, "[DIAGNOSTIC] Adapter language_name: %s\n", adapter->language_name);
    fprintf(stderr, "[DIAGNOSTIC] Adapter get_ts_language pointer: %p\n",
            (void *)adapter->get_ts_language);
    if (adapter->get_ts_language) {
      const TSLanguage *ts_lang_ptr = adapter->get_ts_language();
      fprintf(stderr, "[DIAGNOSTIC] Result of get_ts_language(): %p\n", (void *)ts_lang_ptr);
    } else {
      fprintf(stderr, "[DIAGNOSTIC] Adapter get_ts_language is NULL!\n");
    }
  } else {
    fprintf(stderr, "[DIAGNOSTIC] Adapter is NULL!\n");
  }

  // Set language
  if (!adapter || !adapter->get_ts_language) {
    log_error("CRITICAL ERROR: Unsupported language type: %d", language);
    parser_set_error(ctx, -1, "Unsupported language type");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }
  const TSLanguage *ts_language = adapter->get_ts_language();

  if (ts_language == NULL) {
    log_error("CRITICAL ERROR: Language function returned NULL for language type: %d", language);
    parser_set_error(ctx, -1, "Failed to retrieve Tree-sitter language");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  log_info("Successfully retrieved language object for language type %d, address: %p", language,
           (void *)ts_language);

  log_info("Calling ts_parser_set_language with parser=%p, language=%p", (void *)ctx->ts_parser,
           (void *)ts_language);

  bool result = ts_parser_set_language(ctx->ts_parser, ts_language);
  if (!result) {
    log_info("CRITICAL ERROR: ts_parser_set_language failed");
    parser_set_error(ctx, -1, "Failed to set Tree-sitter language");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  log_info("ts_parser_set_language succeeded");

  // Store the language type in the context
  ctx->language = language;

  // Verify the language was set by querying the parser
  const TSLanguage *verification_lang = ts_parser_language(ctx->ts_parser);
  log_info("Verification: ts_parser_language() returned: %p", (void *)verification_lang);

  if (verification_lang == NULL) {
    log_error("CRITICAL ERROR: Language verification failed - ts_parser_language returned NULL");
    parser_set_error(ctx, -1, "Language verification failed");
    return false;
  } else if (verification_lang != ts_language) {
    log_warning("WARNING: Language pointer mismatch - expected %p, got %p", (void *)ts_language,
                (void *)verification_lang);
  } else {
    log_info("Language verification successful - pointer match confirmed");
  }

  log_info("TS_INIT_PARSER_IMPL: PARSER INITIALIZATION COMPLETE");
  log_info("===================================================================");

  log_info("Successfully set language for Tree-sitter parser");

  // Initialize query manager if needed
  if (!ctx->q_manager) {
    // Get queries directory for this language
    char *queries_dir = build_queries_dir_impl(language);
    if (!queries_dir) {
      log_error("CRITICAL ERROR: Failed to build queries directory path");
      parser_set_error(ctx, -1, "Failed to build queries directory path");
      if (ctx->ts_parser) {
        ts_parser_delete(ctx->ts_parser);
        ctx->ts_parser = NULL;
      }
      return false;
    }

    // Check if queries directory exists and has necessary files
    struct stat st;
    if (stat(queries_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
      log_error("CRITICAL ERROR: Queries directory does not exist or is not accessible: %s",
                SAFE_STR(queries_dir));
      parser_set_error(ctx, -1, "Queries directory does not exist or is not accessible");
      safe_free(queries_dir);
      if (ctx->ts_parser) {
        ts_parser_delete(ctx->ts_parser);
        ctx->ts_parser = NULL;
      }
      return false;
    }

    // Verify existence of critical query files (e.g., docstrings.scm)
    char docstrings_path[1024];
    snprintf(docstrings_path, sizeof(docstrings_path), "%s/docstrings.scm", queries_dir);

    if (stat(docstrings_path, &st) != 0) {
      log_error("WARNING: docstrings.scm not found at: %s", SAFE_STR(docstrings_path));
    } else {
      log_info("Found docstrings.scm at: %s", SAFE_STR(docstrings_path));
    }

    // Load hardcoded node type mappings
    log_info("Loading hardcoded node type mappings (source of truth)...");
    load_node_type_mapping();

    // Initialize query manager with the queries directory
    ctx->q_manager = query_manager_init(queries_dir);
    log_info("Initialized query manager with queries directory: %s", SAFE_STR(queries_dir));
    safe_free(queries_dir);

    if (!ctx->q_manager) {
      log_error("CRITICAL ERROR: Failed to initialize query manager");
      parser_set_error(ctx, -1, "Failed to initialize query manager");
      if (ctx->ts_parser) {
        ts_parser_delete(ctx->ts_parser);
        ctx->ts_parser = NULL;
      }
      return false;
    }
  }

  // Store language type
  ctx->language = language;

  // Check if parser language is correctly set
  const TSLanguage *current_lang = ts_parser_language(ctx->ts_parser);
  log_debug("After setting, Tree-sitter parser language object: %p", (void *)current_lang);

  if (ctx->log_level <= LOG_DEBUG) {
    log_debug("Successfully initialized Tree-sitter parser for language %d", language);
  }

  return true;
}
