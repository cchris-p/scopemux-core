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
#define DIRECT_DEBUG_MODE false

// Controls debugging output for query path resolution
// Shows paths attempted when loading .scm query files
#define QUERY_PATH_DEBUG_MODE false

#define _POSIX_C_SOURCE 200809L // For strdup

#include "../../include/scopemux/query_manager.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "scopemux/parser.h" // For LanguageType constants
#include "scopemux/query_manager.h"

// Forward declarations for Tree-sitter language functions from vendor library
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);
extern const TSLanguage *tree_sitter_python(void);
// JavaScript and TypeScript support removed temporarily
extern const TSLanguage *tree_sitter_javascript(void);
extern const TSLanguage *tree_sitter_typescript(void);

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
  LanguageType *language_types;     // Array of language type enums
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

  QueryManager *manager = (QueryManager *)calloc(1, sizeof(QueryManager));
  if (!manager) {
    return NULL;
  }

  manager->queries_dir = strdup(queries_dir);
  if (!manager->queries_dir) {
    free(manager);
    return NULL;
  }

  // Initialize the supported languages
  manager->language_count = MAX_LANGUAGES;
  manager->max_queries_per_language = 16; // Reasonable max number of queries per language

  // Allocate arrays for languages, language_types, cached_queries, and query_counts
  manager->languages = (const TSLanguage **)calloc(MAX_LANGUAGES, sizeof(TSLanguage *));
  manager->language_types = (LanguageType *)calloc(MAX_LANGUAGES, sizeof(LanguageType));
  manager->cached_queries = (QueryCacheEntry **)calloc(MAX_LANGUAGES, sizeof(QueryCacheEntry *));
  manager->query_counts = (size_t *)calloc(MAX_LANGUAGES, sizeof(size_t));

  // Check if all allocations succeeded
  if (!manager->languages || !manager->language_types || !manager->cached_queries ||
      !manager->query_counts) {
    query_manager_free(manager);
    return NULL;
  }

  // Initialize memory for query cache entries
  for (size_t i = 0; i < MAX_LANGUAGES; i++) {
    manager->cached_queries[i] =
        (QueryCacheEntry *)calloc(manager->max_queries_per_language, sizeof(QueryCacheEntry));
    if (!manager->cached_queries[i]) {
      query_manager_free(manager);
      return NULL;
    }
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
  manager->languages[1] = tree_sitter_c();
  manager->languages[2] = tree_sitter_cpp();
  manager->languages[3] = tree_sitter_python();
  manager->languages[4] = tree_sitter_javascript();
  manager->languages[5] = tree_sitter_typescript();

  // Initialize all query counts to 0
  for (size_t i = 0; i < MAX_LANGUAGES; i++) {
    manager->query_counts[i] = 0;
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
    free((void *)manager->queries_dir);
    manager->queries_dir = NULL;
  }

  // Free all cached queries and their names
  if (manager->cached_queries) {
    printf("[QUERY_MANAGER_FREE] Freeing cached queries array at %p (language_count=%zu)\n", 
           (void *)manager->cached_queries, manager->language_count);
    fflush(stdout);
    
    for (size_t i = 0; i < manager->language_count; i++) {
      if (manager->cached_queries[i]) {
        printf("[QUERY_MANAGER_FREE] Freeing cached queries for language %zu at %p (query_count=%zu)\n", 
               i, (void *)manager->cached_queries[i], manager->query_counts[i]);
        fflush(stdout);
        
        // Free each cached query for this language
        for (size_t j = 0; j < manager->query_counts[i]; j++) {
          if (manager->cached_queries[i][j].query_name) {
            printf("[QUERY_MANAGER_FREE] Freeing query name at %p: '%s'\n", 
                   (void *)manager->cached_queries[i][j].query_name, manager->cached_queries[i][j].query_name);
            fflush(stdout);
            free((void *)manager->cached_queries[i][j].query_name);
            manager->cached_queries[i][j].query_name = NULL;
          }
          
          if (manager->cached_queries[i][j].query) {
            printf("[QUERY_MANAGER_FREE] Freeing TSQuery at %p\n", 
                   (void *)manager->cached_queries[i][j].query);
            fflush(stdout);
            ts_query_delete((TSQuery *)manager->cached_queries[i][j].query); // Cast away const
            manager->cached_queries[i][j].query = NULL;
          }
        }
        
        // Free the array of QueryCacheEntry
        printf("[QUERY_MANAGER_FREE] Freeing QueryCacheEntry array at %p\n", 
               (void *)manager->cached_queries[i]);
        fflush(stdout);
        free(manager->cached_queries[i]);
        manager->cached_queries[i] = NULL;
      }
    }
    
    // Free the array of QueryCacheEntry pointers
    printf("[QUERY_MANAGER_FREE] Freeing cached_queries array at %p\n", 
           (void *)manager->cached_queries);
    fflush(stdout);
    free(manager->cached_queries);
    manager->cached_queries = NULL;
  }

  // Free the languages array
  if (manager->languages) {
    printf("[QUERY_MANAGER_FREE] Freeing languages array at %p\n", (void *)manager->languages);
    fflush(stdout);
    // Note: We don't free the language objects themselves as they are owned elsewhere
    free(manager->languages);
    manager->languages = NULL;
  }

  // Free the language types array
  if (manager->language_types) {
    printf("[QUERY_MANAGER_FREE] Freeing language_types array at %p\n", (void *)manager->language_types);
    fflush(stdout);
    free(manager->language_types);
    manager->language_types = NULL;
  }

  // Free the query counts array
  if (manager->query_counts) {
    printf("[QUERY_MANAGER_FREE] Freeing query_counts array at %p\n", (void *)manager->query_counts);
    fflush(stdout);
    free(manager->query_counts);
    manager->query_counts = NULL;
  }

  // Free the manager itself
  printf("[QUERY_MANAGER_FREE] Freeing manager struct at %p\n", (void *)manager);
  fflush(stdout);
  free(manager);
  
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
    return NULL;
  }

  // Calculate the required buffer size for the full path
  // queries_dir + "/" + language_name + "/" + query_name + ".scm" + "\0"
  size_t path_len = strlen(manager->queries_dir) + strlen(language_name) + strlen(query_name) +
                    7; // 7 for "/", "/", ".scm", and null terminator

  char *full_path = (char *)malloc(path_len);
  if (!full_path) {
    return NULL;
  }

  // Format the path string
  // First try project root relative path
  snprintf(full_path, path_len, "%s/%s/%s.scm", manager->queries_dir, language_name, query_name);

  // Debug path construction - only shown when query path debugging is enabled
  if (QUERY_PATH_DEBUG_MODE) {
    fprintf(stderr, "Trying query path: %s\n", full_path);
  }

  // Check if file exists
  if (access(full_path, F_OK) == -1) {
    // Try with /home/matrillo/apps/scopemux/queries/
    snprintf(full_path, path_len, "/home/matrillo/apps/scopemux/queries/%s/%s.scm", language_name,
             query_name);
    if (QUERY_PATH_DEBUG_MODE) {
      fprintf(stderr, "Trying alternative query path: %s\n", full_path);
    }
  }

  return full_path;
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
  char *content = (char *)malloc(file_size + 1); // +1 for null terminator
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
    free(content);
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
  if (!language || !query_str || query_len == 0) {
    return NULL;
  }

  // Compile the query using Tree-sitter
  uint32_t error_type;
  const TSQuery *query = ts_query_new(language, query_str, query_len, error_offset, &error_type);

  if (!query) {
    const char *error_types[] = {"None", "Syntax", "NodeType", "Field", "Capture"};
    const char *error_type_str = (error_type < 5) ? error_types[error_type] : "Unknown";

    fprintf(stderr, "Failed to compile query: %s error at offset %u\n", error_type_str,
            error_offset ? *error_offset : 0);

    // Print a snippet of the query string around the error location
    if (error_offset && *error_offset < query_len) {
      uint32_t start = *error_offset > 20 ? *error_offset - 20 : 0;
      uint32_t end = *error_offset + 20 < query_len ? *error_offset + 20 : query_len;

      fprintf(stderr, "Query snippet: ...%.*s[ERROR]%.*s...\n", *error_offset - start,
              query_str + start, end - *error_offset, query_str + *error_offset);
    }
  }

  return query;
}

/**
 * @brief Gets the language name string from a LanguageType enum.
 *
 * @param language The language type enum.
 * @return A const string with the language name or NULL if unknown.
 */
static const char *get_language_name(LanguageType language) {
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
static int get_language_index(const QueryManager *manager, LanguageType language) {
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
  manager->cached_queries[lang_idx][query_count].query_name = strdup(query_name);
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
const TSQuery *query_manager_get_query(QueryManager *q_manager, LanguageType language,
                                       const char *query_name) {
  if (!q_manager) {
    log_error("NULL query manager passed to query_manager_get_query");
    return NULL;
  }

  if (!query_name) {
    log_error("NULL query name passed to query_manager_get_query");
    return NULL;
  }

  // Safety check for language bounds
  if (language < 0 || language >= q_manager->language_count) {
    log_error("Invalid language type (%d) passed to query_manager_get_query", language);
    return NULL;
  }

  // Direct debug output - only shown when debug mode is enabled
  if (DIRECT_DEBUG_MODE) {
    fprintf(stderr,
            "DIRECT DEBUG: Entered query_manager_get_query for language %d, query_name: %s\n",
            language, query_name ? query_name : "NULL");
    fflush(stderr);
  }

  // Step 1: Find  // Check if language index is in range
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
  if (!query_path) {
    fprintf(stderr, "Failed to construct query path for %s/%s\n", lang_name, query_name);
    return NULL;
  }

  // Load the query file content
  uint32_t content_len = 0;
  char *query_content = read_query_file(query_path, &content_len);
  free(query_path); // Free the path string as we don't need it anymore

  if (!query_content) {
    fprintf(stderr, "Failed to read query file for %s/%s\n", lang_name, query_name);
    return NULL;
  }

  // Compile the query
  uint32_t error_offset = 0;
  const TSQuery *query = compile_query(ts_language, query_content, content_len, &error_offset);
  free(query_content); // Free the file content as we don't need it anymore

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
