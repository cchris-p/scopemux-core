/**
 * @file project_core.c
 * @brief Core implementation of ProjectContext lifecycle and management
 */

#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

#include "scopemux/logging.h"
#include "scopemux/project_context.h"
#include "scopemux/symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ProjectContext *project_context_create_impl(const char *root_directory);

void project_context_set_config_impl(ProjectContext *project, const ProjectConfig *config);

// Public API for setting and retrieving errors
void project_set_error(ProjectContext *project, int code, const char *message);
const char *project_get_error(const ProjectContext *project, int *out_code);

// Internal implementations
void project_set_error_impl(ProjectContext *project, int code, const char *message);
const char *project_get_error_impl(const ProjectContext *project, int *out_code);

void project_get_stats_impl(const ProjectContext *project, size_t *out_total_files,
                            size_t *out_total_symbols, size_t *out_total_references,
                            size_t *out_unresolved);

void project_context_free_impl(ProjectContext *project);

#endif // PROJECT_UTILS_H
