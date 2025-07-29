/**
 * @file query_manager.c
 * @brief Implementation of the QueryManager for Tree-sitter queries
 *
 * This file implements the functionality for loading, compiling, and caching
 * Tree-sitter queries from .scm files.
 *
 * Debug Control:
 * - DEBUG_MODE: Controls general test-focused debugging messages
 * - DIRECT_DEBUG_MODE: Controls verbose query loading and execution diagnostics
 * - QUERY_PATH_DEBUG_MODE: Controls query path resolution debugging messages
 *
 * The debug mechanism operates on three levels:
 * 1. Standard DEBUG_MODE (defined in test files) - Controls basic test diagnostics
 * 2. DIRECT_DEBUG_MODE - Controls detailed parsing and query execution information
 * 3. QUERY_PATH_DEBUG_MODE - Controls query path resolution messages
 *
 * To enable detailed diagnostics during query loading and compilation, set
 * DIRECT_DEBUG_MODE to true. This will show comprehensive information about query
 * processing and execution.
 *
 * For query path resolution diagnostics only, set QUERY_PATH_DEBUG_MODE to true.
 * This will show the paths being tried when resolving query files.
 */

// Controls detailed debug output from the query manager
// Only enable temporarily when diagnosing issues with query loading or compilation
#define DIRECT_DEBUG_MODE true

// Controls debugging output for query path resolution
// Shows paths attempted when loading .scm query files
#define QUERY_PATH_DEBUG_MODE true

#define _POSIX_C_SOURCE 200809L // For strdup

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../core/include/scopemux/parser.h" // For Language constants
#include "../../core/include/scopemux/query_manager.h"
#include "scopemux/adapters/language_adapter.h"
#include "scopemux/common.h"

/**
 * @brief Structure to represent a cached query.
 */
typedef struct QueryCacheEntry {
  const char *query_name; // Name of the query (e.g., "functions", "calls")
  const TSQuery *query;   // Compiled Tree-sitter query
} QueryCacheEntry;

/**
 * @brief Structure for the QueryManager that manages Tree-sitter queries.
 */
struct QueryManager {
  const char *queries_dir;          // Root directory for query files
  const TSLanguage **languages;     // Array of supported languages
  Language *language_types;         // Array of language type enums
  QueryCacheEntry **cached_queries; // Array of cached queries per language
  size_t *query_counts;             // Array of query counts per language
  size_t language_count;            // Number of supported languages
  size_t max_queries_per_language;  // Maximum number of languages supported
};

#define MAX_LANGUAGES 6 // UNKNOWN, C, CPP, PYTHON, JAVASCRIPT, TYPESCRIPT

/**
 * @brief Initializes the query manager.
 *
 * @param queries_dir The root directory path where language-specific query
 *                    files are stored.
 * @return A pointer to the newly created QueryManager, or NULL on failure.
 */
QueryManager *query_manager_init(const char *queries_dir) {
  if (!queries_dir) {
    return NULL;
  }

  // Allocate memory for the manager
  QueryManager *manager = (QueryManager *)safe_malloc(sizeof(QueryManager));
  if (!manager) {
    return NULL;
  }

  // Initialize with default values
  memset(manager, 0, sizeof(QueryManager));

  // Copy queries directory path
  manager->queries_dir = safe_strdup(queries_dir);
  if (!manager->queries_dir) {
    safe_free(manager);
    return NULL;
  }

  // Initialize the supported languages
  manager->language_count = MAX_LANGUAGES;
  manager->max_queries_per_language = 16; // Reasonable max number of queries per language

  // Allocate arrays for languages, language_types, cached_queries, and query_counts
  manager->languages = (const TSLanguage **)safe_malloc(MAX_LANGUAGES * sizeof(TSLanguage *));
  manager->language_types = (Language *)safe_malloc(MAX_LANGUAGES * sizeof(Language));
  manager->cached_queries =
      (QueryCacheEntry **)safe_malloc(MAX_LANGUAGES * sizeof(QueryCacheEntry *));
  manager->query_counts = (size_t *)safe_malloc(MAX_LANGUAGES * sizeof(size_t));

  // Check if all allocations succeeded
  if (!manager->languages || !manager->language_types || !manager->cached_queries ||
      !manager->query_counts) {
    query_manager_free(manager);
    return NULL;
  }

  // Initialize arrays with zeros
  memset(manager->languages, 0, MAX_LANGUAGES * sizeof(TSLanguage *));
  memset(manager->language_types, 0, MAX_LANGUAGES * sizeof(Language));
  memset(manager->cached_queries, 0, MAX_LANGUAGES * sizeof(QueryCacheEntry *));
  memset(manager->query_counts, 0, MAX_LANGUAGES * sizeof(size_t));

  // Initialize memory for query cache entries
  for (size_t i = 0; i < MAX_LANGUAGES; i++) {
    manager->cached_queries[i] =
        (QueryCacheEntry *)safe_malloc(manager->max_queries_per_language * sizeof(QueryCacheEntry));
    if (!manager->cached_queries[i]) {
      query_manager_free(manager);
      return NULL;
    }
    memset(manager->cached_queries[i], 0,
           manager->max_queries_per_language * sizeof(QueryCacheEntry));
  }

  // Set up the supported languages
  // Initialize both language types and the corresponding Tree-sitter language objects
  manager->language_types[0] = LANG_UNKNOWN;
  manager->language_types[1] = LANG_C;
  manager->language_types[2] = LANG_CPP;
  manager->language_types[3] = LANG_PYTHON;
  manager->language_types[4] = LANG_JAVASCRIPT;
  manager->language_types[5] = LANG_TYPESCRIPT;

  // Initialize language objects
  bool has_valid_language = false;
  for (int lang = LANG_C; lang < LANG_MAX; ++lang) {
    LanguageAdapter *adapter = get_adapter_by_language((Language)lang);
    if (adapter && adapter->get_ts_language) {
      manager->languages[lang] = adapter->get_ts_language();
      if (manager->languages[lang]) {
        has_valid_language = true;
      }
    } else {
      manager->languages[lang] = NULL;
    }
  }

  // If no valid languages were found, clean up and return NULL
  if (!has_valid_language) {
    query_manager_free(manager);
    return NULL;
  }

  return manager;
}

/**
 * @brief Frees all resources associated with the query manager.
 *
 * @param manager The query manager to free.
 */
void query_manager_free(QueryManager *manager) {
  printf("[QUERY_MANAGER_FREE] ENTER: manager=%p\n", (void *)manager);
  fflush(stdout);

  if (!manager) {
    printf("[QUERY_MANAGER_FREE] EXIT: manager is NULL\n");
    fflush(stdout);
    return;
  }

  // Free queries_dir
  if (manager->queries_dir) {
    printf("[QUERY_MANAGER_FREE] Freeing queries_dir at %p\n", (void *)manager->queries_dir);
    fflush(stdout);
    safe_free((void *)manager->queries_dir);
    manager->queries_dir = NULL;
  }

  // Free all cached queries and their names
  if (manager->cached_queries) {
    printf("[QUERY_MANAGER_FREE] Freeing cached queries array at %p (language_count=%zu)\n",
           (void *)manager->cached_queries, manager->language_count);
    fflush(stdout);

    for (size_t i = 0; i < manager->language_count; i++) {
      if (manager->cached_queries[i]) {
        printf("[QUERY_MANAGER_FREE] Freeing cached queries for language %zu at %p "
               "(query_count=%zu)\n",
               i, (void *)manager->cached_queries[i], manager->query_counts[i]);
        fflush(stdout);

        // Free each query in the language
        for (size_t j = 0; j < manager->query_counts[i]; j++) {
          if (manager->cached_queries[i][j].query_name) {
            safe_free((void *)manager->cached_queries[i][j].query_name);
          }
          if (manager->cached_queries[i][j].query) {
            ts_query_delete((TSQuery *)manager->cached_queries[i][j].query);
          }
        }

        printf("[QUERY_MANAGER_FREE] Freeing QueryCacheEntry array at %p\n",
               (void *)manager->cached_queries[i]);
        fflush(stdout);
        safe_free(manager->cached_queries[i]);
        manager->cached_queries[i] = NULL;
      }
    }

    printf("[QUERY_MANAGER_FREE] Freeing cached_queries array at %p\n",
           (void *)manager->cached_queries);
    fflush(stdout);
    safe_free(manager->cached_queries);
    manager->cached_queries = NULL;
  }

  // Free languages array
  if (manager->languages) {
    printf("[QUERY_MANAGER_FREE] Freeing languages array at %p\n", (void *)manager->languages);
    fflush(stdout);
    safe_free(manager->languages);
    manager->languages = NULL;
  }

  // Free language_types array
  if (manager->language_types) {
    printf("[QUERY_MANAGER_FREE] Freeing language_types array at %p\n",
           (void *)manager->language_types);
    fflush(stdout);
    safe_free(manager->language_types);
    manager->language_types = NULL;
  }

  // Free query_counts array
  if (manager->query_counts) {
    printf("[QUERY_MANAGER_FREE] Freeing query_counts array at %p\n",
           (void *)manager->query_counts);
    fflush(stdout);
    safe_free(manager->query_counts);
    manager->query_counts = NULL;
  }

  // Free the manager struct itself
  printf("[QUERY_MANAGER_FREE] Freeing manager struct at %p\n", (void *)manager);
  fflush(stdout);
  safe_free(manager);

  printf("[QUERY_MANAGER_FREE] EXIT: Query manager cleanup complete\n");
  fflush(stdout);
}

/**
 * @brief Constructs the file path for a specific query and language.
 *
 * @param manager The query manager instance.
 * @param language_name The name of the programming language (lowercase).
 * @param query_name The name of the query (without extension).
 * @return A dynamically allocated string with the full path, or NULL on failure.
 *         The caller is responsible for freeing this memory.
 */
static char *construct_query_path(const QueryManager *manager, const char *language_name,
                                  const char *query_name) {
  if (!manager || !manager->queries_dir || !language_name || !query_name) {
    fprintf(stderr,
            "[QUERY_PATH_DEBUG] Invalid parameters for construct_query_path: manager=%p, "
            "queries_dir=%s, language_name=%s, query_name=%s\n",
            (void *)manager,
            manager ? (manager->queries_dir ? manager->queries_dir : "NULL") : "NULL",
            language_name ? language_name : "NULL", query_name ? query_name : "NULL");
    return NULL;
  }

  size_t path_len = strlen(manager->queries_dir) + strlen(language_name) + strlen(query_name) + 7;
  char *full_path = (char *)safe_malloc(path_len);
  if (!full_path) {
    fprintf(stderr, "[QUERY_PATH_DEBUG] Failed to allocate memory for query path\n");
    return NULL;
  }

  // Primary path - manager->queries_dir already includes the language subdirectory
  snprintf(full_path, path_len, "%s/%s.scm", manager->queries_dir, query_name);
  fprintf(stderr, "[QUERY_PATH_DEBUG] Trying primary path: %s\n", full_path);
  if (access(full_path, F_OK) == 0) {
    fprintf(stderr, "[QUERY_PATH_DEBUG] SUCCESS: Found query at primary path: %s\n", full_path);
    return full_path;
  }
  fprintf(stderr, "[QUERY_PATH_DEBUG] Primary path not found: %s (error: %s)\n", full_path,
          strerror(errno));

  // Fallback path
  snprintf(full_path, path_len, "/home/matrillo/apps/scopemux/queries/%s/%s.scm", language_name,
           query_name);
  fprintf(stderr, "[QUERY_PATH_DEBUG] Trying fallback path: %s\n", full_path);
  if (access(full_path, F_OK) == 0) {
    fprintf(stderr, "[QUERY_PATH_DEBUG] SUCCESS: Found query at fallback path: %s\n", full_path);
    return full_path;
  }
  fprintf(stderr, "[QUERY_PATH_DEBUG] Fallback path not found: %s (error: %s)\n", full_path,
          strerror(errno));

  // Try another fallback with relative path
  snprintf(full_path, path_len, "./queries/%s/%s.scm", language_name, query_name);
  fprintf(stderr, "[QUERY_PATH_DEBUG] Trying relative fallback path: %s\n", full_path);
  if (access(full_path, F_OK) == 0) {
    fprintf(stderr, "[QUERY_PATH_DEBUG] SUCCESS: Found query at relative fallback path: %s\n",
            full_path);
    return full_path;
  }
  fprintf(stderr, "[QUERY_PATH_DEBUG] Relative fallback path not found: %s (error: %s)\n",
          full_path, strerror(errno));

  fprintf(stderr, "[QUERY_PATH_DEBUG] FAILED: Query file not found at any path: %s/%s.scm\n",
          language_name, query_name);

  safe_free(full_path);
  return NULL;
}

/**
 * @brief Reads the content of a query file into memory.
 *
 * @param file_path The path to the query file.
 * @param content_len Pointer where the length of the content will be stored.
 * @return A dynamically allocated buffer containing the file content, or NULL on failure.
 *         The caller is responsible for freeing this memory.
 */
static char *read_query_file(const char *file_path, uint32_t *content_len) {
  if (!file_path || !content_len) {
    return NULL;
  }

  // Open the file
  FILE *file = fopen(file_path, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open query file: %s (error: %s)\n", file_path, strerror(errno));
    return NULL;
  }

  // Get the file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  if (file_size < 0) {
    fprintf(stderr, "Failed to determine query file size: %s\n", file_path);
    fclose(file);
    return NULL;
  }
  rewind(file);

  // Allocate buffer for the file content
  char *content = (char *)safe_malloc(file_size + 1); // +1 for null terminator
  if (!content) {
    fprintf(stderr, "Failed to allocate memory for query content\n");
    fclose(file);
    return NULL;
  }

  // Read the file content
  size_t bytes_read = fread(content, 1, file_size, file);
  fclose(file);

  if (bytes_read != (size_t)file_size) {
    fprintf(stderr, "Failed to read entire query file: %s\n", file_path);
    safe_free(content);
    return NULL;
  }

  // Null-terminate the content buffer
  content[file_size] = '\0';
  *content_len = (uint32_t)file_size;

  return content;
}

/**
 * @brief Compiles a Tree-sitter query from a string.
 *
 * @param language The Tree-sitter language.
 * @param query_str The query string in the Tree-sitter query language.
 * @param query_len The length of the query string.
 * @param error_offset Pointer to store the error offset if compilation fails.
 * @return A pointer to the compiled TSQuery, or NULL on failure.
 */
static const TSQuery *compile_query(const TSLanguage *language, const char *query_str,
                                    uint32_t query_len, uint32_t *error_offset) {
  log_debug("ENTERING compile_query with language=%p, query_len=%u", (void *)language, query_len);

  // Validate input parameters
  if (!language) {
    log_error("compile_query: language is NULL");
    return NULL;
  }

  if (!query_str) {
    log_error("compile_query: query_str is NULL");
    return NULL;
  }

  if (query_len == 0) {
    log_error("compile_query: query_len is 0");
    return NULL;
  }

  // Log first few characters of the query for debugging
  char preview[41] = {0};
  uint32_t preview_len = query_len < 40 ? query_len : 40;
  memcpy(preview, query_str, preview_len);
  preview[preview_len] = '\0';
  log_debug("Query preview: '%s%s'", SAFE_STR(preview), query_len > 40 ? "..." : "");

  // Compile the query using Tree-sitter
  uint32_t error_type = 0;
  log_debug("Calling ts_query_new with language=%p, query_str=%p, query_len=%u", (void *)language,
            (void *)query_str, query_len);

  const TSQuery *query = ts_query_new(language, query_str, query_len, error_offset, &error_type);

  if (!query) {
    const char *error_types[] = {"None", "Syntax", "NodeType", "Field", "Capture"};
    const char *error_type_str = (error_type < 5) ? error_types[error_type] : "Unknown";

    log_error("Failed to compile query: %s error at offset %u", error_type_str,
              error_offset ? *error_offset : 0);

    // Print a snippet of the query string around the error location
    if (error_offset && *error_offset < query_len) {
      uint32_t start = *error_offset > 20 ? *error_offset - 20 : 0;
      uint32_t end = *error_offset + 20 < query_len ? *error_offset + 20 : query_len;
      fprintf(stderr, "[QUERY_DEBUG] Query error context: ...%.*s[ERROR]%.*s...\n",
              *error_offset - start, query_str + start, end - *error_offset,
              query_str + *error_offset);
    }
    // Temporarily commenting this out
    // fprintf(stderr, "[QUERY_DEBUG] Loaded query content:\n%s\n", query_str);
  }
  // We don't need to print the contents
  // else {
  //   log_debug("Successfully compiled query");
  //   fprintf(stderr, "[QUERY_DEBUG] Loaded query content:\n%s\n", query_str);
  // }

  return query;
}

/**
 * @brief Gets the language name string from a Language enum.
 *
 * @param language The language type enum.
 * @return A const string with the language name or NULL if unknown.
 */
static const char *get_language_name(Language language) {
  switch (language) {
  case LANG_C:
    return "c";
  case LANG_CPP:
    return "cpp";
  case LANG_PYTHON:
    return "python";
  case LANG_JAVASCRIPT:
    return "javascript";
  case LANG_TYPESCRIPT:
    return "typescript";
  default:
    return NULL;
  }
}

/**
 * @brief Gets the index for a language in the query manager's arrays.
 *
 * @param manager The query manager instance.
 * @param language The language type to find.
 * @return The index in the arrays, or -1 if not found.
 */
static int get_language_index(const QueryManager *manager, Language language) {
  if (!manager || language == LANG_UNKNOWN) {
    return -1;
  }

  for (size_t i = 0; i < manager->language_count; i++) {
    if (manager->language_types[i] == language) {
      return (int)i;
    }
  }

  return -1; // Language not found
}

/**
 * @brief Searches for a cached query in the query manager.
 *
 * @param manager The query manager instance.
 * @param lang_idx The language index.
 * @param query_name The name of the query to find.
 * @return The index of the cached query, or -1 if not found.
 */
static int find_cached_query(const QueryManager *manager, int lang_idx, const char *query_name) {
  if (!manager || lang_idx < 0 || lang_idx >= (int)manager->language_count || !query_name) {
    return -1;
  }

  for (size_t i = 0; i < manager->query_counts[lang_idx]; i++) {
    if (manager->cached_queries[lang_idx][i].query_name &&
        strcmp(manager->cached_queries[lang_idx][i].query_name, query_name) == 0) {
      return (int)i;
    }
  }

  return -1; // Query not found in cache
}

/**
 * @brief Adds a compiled query to the cache.
 *
 * @param manager The query manager instance.
 * @param lang_idx The language index.
 * @param query_name The name of the query.
 * @param query The compiled Tree-sitter query.
 * @return true if the query was successfully cached, false otherwise.
 */
static bool cache_query(QueryManager *manager, int lang_idx, const char *query_name,
                        const TSQuery *query) {
  if (!manager || lang_idx < 0 || lang_idx >= (int)manager->language_count || !query_name ||
      !query) {
    return false;
  }

  // Check if we've reached the maximum number of queries for this language
  size_t query_count = manager->query_counts[lang_idx];
  if (query_count >= manager->max_queries_per_language) {
    fprintf(stderr, "Cannot cache query: maximum number of queries reached for language %d\n",
            lang_idx);
    return false;
  }

  // Add the query to the cache
  manager->cached_queries[lang_idx][query_count].query_name = safe_strdup(query_name);
  if (!manager->cached_queries[lang_idx][query_count].query_name) {
    return false;
  }

  manager->cached_queries[lang_idx][query_count].query = query;
  manager->query_counts[lang_idx]++;

  return true;
}

/**
 * @brief Gets a compiled Tree-sitter query by language and name.
 *
 * This function retrieves a compiled Tree-sitter query from the QueryManager.
 * It first checks if the query is already cached. If found in the cache, it returns
 * the cached query. Otherwise, it attempts to load the query file from disk,
 * compile it using the Tree-sitter API, cache it for future use, and then return
 * the compiled query.
 *
 * @param q_manager The QueryManager instance.
 * @param language The language for the query.
 * @param query_name The name of the query to retrieve.
 * @return A const pointer to the compiled TSQuery, or NULL if not found.
 */
const TSQuery *query_manager_get_query(QueryManager *q_manager, Language language,
                                       const char *query_name) {
  // Log entry into this function for debugging
  log_debug("ENTERING query_manager_get_query with language=%d, query_name='%s'", language,
            SAFE_STR(query_name));

  // Validate query manager
  if (!q_manager) {
    log_error("NULL query manager passed to query_manager_get_query");
    return NULL;
  }

  // Validate query name
  if (!query_name) {
    log_error("NULL query name passed to query_manager_get_query");
    return NULL;
  }

  // Validate query name is not empty
  if (query_name[0] == '\0') {
    log_error("Empty query name passed to query_manager_get_query");
    return NULL;
  }

  // Safety check for language bounds
  if (language < 0 || language >= q_manager->language_count) {
    log_error("Invalid language type (%d) passed to query_manager_get_query (max=%d)", language,
              q_manager->language_count - 1);
    return NULL;
  }

  // Check if query manager is properly initialized
  if (!q_manager->queries_dir) {
    log_error("Query manager not properly initialized (queries_dir is NULL)");
    return NULL;
  }

  // Direct debug output - only shown when debug mode is enabled
  if (DIRECT_DEBUG_MODE) {
    fprintf(stderr,
            "DIRECT DEBUG: Entered query_manager_get_query for language %d, query_name: %s\n",
            language, query_name ? query_name : "NULL");
    fflush(stderr);
  }

  // SPECIAL CASE: For "functions" query, try a simple fallback query if normal loading fails
  bool is_functions_query = (strcmp(query_name, "functions") == 0);

  if (is_functions_query) {
    fprintf(stderr, "[FUNCTIONS_DEBUG] *** DETECTED functions query request ***\n");
  }

  // Step 1: Find language index
  size_t lang_idx = get_language_index(q_manager, language);
  if (lang_idx == (size_t)-1) {
    fprintf(stderr, "Unsupported language %d in query_manager_get_query\n", language);
    return NULL;
  }

  // Check if corresponding Tree-sitter language object is available
  if (!q_manager->languages[lang_idx]) {
    fprintf(stderr, "No Tree-sitter language object available for language %d\n", language);
    return NULL;
  }

  // Step 2: Check if the query is already cached
  int query_idx = find_cached_query(q_manager, lang_idx, query_name);
  if (query_idx >= 0) {
    // Query found in cache, return it
    return (const TSQuery *)q_manager->cached_queries[lang_idx][query_idx].query;
  }

  // Step 3: Query not found in cache, need to load and compile it
  const TSLanguage *ts_language = q_manager->languages[lang_idx];
  if (!ts_language) {
    fprintf(stderr, "No Tree-sitter language object available for language %d\n", language);
    return NULL;
  }

  // Get the language name string for file path construction
  const char *lang_name = get_language_name(language);
  if (!lang_name) {
    fprintf(stderr, "Could not determine language name for language %d\n", language);
    return NULL;
  }

  // Construct the query file path
  char *query_path = construct_query_path(q_manager, lang_name, query_name);

  // If we couldn't find the query file and this is a functions query, try a simple fallback
  if (!query_path && is_functions_query) {
    fprintf(stderr, "[QUERY_DEBUG] Using simple fallback query for functions\n");

    // Simple query that should match any function definition or declaration
    const char *simple_query = "(function_definition) @function";

    uint32_t error_offset = 0;
    const TSQuery *query =
        compile_query(ts_language, simple_query, strlen(simple_query), &error_offset);

    if (query) {
      fprintf(stderr, "[QUERY_DEBUG] Successfully compiled simple fallback query\n");

      // Cache the compiled query for future use
      if (!cache_query(q_manager, lang_idx, query_name, query)) {
        fprintf(stderr, "Failed to cache simple fallback query\n");
        ts_query_delete((TSQuery *)query); // Cast away const
        return NULL;
      }

      return query;
    } else {
      fprintf(stderr, "[QUERY_DEBUG] Failed to compile simple fallback query\n");
    }

    return NULL;
  }

  if (!query_path) {
    fprintf(stderr, "Failed to construct query path for %s/%s\n", lang_name, query_name);
    return NULL;
  }

  // Load the query file content
  uint32_t content_len = 0;
  char *query_content = read_query_file(query_path, &content_len);

  // Temporarily commenting this out:
  // if (query_content) {
  //   fprintf(stderr, "[QUERY_DEBUG] Contents of '%s':\n%.*s\n", query_path, (int)content_len,
  //           query_content);
  // }
  // safe_free(query_path); // Free the path string as we don't need it anymore

  if (!query_content) {
    fprintf(stderr, "Failed to read query file for %s/%s\n", lang_name, query_name);
    return NULL;
  }

  // Compile the query
  uint32_t error_offset = 0;
  const TSQuery *query = compile_query(ts_language, query_content, content_len, &error_offset);
  safe_free(query_content); // Free the file content as we don't need it anymore

  if (!query) {
    fprintf(stderr, "Failed to compile query %s/%s\n", lang_name, query_name);
    return NULL;
  }

  // Cache the compiled query for future use
  if (!cache_query(q_manager, lang_idx, query_name, query)) {
    fprintf(stderr, "Failed to cache query %s/%s\n", lang_name, query_name);
    ts_query_delete((TSQuery *)query); // Cast away const
    return NULL;
  }

  return (const TSQuery *)query;
}
