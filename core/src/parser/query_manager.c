#include "../../include/scopemux/query_manager.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // For malloc, free
#include <string.h> // For strdup

// Forward declarations for Tree-sitter language functions
TSLanguage *tree_sitter_c(void);
TSLanguage *tree_sitter_cpp(void);
TSLanguage *tree_sitter_python(void);
TSLanguage *tree_sitter_javascript(void);
TSLanguage *tree_sitter_typescript(void);

// Represents a single cached query.
typedef struct CachedQuery {
  char *key;           // Composite key, e.g., "python_functions"
  const TSQuery *query;  // The compiled Tree-sitter query
  struct CachedQuery *next;
} CachedQuery;

// The full definition of the QueryManager struct.
struct QueryManager {
  char *queries_dir;        // Root directory for .scm files
  CachedQuery *cache_head; // Head of the linked list for the cache
};

// Helper to convert LanguageType enum to its string representation for paths.
static const char *language_to_string(LanguageType lang) {
  switch (lang) {
  case LANG_C: return "c";
  case LANG_CPP: return "cpp";
  case LANG_PYTHON: return "python";
  case LANG_JAVASCRIPT: return "javascript";
  case LANG_TYPESCRIPT: return "typescript";
  default: return NULL;
  }
}

// Helper to get the TSLanguage function pointer for a given LanguageType.
static TSLanguage *get_ts_language(LanguageType lang) {
  switch (lang) {
  case LANG_C: return tree_sitter_c();
  case LANG_CPP: return tree_sitter_cpp();
  case LANG_PYTHON: return tree_sitter_python();
  case LANG_JAVASCRIPT: return tree_sitter_javascript();
  case LANG_TYPESCRIPT: return tree_sitter_typescript();
  default: return NULL;
  }
}

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
  manager->cache_head = NULL;
  return manager;
}

void query_manager_free(QueryManager *manager) {
  if (!manager) {
    return;
  }
  CachedQuery *current = manager->cache_head;
  while (current) {
    CachedQuery *next = current->next;
    free(current->key);
    ts_query_delete((TSQuery *)current->query); // Cast away const
    free(current);
    current = next;
  }
  free(manager->queries_dir);
  free(manager);
}

// Helper to read an entire file into a string.
static char *read_file_content(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer = (char *)malloc(length + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  fread(buffer, 1, length, file);
  buffer[length] = '\0';
  fclose(file);
  return buffer;
}

const TSQuery *query_manager_get_query(QueryManager *manager, LanguageType language,
                                     const char *query_name) {
  if (!manager || !query_name) {
    return NULL;
  }

  const char *lang_str = language_to_string(language);
  if (!lang_str) {
    return NULL; // Unsupported language
  }

  // 1. Check cache first
  char key[256];
  snprintf(key, sizeof(key), "%s_%s", lang_str, query_name);
  CachedQuery *cached = manager->cache_head;
  while (cached) {
    if (strcmp(cached->key, key) == 0) {
      return cached->query;
    }
    cached = cached->next;
  }

  // 2. If not in cache, load from file
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s/%s.scm", manager->queries_dir, lang_str, query_name);

  char *source = read_file_content(path);
  if (!source) {
    fprintf(stderr, "Query file not found or could not be read: %s\n", path);
    return NULL;
  }

  // 3. Compile the query
  TSLanguage *ts_lang = get_ts_language(language);
  if (!ts_lang) {
    fprintf(stderr, "Query manager does not support the selected language.\n");
    free(source);
    return NULL;
  }
  uint32_t error_offset;
  TSQueryError error_type;
  TSQuery *new_query = ts_query_new(ts_lang, source, strlen(source), &error_offset, &error_type);
  free(source); // Free source now that it's been used

  if (!new_query) {
    fprintf(stderr, "Failed to compile query file: %s. Error type: %d at offset %u\n", path, error_type, error_offset);
    return NULL;
  }

  // 4. Add to cache
  CachedQuery *new_cached_item = (CachedQuery *)malloc(sizeof(CachedQuery));
  if (!new_cached_item) {
    ts_query_delete(new_query);
    return NULL;
  }
  new_cached_item->key = strdup(key);
  new_cached_item->query = new_query;
  new_cached_item->next = manager->cache_head;
  manager->cache_head = new_cached_item;

  return new_cached_item->query;
}
