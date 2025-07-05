/**
 * @file project_context_facade.c
 * @brief Facade implementation for project context functions
 *
 * This file implements the public API functions for project context management,
 * delegating to internal implementation functions.
 */

#include "project_context_internal.h"
#include "scopemux/logging.h"
#include "scopemux/project_context.h"
#include <stdlib.h>
#include <string.h>

/**
 * Add a file to the project context
 * @note Implemented in project_context.c
 */
extern bool project_context_add_file(ProjectContext *project, const char *filepath,
                                     Language language);

/**
 * Remove a file from the project context
 * @note Implemented in project_context.c
 */
extern bool project_context_remove_file(ProjectContext *project, const char *filepath);

/**
 * Add a dependency between two files
 * @note Implemented in project_context.c
 */
extern bool project_context_add_dependency(ProjectContext *project, const char *source_file,
                                           const char *target_file);

/**
 * Get dependencies for a file
 * @note Implemented in project_context.c
 */
extern size_t project_context_get_dependencies(ProjectContext *project, const char *filepath,
                                               const char ***out_dependencies);
