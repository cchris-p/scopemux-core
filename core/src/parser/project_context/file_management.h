/**
 * @file file_management.h
 * @brief File management API for ProjectContext
 *
 * Declares file discovery, tracking, and path normalization functions for use by project context
 * modules.
 */

#ifndef FILE_MANAGEMENT_H
#define FILE_MANAGEMENT_H

#include "scopemux/project_context.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Path normalization
bool normalize_file_path(const char *project_root, const char *filepath, char *out_path,
                         size_t out_size);

// File discovery
bool add_discovered_file(ProjectContext *project, const char *filepath);
bool is_file_parsed(const ProjectContext *project, const char *filepath);

// File addition/removal
bool project_add_file_impl(ProjectContext *project, const char *filepath, Language language);
size_t project_add_directory_impl(ProjectContext *project, const char *dirpath,
                                  const char **extensions, bool recursive);

// File context accessor
ParserContext *project_get_file_context_impl(const ProjectContext *project, const char *filepath);
bool project_remove_file_impl(ProjectContext *project, const char *filepath);

#ifdef __cplusplus
}
#endif

#endif /* FILE_MANAGEMENT_H */
