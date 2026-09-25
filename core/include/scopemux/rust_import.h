#ifndef SCOPEMUX_RUST_IMPORT_H
#define SCOPEMUX_RUST_IMPORT_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum length of a single discovered Rust module path, including NUL.
 */
#define RUST_IMPORT_MAX_PATH 1024

/**
 * Normalize a raw Rust `use` argument into a canonical `::`-separated path.
 *
 * Handles alias clauses (`a::b as C` -> `a::b`), globs (`a::b::*` -> `a::b`),
 * grouped imports (`a::{b, c}` -> `a`), absolute leading `::`, surrounding
 * whitespace, and a trailing semicolon. Whitespace inside the path is removed
 * because Rust path segments cannot contain spaces.
 *
 * @param raw_use Raw captured use-argument text (may be NULL)
 * @param out Destination buffer for the normalized path
 * @param out_size Capacity of the destination buffer
 * @return true if a non-empty normalized path was produced
 */
bool rust_import_normalize(const char *raw_use, char *out, size_t out_size);

/**
 * Find existing local Rust module files referenced by a normalized use path.
 *
 * Candidate files are generated relative to the crate source directory inferred
 * from current_file (the directory containing `src`) and the file's own
 * directory. `crate`, `self`, and `super` prefixes are resolved; external crate
 * paths such as `std::...` yield no matches. Only paths that exist on disk are
 * returned, so no non-existent files are added to a project.
 *
 * @param normalized_use Path returned by rust_import_normalize
 * @param current_file Path of the file containing the use declaration
 * @param project_root Fallback project root directory (may be NULL)
 * @param out_paths Output buffer holding up to max_paths paths of path_size bytes each
 * @param path_size Size of each output path buffer
 * @param max_paths Maximum number of paths to write
 * @return Number of existing module files found
 */
size_t rust_import_find_module_files(const char *normalized_use, const char *current_file,
                                     const char *project_root, char *out_paths, size_t path_size,
                                     size_t max_paths);

#ifdef __cplusplus
}
#endif

#endif // SCOPEMUX_RUST_IMPORT_H
