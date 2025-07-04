/**
 * @file project_context_internal.h
 * @brief Internal coordination header for project context implementation modules
 *
 * This header provides internal function declarations for the project context
 * implementation modules to coordinate between specialized components.
 * It is not part of the public API and should only be included by
 * project context implementation files.
 */

#ifndef PROJECT_CONTEXT_INTERNAL_H
#define PROJECT_CONTEXT_INTERNAL_H

#include "scopemux/parser.h"
#include "scopemux/project_context.h"
#include <stdbool.h>
#include <stddef.h>

// Helper functions for file management
bool normalize_file_path(const char *project_root, const char *filepath, char *out_path,
                         size_t out_size);
bool is_file_parsed(const ProjectContext *project, const char *filepath);

// Helper functions for dependency management
bool parser_context_add_dependency(ParserContext *source, ParserContext *target);
void register_file_symbols(ProjectContext *project, ParserContext *ctx, const char *filepath);

// Implementation functions from file_management.c
bool project_add_file_impl(ProjectContext *project, const char *filepath, Language language);
size_t project_add_directory_impl(ProjectContext *project, const char *dirpath,
                                  const char **extensions, bool recursive);
ParserContext *project_get_file_context_impl(const ProjectContext *project, const char *filepath);
bool project_remove_file_impl(ProjectContext *project, const char *filepath);

// Implementation functions from dependency_management.c
bool project_add_dependency_impl(ProjectContext *project, const char *source_file,
                                 const char *target_file);
bool project_parse_all_files_impl(ProjectContext *project);
void extract_and_process_includes(ProjectContext *project, ParserContext *ctx,
                                  const char *filepath);

// Implementation functions from project_utils.c
void project_set_error(ProjectContext *project, int code, const char *message);
const char *project_get_error(const ProjectContext *project, int *out_code);

// Implementation functions for parser context
ParserContext *parser_context_create(void);
bool parser_context_add_ast(ParserContext *ctx, ASTNode *node, const char *filename);

#endif /* PROJECT_CONTEXT_INTERNAL_H */
