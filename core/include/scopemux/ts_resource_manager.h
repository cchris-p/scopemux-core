/**
 * @file ts_resource_manager.h
 * @brief Resource management utilities for Tree-sitter resources
 *
 * This module provides safe lifecycle management for Tree-sitter resources,
 * helping to prevent leaks and memory corruption during shutdown.
 */

#ifndef SCOPEMUX_TS_RESOURCE_MANAGER_H
#define SCOPEMUX_TS_RESOURCE_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

// Forward declarations of Tree-sitter types to avoid direct inclusion
typedef struct TSParser TSParser;
typedef struct TSTree TSTree;
typedef struct TSQuery TSQuery;
typedef struct TSQueryCursor TSQueryCursor;

/**
 * @brief Resource manager for Tree-sitter objects
 *
 * This structure tracks all active Tree-sitter resources to ensure
 * proper cleanup during shutdown.
 */
typedef struct TSResourceManager TSResourceManager;

/**
 * @brief Create a new Tree-sitter resource manager
 *
 * @return TSResourceManager* New resource manager or NULL on failure
 */
TSResourceManager *ts_resource_manager_create(void);

/**
 * @brief Destroy a Tree-sitter resource manager and free all tracked resources
 *
 * @param manager Resource manager to destroy
 */
void ts_resource_manager_destroy(TSResourceManager *manager);

/**
 * @brief Create a new Tree-sitter parser with tracking
 *
 * @param manager Resource manager
 * @return TSParser* New parser or NULL on failure
 */
TSParser *ts_resource_manager_create_parser(TSResourceManager *manager);

/**
 * @brief Register an existing Tree-sitter parser with the manager
 *
 * @param manager Resource manager
 * @param parser Parser to register
 * @return true if registration was successful, false otherwise
 */
bool ts_resource_manager_register_parser(TSResourceManager *manager, TSParser *parser);

/**
 * @brief Remove a Tree-sitter parser from tracking without freeing it
 *
 * @param manager Resource manager
 * @param parser Parser to unregister
 * @return true if unregistration was successful, false otherwise
 */
bool ts_resource_manager_unregister_parser(TSResourceManager *manager, TSParser *parser);

/**
 * @brief Register a Tree-sitter tree with the manager
 *
 * @param manager Resource manager
 * @param tree Tree to register
 * @return true if registration was successful, false otherwise
 */
bool ts_resource_manager_register_tree(TSResourceManager *manager, TSTree *tree);

/**
 * @brief Unregister a Tree-sitter tree without freeing it
 *
 * @param manager Resource manager
 * @param tree Tree to unregister
 * @return true if unregistration was successful, false otherwise
 */
bool ts_resource_manager_unregister_tree(TSResourceManager *manager, TSTree *tree);

/**
 * @brief Create a new Tree-sitter query cursor with tracking
 *
 * @param manager Resource manager
 * @return TSQueryCursor* New query cursor or NULL on failure
 */
TSQueryCursor *ts_resource_manager_create_query_cursor(TSResourceManager *manager);

/**
 * @brief Register an existing Tree-sitter query cursor with the manager
 *
 * @param manager Resource manager
 * @param cursor Query cursor to register
 * @return true if registration was successful, false otherwise
 */
bool ts_resource_manager_register_query_cursor(TSResourceManager *manager, TSQueryCursor *cursor);

/**
 * @brief Register a Tree-sitter query with the manager
 *
 * @param manager Resource manager
 * @param query Query to register
 * @return true if registration was successful, false otherwise
 */
bool ts_resource_manager_register_query(TSResourceManager *manager, TSQuery *query);

/**
 * @brief Get the number of tracked resources by type
 *
 * @param manager Resource manager
 * @param parser_count Output parameter for parser count
 * @param tree_count Output parameter for tree count
 * @param query_count Output parameter for query count
 * @param cursor_count Output parameter for cursor count
 */
void ts_resource_manager_get_counts(TSResourceManager *manager, size_t *parser_count,
                                    size_t *tree_count, size_t *query_count, size_t *cursor_count);

/**
 * @brief Print statistics about tracked Tree-sitter resources
 *
 * @param manager Resource manager
 */
void ts_resource_manager_print_stats(TSResourceManager *manager);

/**
 * @brief Check if a Tree-sitter parser is valid and tracked
 *
 * @param manager Resource manager
 * @param parser Parser to check
 * @return true if parser is valid and tracked, false otherwise
 */
bool ts_resource_manager_is_valid_parser(TSResourceManager *manager, TSParser *parser);

/**
 * @brief Check if a Tree-sitter tree is valid and tracked
 *
 * @param manager Resource manager
 * @param tree Tree to check
 * @return true if tree is valid and tracked, false otherwise
 */
bool ts_resource_manager_is_valid_tree(TSResourceManager *manager, TSTree *tree);

/**
 * @brief Check if a Tree-sitter query is valid and tracked
 *
 * @param manager Resource manager
 * @param query Query to check
 * @return true if query is valid and tracked, false otherwise
 */
bool ts_resource_manager_is_valid_query(TSResourceManager *manager, TSQuery *query);

/**
 * @brief Check if a Tree-sitter query cursor is valid and tracked
 *
 * @param manager Resource manager
 * @param cursor Cursor to check
 * @return true if cursor is valid and tracked, false otherwise
 */
bool ts_resource_manager_is_valid_query_cursor(TSResourceManager *manager, TSQueryCursor *cursor);

#endif /* SCOPEMUX_TS_RESOURCE_MANAGER_H */
