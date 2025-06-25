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

#include "../../core/include/scopemux/adapters/adapter_registry.h"
#include "../../core/include/scopemux/adapters/language_adapter.h"
#include "../../core/include/scopemux/logging.h"
#include "../../core/include/scopemux/parser.h"
#include "../../core/include/scopemux/query_manager.h"
#include "../../core/include/scopemux/tree_sitter_integration.h"
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
char *build_queries_dir_impl(LanguageType language) {
  const char *base = "/home/matrillo/apps/scopemux/queries";
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

  // Calculate length and allocate memory
  size_t len = strlen(base) + 1 + strlen(subdir) + 1;
  char *result = (char *)malloc(len);
  if (!result) {
    fprintf(stderr, "ERROR: Failed to allocate memory for queries directory path\n");
    return NULL;
  }

  // Format the full path
  snprintf(result, len, "%s/%s", base, subdir);

  // Verify that the directory exists
  struct stat st;
  if (stat(result, &st) != 0 || !S_ISDIR(st.st_mode)) {
    fprintf(stderr, "WARNING: Queries directory does not exist or is not a directory: %s\n",
            result);
  } else {
    fprintf(stderr, "DEBUG: Using queries directory: %s\n", result);
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
bool ts_init_parser_impl(ParserContext *ctx, LanguageType language) {
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

// Force symbol resolution check for our language functions using dlsym
#if defined(__GNUC__)
  void *c_sym = dlsym(NULL, "tree_sitter_c");
  void *cpp_sym = dlsym(NULL, "tree_sitter_cpp");
  void *python_sym = dlsym(NULL, "tree_sitter_python");
  void *js_sym = dlsym(NULL, "tree_sitter_javascript");
  void *ts_sym = dlsym(NULL, "tree_sitter_typescript");

  fprintf(stderr, "SYMBOL RESOLUTION CHECK:\n");
  fprintf(stderr, "  dlsym(tree_sitter_c): %p\n", c_sym);
  fprintf(stderr, "  dlsym(tree_sitter_cpp): %p\n", cpp_sym);
  fprintf(stderr, "  dlsym(tree_sitter_python): %p\n", python_sym);
  fprintf(stderr, "  dlsym(tree_sitter_javascript): %p\n", js_sym);
  fprintf(stderr, "  dlsym(tree_sitter_typescript): %p\n", ts_sym);
#endif

  // Set language
  const TSLanguage *ts_language = NULL;

  fprintf(stderr, "\n===== TS_INIT_PARSER: LANGUAGE FUNCTION DIAGNOSTICS =====\n");
  fprintf(stderr, "Language type: %d (1=C, 2=CPP, 3=Python, 4=JavaScript, 5=TypeScript)\n",
          language);
  fprintf(stderr, "  tree_sitter_c function address: %p\n", (void *)&tree_sitter_c);
  fprintf(stderr, "  tree_sitter_cpp function address: %p\n", (void *)&tree_sitter_cpp);
  fprintf(stderr, "  tree_sitter_python function address: %p\n", (void *)&tree_sitter_python);
  fprintf(stderr, "  tree_sitter_javascript function address: %p\n",
          (void *)&tree_sitter_javascript);
  fprintf(stderr, "  tree_sitter_typescript function address: %p\n",
          (void *)&tree_sitter_typescript);

  // ADVANCED DIAGNOSTICS: Test direct function pointer usage
  typedef const TSLanguage *(*LangFunc)(void);
  LangFunc c_func_ptr = &tree_sitter_c;
  LangFunc cpp_func_ptr = &tree_sitter_cpp;
  LangFunc python_func_ptr = &tree_sitter_python;
  LangFunc js_func_ptr = &tree_sitter_javascript;
  LangFunc ts_func_ptr = &tree_sitter_typescript;

  // Use printf directly to ensure output is visible regardless of log level
  fprintf(stderr, "\n==== TREE-SITTER LANGUAGE FUNCTION DIAGNOSTICS ====\n");
  fprintf(stderr, "Function pointers from language libraries:\n");
  fprintf(stderr, "  C function pointer:          %p\n", (void *)c_func_ptr);
  fprintf(stderr, "  C++ function pointer:        %p\n", (void *)cpp_func_ptr);
  fprintf(stderr, "  Python function pointer:     %p\n", (void *)python_func_ptr);
  fprintf(stderr, "  JavaScript function pointer: %p\n", (void *)js_func_ptr);
  fprintf(stderr, "  TypeScript function pointer: %p\n", (void *)ts_func_ptr);

  // Try calling C++ function pointer directly
  if (language == LANG_CPP) {
    fprintf(stderr, "\nCalling C++ function pointer directly:\n");
    const TSLanguage *direct_result = NULL;
    if (cpp_func_ptr) {
      direct_result = (*cpp_func_ptr)();
      fprintf(stderr, "  Direct function pointer call result: %p\n", (void *)direct_result);
    } else {
      fprintf(stderr, "  ERROR: C++ function pointer is NULL, cannot call\n");
    }
  }

  fprintf(stderr, "\nAttempting to call appropriate language function...\n");
  switch (language) {
  case LANG_C:
    fprintf(stderr, "  Using tree_sitter_c()\n");
    ts_language = tree_sitter_c();
    fprintf(stderr, "  tree_sitter_c() returned: %p\n", (void *)ts_language);
    break;
  case LANG_CPP:
    fprintf(stderr, "  Using tree_sitter_cpp()\n");
    ts_language = tree_sitter_cpp();
    fprintf(stderr, "  tree_sitter_cpp() returned: %p\n", (void *)ts_language);
    break;
  case LANG_PYTHON:
    fprintf(stderr, "  Using tree_sitter_python()\n");
    ts_language = tree_sitter_python();
    fprintf(stderr, "  tree_sitter_python() returned: %p\n", (void *)ts_language);
    break;
  case LANG_JAVASCRIPT:
    fprintf(stderr, "  Using tree_sitter_javascript()\n");
    ts_language = tree_sitter_javascript();
    fprintf(stderr, "  tree_sitter_javascript() returned: %p\n", (void *)ts_language);
    break;
  case LANG_TYPESCRIPT:
    fprintf(stderr, "  Using tree_sitter_typescript()\n");
    ts_language = tree_sitter_typescript();
    fprintf(stderr, "  tree_sitter_typescript() returned: %p\n", (void *)ts_language);
    break;
  default:
    log_error("CRITICAL ERROR: Unsupported language type: %d", language);
    parser_set_error(ctx, -1, "Unsupported language type");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  if (ts_language == NULL) {
    log_error("CRITICAL ERROR: Language function returned NULL for language type: %d", language);
    parser_set_error(ctx, -1, "Failed to retrieve Tree-sitter language");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  log_error("Successfully retrieved language object for language type %d, address: %p", language,
            (void *)ts_language);

  log_error("Calling ts_parser_set_language with parser=%p, language=%p", (void *)ctx->ts_parser,
            (void *)ts_language);

  bool result = ts_parser_set_language(ctx->ts_parser, ts_language);
  if (!result) {
    log_error("CRITICAL ERROR: ts_parser_set_language failed");
    parser_set_error(ctx, -1, "Failed to set Tree-sitter language");
    ts_parser_delete(ctx->ts_parser);
    ctx->ts_parser = NULL;
    return false;
  }

  log_error("ts_parser_set_language succeeded");

  // Store the language type in the context
  ctx->language = language;

  // Verify the language was set by querying the parser
  const TSLanguage *verification_lang = ts_parser_language(ctx->ts_parser);
  log_error("Verification: ts_parser_language() returned: %p", (void *)verification_lang);

  if (verification_lang == NULL) {
    log_error("CRITICAL ERROR: Language verification failed - ts_parser_language returned NULL");
    parser_set_error(ctx, -1, "Language verification failed");
    return false;
  } else if (verification_lang != ts_language) {
    log_error("WARNING: Language pointer mismatch - expected %p, got %p", (void *)ts_language,
              (void *)verification_lang);
  } else {
    log_error("Language verification successful - pointer match confirmed");
  }

  log_error("TS_INIT_PARSER_IMPL: PARSER INITIALIZATION COMPLETE");
  log_error("===================================================================");

  log_debug("Successfully set language for Tree-sitter parser");

  // Initialize query manager if needed
  if (!ctx->q_manager) {
    // Get queries directory for this language
    char *queries_dir = build_queries_dir_impl(language);
    if (!queries_dir) {
      log_error("CRITICAL ERROR: Failed to build queries directory path");
      parser_set_error(ctx, -1, "Failed to build queries directory path");
      ts_parser_delete(ctx->ts_parser);
      ctx->ts_parser = NULL;
      return false;
    }
    
    // Check if queries directory exists and has necessary files
    struct stat st;
    if (stat(queries_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
      log_error("CRITICAL ERROR: Queries directory does not exist or is not accessible: %s", queries_dir);
      parser_set_error(ctx, -1, "Queries directory does not exist or is not accessible");
      free(queries_dir);
      ts_parser_delete(ctx->ts_parser);
      ctx->ts_parser = NULL;
      return false;
    }
    
    // Verify existence of critical query files (e.g., docstrings.scm)
    char docstrings_path[1024];
    snprintf(docstrings_path, sizeof(docstrings_path), "%s/docstrings.scm", queries_dir);
    
    if (stat(docstrings_path, &st) != 0) {
      log_error("WARNING: docstrings.scm not found at: %s", docstrings_path);
    } else {
      log_error("Found docstrings.scm at: %s", docstrings_path);
    }
    
    // Load node type mappings
    char mapping_path[1024];
    snprintf(mapping_path, sizeof(mapping_path), "%s/../../core/config/node_type_mapping.json", queries_dir);
    
    if (stat(mapping_path, &st) == 0) {
      log_error("Loading node type mappings from: %s", mapping_path);
      load_node_type_mapping(mapping_path);
    } else {
      log_error("WARNING: Node type mapping not found at: %s", mapping_path);
    }

    // Initialize query manager with the queries directory
    ctx->q_manager = query_manager_init(queries_dir);
    log_error("Initialized query manager with queries directory: %s", queries_dir);
    free(queries_dir);

    if (!ctx->q_manager) {
      log_error("CRITICAL ERROR: Failed to initialize query manager");
      parser_set_error(ctx, -1, "Failed to initialize query manager");
      ts_parser_delete(ctx->ts_parser);
      ctx->ts_parser = NULL;
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
