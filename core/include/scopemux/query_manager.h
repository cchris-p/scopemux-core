#ifndef SCOPEMUX_QUERY_MANAGER_H
#define SCOPEMUX_QUERY_MANAGER_H

#include "parser.h" // For LanguageType
#include <tree_sitter/api.h>

// Forward declaration of the QueryManager structure.
// The full definition will be in the .c file to hide implementation details.
typedef struct QueryManager QueryManager;

/**
 * @brief Initializes the query manager.
 *
 * This function creates and initializes the query manager, which is responsible
 * for loading and caching Tree-sitter queries from .scm files. The manager
 * should be freed using query_manager_free when no longer needed.
 *
 * @param queries_dir The root directory path where language-specific query
 *                    files (e.g., 'queries/python/functions.scm') are stored.
 * @return A pointer to the newly created QueryManager, or NULL on failure.
 */
QueryManager *query_manager_init(const char *queries_dir);

/**
 * @brief Frees all resources associated with the query manager.
 *
 * This includes freeing any cached queries and the manager struct itself.
 *
 * @param manager The query manager to free.
 */
void query_manager_free(QueryManager *manager);

/**
 * @brief Retrieves a compiled Tree-sitter query for a specific language.
 *
 * This function loads a query from a file (e.g., 'queries/python/functions.scm')
 * on its first request, compiles it, and caches it for subsequent use.
 * The returned query object is owned by the manager and should not be freed
 * by the caller.
 *
 * @param manager The query manager instance.
 * @param language The programming language for which to get the query.
 * @param query_name The name of the query to retrieve (e.g., "functions", "classes").
 * @return A const pointer to the compiled TSQuery, or NULL if the query
 *         could not be found, loaded, or compiled.
 */
const TSQuery *query_manager_get_query(QueryManager *manager, LanguageType language,
                                       const char *query_name);

#endif // SCOPEMUX_QUERY_MANAGER_H
