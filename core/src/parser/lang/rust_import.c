/**
 * @file rust_import.c
 * @brief Rust `use` path normalization and local module-file discovery
 *
 * Rust resolves `use` paths through the crate module tree, not through file
 * paths in the source text. This module provides a conservative mapping from a
 * normalized `use` path to existing module files so dependency discovery can
 * follow local modules. It intentionally resolves only paths that exist on disk
 * and treats external crates (for example `std`, `serde`) as non-local.
 *
 * Known gaps (tracked on WI-030): `Cargo.toml`-driven crate roots, workspace
 * members, `#[path = "..."]` module attributes, and edition-dependent module
 * layouts (2015 `foo/mod.rs` vs 2018 `foo.rs`).
 */

#include "scopemux/rust_import.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define RUST_IMPORT_MAX_SEGMENTS 32

/**
 * @brief Copy the directory portion of a path (everything before the last '/').
 */
static void dirname_of(const char *path, char *out, size_t out_size) {
  if (!out || out_size == 0) {
    return;
  }

  snprintf(out, out_size, "%s", path ? path : "");

  char *slash = strrchr(out, '/');
  if (slash) {
    *slash = '\0';
  } else {
    out[0] = '\0';
  }
}

/**
 * @brief Infer the crate source directory by finding the last "/src" component.
 *
 * Returns false when the path has no source-directory component.
 */
static bool crate_source_dir(const char *file, char *out, size_t out_size) {
  const char *best = NULL;
  const char *cursor;

  if (!file || !out || out_size == 0) {
    return false;
  }

  cursor = file;
  while ((cursor = strstr(cursor, "/src/")) != NULL) {
    best = cursor;
    cursor += 1;
  }

  if (!best) {
    return false;
  }

  size_t len = (size_t)(best - file) + strlen("/src");
  if (len >= out_size) {
    return false;
  }

  memcpy(out, file, len);
  out[len] = '\0';
  return true;
}

/**
 * @brief Whether a path was already collected.
 */
static bool path_already_seen(const char *out_paths, size_t path_size, size_t count,
                              const char *candidate) {
  for (size_t i = 0; i < count; i++) {
    if (strcmp(out_paths + i * path_size, candidate) == 0) {
      return true;
    }
  }
  return false;
}

bool rust_import_normalize(const char *raw_use, char *out, size_t out_size) {
  if (!raw_use || !out || out_size == 0) {
    return false;
  }

  out[0] = '\0';

  const char *start = raw_use;
  while (*start && isspace((unsigned char)*start)) {
    start++;
  }

  // Absolute paths may be written with a leading `::`.
  if (start[0] == ':' && start[1] == ':') {
    start += 2;
  }

  // Stop at an alias clause, grouped import, glob, or statement terminator.
  const char *end = start;
  while (*end) {
    if (end[0] == ' ' && end[1] == 'a' && end[2] == 's' && end[3] == ' ') {
      break;
    }
    if (*end == '{' || *end == '*' || *end == ';' || *end == '\n' || *end == '\r') {
      break;
    }
    end++;
  }

  size_t written = 0;
  for (const char *p = start; p < end && written + 1 < out_size; p++) {
    if (isspace((unsigned char)*p)) {
      continue;
    }
    out[written++] = *p;
  }
  out[written] = '\0';

  // Drop trailing separators left by a truncated group or glob.
  while (written > 0 && out[written - 1] == ':') {
    out[--written] = '\0';
  }

  return written > 0;
}

size_t rust_import_find_module_files(const char *normalized_use, const char *current_file,
                                     const char *project_root, char *out_paths, size_t path_size,
                                     size_t max_paths) {
  if (!normalized_use || !out_paths || path_size == 0 || max_paths == 0) {
    return 0;
  }

  char buf[512];
  snprintf(buf, sizeof(buf), "%s", normalized_use);

  char *segments[RUST_IMPORT_MAX_SEGMENTS];
  size_t num_segments = 0;
  char *cursor = buf;
  while (cursor && *cursor && num_segments < RUST_IMPORT_MAX_SEGMENTS) {
    char *next = strstr(cursor, "::");
    if (next) {
      *next = '\0';
    }
    segments[num_segments++] = cursor;
    cursor = next ? next + 2 : NULL;
  }

  if (num_segments == 0) {
    return 0;
  }

  char crate_dir[RUST_IMPORT_MAX_PATH];
  char file_dir[RUST_IMPORT_MAX_PATH];
  bool have_crate = crate_source_dir(current_file, crate_dir, sizeof(crate_dir));
  dirname_of(current_file, file_dir, sizeof(file_dir));

  char bases[3][RUST_IMPORT_MAX_PATH];
  size_t num_bases = 0;
  size_t start = 0;

  if (strcmp(segments[0], "crate") == 0) {
    if (have_crate) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", crate_dir);
    } else if (project_root) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", project_root);
    }
    start = 1;
  } else if (strcmp(segments[0], "self") == 0) {
    if (file_dir[0]) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", file_dir);
    }
    start = 1;
  } else if (strcmp(segments[0], "super") == 0) {
    char base[RUST_IMPORT_MAX_PATH];
    // The first `super` moves from the file's directory to its parent.
    dirname_of(file_dir, base, sizeof(base));
    start = 1;
    while (start < num_segments && strcmp(segments[start], "super") == 0) {
      char parent[RUST_IMPORT_MAX_PATH];
      dirname_of(base, parent, sizeof(parent));
      snprintf(base, sizeof(base), "%s", parent);
      start++;
    }
    if (base[0]) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", base);
    }
  } else {
    // No explicit root: try the crate source dir, the file's own dir, then the
    // project root. External crate paths simply match nothing.
    if (have_crate) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", crate_dir);
    }
    if (file_dir[0]) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", file_dir);
    }
    if (project_root) {
      snprintf(bases[num_bases++], RUST_IMPORT_MAX_PATH, "%s", project_root);
    }
  }

  size_t count = 0;
  for (size_t b = 0; b < num_bases && count < max_paths; b++) {
    for (size_t k = num_segments - start; k >= 1 && count < max_paths; k--) {
      char rel[512];
      size_t rel_len = 0;
      rel[0] = '\0';

      for (size_t s = 0; s < k; s++) {
        int written = snprintf(rel + rel_len, sizeof(rel) - rel_len, "%s%s", s ? "/" : "",
                               segments[start + s]);
        if (written < 0 || (size_t)written >= sizeof(rel) - rel_len) {
          break;
        }
        rel_len += (size_t)written;
      }

      char candidates[2][RUST_IMPORT_MAX_PATH];
      snprintf(candidates[0], sizeof(candidates[0]), "%s/%s.rs", bases[b], rel);
      snprintf(candidates[1], sizeof(candidates[1]), "%s/%s/mod.rs", bases[b], rel);

      for (size_t c = 0; c < 2 && count < max_paths; c++) {
        if (access(candidates[c], F_OK) != 0) {
          continue;
        }
        if (path_already_seen(out_paths, path_size, count, candidates[c])) {
          continue;
        }
        snprintf(out_paths + count * path_size, path_size, "%s", candidates[c]);
        count++;
      }
    }
  }

  return count;
}
