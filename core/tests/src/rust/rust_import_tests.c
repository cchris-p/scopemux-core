/**
 * @file rust_import_tests.c
 * @brief Unit tests for Rust `use` path normalization and module discovery
 */

#include "scopemux/rust_import.h"
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void write_stub_file(const char *path) {
  FILE *file = fopen(path, "w");
  cr_assert_not_null(file, "Failed to create fixture file: %s", path);
  fputs("// fixture\n", file);
  fclose(file);
}

static void make_dir(const char *path) {
  cr_assert_eq(mkdir(path, 0700), 0, "Failed to create fixture directory: %s", path);
}

static void remove_tree(const char *path) {
  char command[1024];
  snprintf(command, sizeof(command), "rm -rf '%s'", path);
  system(command);
}

Test(rust_import, normalize_paths) {
  char out[256];

  cr_assert(rust_import_normalize("std::collections::HashMap", out, sizeof(out)));
  cr_assert_str_eq(out, "std::collections::HashMap");

  cr_assert(rust_import_normalize("crate::foo::bar::Baz", out, sizeof(out)));
  cr_assert_str_eq(out, "crate::foo::bar::Baz");
}

Test(rust_import, normalize_alias_group_glob_and_absolute) {
  char out[256];

  cr_assert(rust_import_normalize("foo::Bar as Baz", out, sizeof(out)));
  cr_assert_str_eq(out, "foo::Bar");

  cr_assert(rust_import_normalize("foo::{Bar, Baz}", out, sizeof(out)));
  cr_assert_str_eq(out, "foo");

  cr_assert(rust_import_normalize("foo::bar::*", out, sizeof(out)));
  cr_assert_str_eq(out, "foo::bar");

  cr_assert(rust_import_normalize("::std::io", out, sizeof(out)));
  cr_assert_str_eq(out, "std::io");

  cr_assert(rust_import_normalize("  std :: io ;  ", out, sizeof(out)));
  cr_assert_str_eq(out, "std::io");

  cr_assert_not(rust_import_normalize("", out, sizeof(out)));
  cr_assert_not(rust_import_normalize("{a, b}", out, sizeof(out)));
}

Test(rust_import, finds_local_module_files) {
  char tmpl[] = "/tmp/scopemux_rust_import_XXXXXX";
  char *dir = mkdtemp(tmpl);
  cr_assert_not_null(dir, "Failed to create temp directory");

  char src[512];
  char foo[512];
  char bar[512];
  char current[512];
  snprintf(src, sizeof(src), "%s/src", dir);
  snprintf(foo, sizeof(foo), "%s/src/foo", dir);
  snprintf(bar, sizeof(bar), "%s/src/foo/bar.rs", dir);
  snprintf(current, sizeof(current), "%s/src/lib.rs", dir);

  make_dir(src);
  make_dir(foo);
  write_stub_file(bar);

  char out[4 * RUST_IMPORT_MAX_PATH];
  char normalized[256];

  cr_assert(rust_import_normalize("crate::foo::bar::Baz", normalized, sizeof(normalized)));
  size_t found = rust_import_find_module_files(normalized, current, dir, out, RUST_IMPORT_MAX_PATH, 4);
  cr_assert_geq(found, 1, "Expected to find crate::foo::bar module file");

  bool saw_bar = false;
  for (size_t i = 0; i < found; i++) {
    if (strcmp(out + i * RUST_IMPORT_MAX_PATH, bar) == 0) {
      saw_bar = true;
    }
  }
  cr_assert(saw_bar, "Expected the bar.rs module path to be discovered");

  // External crate paths must not resolve to local files.
  cr_assert(rust_import_normalize("std::collections::HashMap", normalized, sizeof(normalized)));
  cr_assert_eq(
      rust_import_find_module_files(normalized, current, dir, out, RUST_IMPORT_MAX_PATH, 4), 0,
      "External crate path should not match local files");

  remove_tree(dir);
}

Test(rust_import, resolves_self_and_super) {
  char tmpl[] = "/tmp/scopemux_rust_import_XXXXXX";
  char *dir = mkdtemp(tmpl);
  cr_assert_not_null(dir, "Failed to create temp directory");

  char src[512];
  char foo[512];
  char mod_rs[512];
  char bar[512];
  char current[512];
  snprintf(src, sizeof(src), "%s/src", dir);
  snprintf(foo, sizeof(foo), "%s/src/foo", dir);
  snprintf(mod_rs, sizeof(mod_rs), "%s/src/foo/mod.rs", dir);
  snprintf(bar, sizeof(bar), "%s/src/foo/bar.rs", dir);
  snprintf(current, sizeof(current), "%s/src/foo/mod.rs", dir);

  make_dir(src);
  make_dir(foo);
  write_stub_file(mod_rs);
  write_stub_file(bar);

  char out[4 * RUST_IMPORT_MAX_PATH];
  char normalized[256];

  // self::bar resolves next to the current file.
  cr_assert(rust_import_normalize("self::bar", normalized, sizeof(normalized)));
  size_t found = rust_import_find_module_files(normalized, current, dir, out, RUST_IMPORT_MAX_PATH, 4);
  cr_assert_geq(found, 1, "Expected self::bar to resolve");
  cr_assert_str_eq(out, bar);

  // super::foo resolves in the parent module directory.
  cr_assert(rust_import_normalize("super::foo", normalized, sizeof(normalized)));
  found = rust_import_find_module_files(normalized, current, dir, out, RUST_IMPORT_MAX_PATH, 4);
  cr_assert_geq(found, 1, "Expected super::foo to resolve");
  bool saw_mod = false;
  for (size_t i = 0; i < found; i++) {
    if (strcmp(out + i * RUST_IMPORT_MAX_PATH, mod_rs) == 0) {
      saw_mod = true;
    }
  }
  cr_assert(saw_mod, "Expected src/foo/mod.rs to be discovered for super::foo");

  remove_tree(dir);
}
