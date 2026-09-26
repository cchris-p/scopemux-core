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
bool is_file_parsed(const ProjectContext *project, const char *filepath);

// Helper functions for dependency management
bool parser_context_add_dependency(ParserContext *source, ParserContext *target);

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

// Reverse-edge maintenance for incremental indexing. File contexts are linked by
// raw pointers, so a context that is replaced or removed must be repointed or
// scrubbed from every other file's dependency array to avoid dangling edges.
void project_context_repoint_dependency_target(ProjectContext *project, ParserContext *old_ctx,
                                               ParserContext *new_ctx);
void project_context_scrub_dependency_target(ProjectContext *project, ParserContext *target);

// Project-level IR lifecycle helpers
void project_context_clear_ir(ProjectContext *project);
bool project_context_rebuild_ir(ProjectContext *project);
void project_context_clear_info_blocks(ProjectContext *project);

// Incremental indexing (WI-018): mark a file changed/removed, expanding the
// dirty set through the retained reverse-edge index without recomputing yet.
void project_context_mark_file_dirty(ProjectContext *project, const char *filepath);

// Implementation functions for parser context
void parser_context_free(ParserContext *ctx);
bool parser_context_add_ast(ParserContext *ctx, ASTNode *node);
bool parser_context_add_ast_with_filename(ParserContext *ctx, ASTNode *node, const char *filename);

#endif /* PROJECT_CONTEXT_INTERNAL_H */
