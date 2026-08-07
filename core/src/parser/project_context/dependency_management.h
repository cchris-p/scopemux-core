/**
 * @file dependency_management.h
 * @brief Dependency tracking and include/import resolution API for ProjectContext
 *
 * Declares functions for extracting and processing includes, imports, and other inter-file
 * dependencies.
 */

#ifndef DEPENDENCY_MANAGEMENT_H
#define DEPENDENCY_MANAGEMENT_H

#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Extract and process includes/imports from a parsed file
void extract_and_process_includes(ProjectContext *project, ParserContext *ctx,
                                  const char *filepath);

// Dependency addition and parsing
bool project_add_dependency_impl(ProjectContext *project, const char *source_file,
                                 const char *target_file);
bool project_parse_all_files_impl(ProjectContext *project);
size_t project_get_dependencies_impl(const ProjectContext *project, const char *filepath,
                                     char ***out_dependencies);

#ifdef __cplusplus
}
#endif

#endif /* DEPENDENCY_MANAGEMENT_H */
