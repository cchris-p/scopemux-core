/**
 * @file ts_resource_manager.c
 * @brief Implementation of resource management utilities for Tree-sitter resources
 *
 * This module provides safe lifecycle management for Tree-sitter resources,
 * helping to prevent leaks and memory corruption during shutdown.
 */

#include "../../core/include/scopemux/ts_resource_manager.h"
#include "../../core/include/scopemux/logging.h"
#include "../../core/include/scopemux/memory_debug.h"
#include "../../core/include/tree_sitter/api.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of resources to track per type
#define MAX_TRACKED_PARSERS 16
#define MAX_TRACKED_TREES 256
#define MAX_TRACKED_QUERIES 128
#define MAX_TRACKED_CURSORS 64

/**
 * Resource manager structure that tracks all active Tree-sitter resources
 */
struct TSResourceManager {
  // Tracked resources
  TSParser *parsers[MAX_TRACKED_PARSERS];
  TSTree *trees[MAX_TRACKED_TREES];
  TSQuery *queries[MAX_TRACKED_QUERIES];
  TSQueryCursor *query_cursors[MAX_TRACKED_CURSORS];

  // Resource counts
  size_t parser_count;
  size_t tree_count;
  size_t query_count;
  size_t cursor_count;

  // Mutex for thread safety
  pthread_mutex_t mutex;
};

TSResourceManager *ts_resource_manager_create(void) {
  TSResourceManager *manager = calloc(1, sizeof(TSResourceManager));
  if (!manager) {
    log_error("Failed to allocate memory for TS resource manager");
    return NULL;
  }

  // Initialize mutex
  if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
    log_error("Failed to initialize mutex for TS resource manager");
    free(manager);
    return NULL;
  }

  log_info("Tree-sitter resource manager initialized");
  return manager;
}

void ts_resource_manager_destroy(TSResourceManager *manager) {
  if (!manager) {
    return;
  }

  pthread_mutex_lock(&manager->mutex);

  // Free all tracked resources
  for (size_t i = 0; i < manager->cursor_count; i++) {
    if (manager->query_cursors[i]) {
      ts_query_cursor_delete(manager->query_cursors[i]);
      manager->query_cursors[i] = NULL;
    }
  }

  for (size_t i = 0; i < manager->query_count; i++) {
    if (manager->queries[i]) {
      ts_query_delete(manager->queries[i]);
      manager->queries[i] = NULL;
    }
  }

  for (size_t i = 0; i < manager->tree_count; i++) {
    if (manager->trees[i]) {
      ts_tree_delete(manager->trees[i]);
      manager->trees[i] = NULL;
    }
  }

  for (size_t i = 0; i < manager->parser_count; i++) {
    if (manager->parsers[i]) {
      ts_parser_delete(manager->parsers[i]);
      manager->parsers[i] = NULL;
    }
  }

  // Reset counts
  manager->parser_count = 0;
  manager->tree_count = 0;
  manager->query_count = 0;
  manager->cursor_count = 0;

  pthread_mutex_unlock(&manager->mutex);

  // Destroy mutex and free manager
  pthread_mutex_destroy(&manager->mutex);
  free(manager);

  log_info("Tree-sitter resource manager destroyed");
}

TSParser *ts_resource_manager_create_parser(TSResourceManager *manager) {
  if (!manager) {
    log_error("NULL manager provided to ts_resource_manager_create_parser");
    return NULL;
  }

  pthread_mutex_lock(&manager->mutex);

  // Check if we have room for another parser
  if (manager->parser_count >= MAX_TRACKED_PARSERS) {
    log_error("Maximum number of tracked parsers (%d) reached", MAX_TRACKED_PARSERS);
    pthread_mutex_unlock(&manager->mutex);
    return NULL;
  }

  // Create parser
  TSParser *parser = ts_parser_new();
  if (!parser) {
    log_error("Failed to create Tree-sitter parser");
    pthread_mutex_unlock(&manager->mutex);
    return NULL;
  }

  // Add to tracked parsers
  manager->parsers[manager->parser_count++] = parser;

  pthread_mutex_unlock(&manager->mutex);
  return parser;
}

bool ts_resource_manager_register_parser(TSResourceManager *manager, TSParser *parser) {
  if (!manager || !parser) {
    log_error("NULL manager or parser provided to ts_resource_manager_register_parser");
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  // Check if parser is already registered
  for (size_t i = 0; i < manager->parser_count; i++) {
    if (manager->parsers[i] == parser) {
      pthread_mutex_unlock(&manager->mutex);
      return true; // Already registered
    }
  }

  // Check if we have room for another parser
  if (manager->parser_count >= MAX_TRACKED_PARSERS) {
    log_error("Maximum number of tracked parsers (%d) reached", MAX_TRACKED_PARSERS);
    pthread_mutex_unlock(&manager->mutex);
    return false;
  }

  // Add to tracked parsers
  manager->parsers[manager->parser_count++] = parser;

  pthread_mutex_unlock(&manager->mutex);
  return true;
}

bool ts_resource_manager_unregister_parser(TSResourceManager *manager, TSParser *parser) {
  if (!manager || !parser) {
    log_error("NULL manager or parser provided to ts_resource_manager_unregister_parser");
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  bool found = false;
  for (size_t i = 0; i < manager->parser_count; i++) {
    if (manager->parsers[i] == parser) {
      // Remove by shifting remaining elements
      for (size_t j = i; j < manager->parser_count - 1; j++) {
        manager->parsers[j] = manager->parsers[j + 1];
      }
      manager->parser_count--;
      found = true;
      break;
    }
  }

  pthread_mutex_unlock(&manager->mutex);
  return found;
}

bool ts_resource_manager_register_tree(TSResourceManager *manager, TSTree *tree) {
  if (!manager || !tree) {
    log_error("NULL manager or tree provided to ts_resource_manager_register_tree");
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  // Check if tree is already registered
  for (size_t i = 0; i < manager->tree_count; i++) {
    if (manager->trees[i] == tree) {
      pthread_mutex_unlock(&manager->mutex);
      return true; // Already registered
    }
  }

  // Check if we have room for another tree
  if (manager->tree_count >= MAX_TRACKED_TREES) {
    log_error("Maximum number of tracked trees (%d) reached", MAX_TRACKED_TREES);
    pthread_mutex_unlock(&manager->mutex);
    return false;
  }

  // Add to tracked trees
  manager->trees[manager->tree_count++] = tree;

  pthread_mutex_unlock(&manager->mutex);
  return true;
}

bool ts_resource_manager_unregister_tree(TSResourceManager *manager, TSTree *tree) {
  if (!manager || !tree) {
    log_error("NULL manager or tree provided to ts_resource_manager_unregister_tree");
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  bool found = false;
  for (size_t i = 0; i < manager->tree_count; i++) {
    if (manager->trees[i] == tree) {
      // Remove by shifting remaining elements
      for (size_t j = i; j < manager->tree_count - 1; j++) {
        manager->trees[j] = manager->trees[j + 1];
      }
      manager->tree_count--;
      found = true;
      break;
    }
  }

  pthread_mutex_unlock(&manager->mutex);
  return found;
}

TSQueryCursor *ts_resource_manager_create_query_cursor(TSResourceManager *manager) {
  if (!manager) {
    log_error("NULL manager provided to ts_resource_manager_create_query_cursor");
    return NULL;
  }

  pthread_mutex_lock(&manager->mutex);

  // Check if we have room for another query cursor
  if (manager->cursor_count >= MAX_TRACKED_CURSORS) {
    log_error("Maximum number of tracked query cursors (%d) reached", MAX_TRACKED_CURSORS);
    pthread_mutex_unlock(&manager->mutex);
    return NULL;
  }

  // Create query cursor
  TSQueryCursor *cursor = ts_query_cursor_new();
  if (!cursor) {
    log_error("Failed to create Tree-sitter query cursor");
    pthread_mutex_unlock(&manager->mutex);
    return NULL;
  }

  // Add to tracked query cursors
  manager->query_cursors[manager->cursor_count++] = cursor;

  pthread_mutex_unlock(&manager->mutex);
  return cursor;
}

bool ts_resource_manager_register_query_cursor(TSResourceManager *manager, TSQueryCursor *cursor) {
  if (!manager || !cursor) {
    log_error("NULL manager or cursor provided to ts_resource_manager_register_query_cursor");
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  // Check if cursor is already registered
  for (size_t i = 0; i < manager->cursor_count; i++) {
    if (manager->query_cursors[i] == cursor) {
      pthread_mutex_unlock(&manager->mutex);
      return true; // Already registered
    }
  }

  // Check if we have room for another cursor
  if (manager->cursor_count >= MAX_TRACKED_CURSORS) {
    log_error("Maximum number of tracked query cursors (%d) reached", MAX_TRACKED_CURSORS);
    pthread_mutex_unlock(&manager->mutex);
    return false;
  }

  // Add to tracked cursors
  manager->query_cursors[manager->cursor_count++] = cursor;

  pthread_mutex_unlock(&manager->mutex);
  return true;
}

bool ts_resource_manager_register_query(TSResourceManager *manager, TSQuery *query) {
  if (!manager || !query) {
    log_error("NULL manager or query provided to ts_resource_manager_register_query");
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  // Check if query is already registered
  for (size_t i = 0; i < manager->query_count; i++) {
    if (manager->queries[i] == query) {
      pthread_mutex_unlock(&manager->mutex);
      return true; // Already registered
    }
  }

  // Check if we have room for another query
  if (manager->query_count >= MAX_TRACKED_QUERIES) {
    log_error("Maximum number of tracked queries (%d) reached", MAX_TRACKED_QUERIES);
    pthread_mutex_unlock(&manager->mutex);
    return false;
  }

  // Add to tracked queries
  manager->queries[manager->query_count++] = query;

  pthread_mutex_unlock(&manager->mutex);
  return true;
}

void ts_resource_manager_get_counts(TSResourceManager *manager, size_t *parser_count,
                                    size_t *tree_count, size_t *query_count, size_t *cursor_count) {
  if (!manager) {
    if (parser_count)
      *parser_count = 0;
    if (tree_count)
      *tree_count = 0;
    if (query_count)
      *query_count = 0;
    if (cursor_count)
      *cursor_count = 0;
    return;
  }

  pthread_mutex_lock(&manager->mutex);

  if (parser_count)
    *parser_count = manager->parser_count;
  if (tree_count)
    *tree_count = manager->tree_count;
  if (query_count)
    *query_count = manager->query_count;
  if (cursor_count)
    *cursor_count = manager->cursor_count;

  pthread_mutex_unlock(&manager->mutex);
}

void ts_resource_manager_print_stats(TSResourceManager *manager) {
  if (!manager) {
    log_error("NULL manager provided to ts_resource_manager_print_stats");
    return;
  }

  pthread_mutex_lock(&manager->mutex);

  log_info("Tree-sitter resource statistics:");
  log_info("  Parsers: %zu/%d", manager->parser_count, MAX_TRACKED_PARSERS);
  log_info("  Trees: %zu/%d", manager->tree_count, MAX_TRACKED_TREES);
  log_info("  Queries: %zu/%d", manager->query_count, MAX_TRACKED_QUERIES);
  log_info("  Query Cursors: %zu/%d", manager->cursor_count, MAX_TRACKED_CURSORS);

  pthread_mutex_unlock(&manager->mutex);
}

bool ts_resource_manager_is_valid_parser(TSResourceManager *manager, TSParser *parser) {
  if (!manager || !parser) {
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  bool valid = false;
  for (size_t i = 0; i < manager->parser_count; i++) {
    if (manager->parsers[i] == parser) {
      valid = true;
      break;
    }
  }

  pthread_mutex_unlock(&manager->mutex);
  return valid;
}

bool ts_resource_manager_is_valid_tree(TSResourceManager *manager, TSTree *tree) {
  if (!manager || !tree) {
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  bool valid = false;
  for (size_t i = 0; i < manager->tree_count; i++) {
    if (manager->trees[i] == tree) {
      valid = true;
      break;
    }
  }

  pthread_mutex_unlock(&manager->mutex);
  return valid;
}

bool ts_resource_manager_is_valid_query(TSResourceManager *manager, TSQuery *query) {
  if (!manager || !query) {
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  bool valid = false;
  for (size_t i = 0; i < manager->query_count; i++) {
    if (manager->queries[i] == query) {
      valid = true;
      break;
    }
  }

  pthread_mutex_unlock(&manager->mutex);
  return valid;
}

bool ts_resource_manager_is_valid_query_cursor(TSResourceManager *manager, TSQueryCursor *cursor) {
  if (!manager || !cursor) {
    return false;
  }

  pthread_mutex_lock(&manager->mutex);

  bool valid = false;
  for (size_t i = 0; i < manager->cursor_count; i++) {
    if (manager->query_cursors[i] == cursor) {
      valid = true;
      break;
    }
  }

  pthread_mutex_unlock(&manager->mutex);
  return valid;
}
