/**
 * @file watcher_tests.c
 * @brief Tests for the polling filesystem watcher and change batching (WI-018).
 *
 * Timing is driven by an injected millisecond clock so debounce behavior is
 * deterministic; file mtimes are set explicitly so change detection does not
 * depend on filesystem timestamp resolution or wall-clock sleeps.
 */

#include "scopemux/project_context.h"
#include "scopemux/watcher.h"
#include <criterion/criterion.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char test_root[512];

static void join_path(const char *name, char *out, size_t out_size) {
  snprintf(out, out_size, "%s/%s", test_root, name);
}

static void set_mtime(const char *path, long long seconds) {
  struct timespec times[2];
  times[0].tv_sec = (time_t)seconds;
  times[0].tv_nsec = 0;
  times[1].tv_sec = (time_t)seconds;
  times[1].tv_nsec = 0;
  utimensat(AT_FDCWD, path, times, 0);
}

static void write_file(const char *name, const char *content, long long mtime_seconds) {
  char path[600];
  FILE *file;

  join_path(name, path, sizeof(path));
  file = fopen(path, "w");
  cr_assert(file != NULL, "Failed to write test file %s", path);
  fputs(content, file);
  fclose(file);
  set_mtime(path, mtime_seconds);
}

static void remove_file(const char *name) {
  char path[600];
  join_path(name, path, sizeof(path));
  remove(path);
}

static const char *event_basename(const ProjectWatchEvent *event) {
  const char *slash = event && event->file_path ? strrchr(event->file_path, '/') : NULL;
  return slash ? slash + 1 : (event ? event->file_path : NULL);
}

static void setup_watch(void) {
  char template[] = "/tmp/scopemux-watcher-XXXXXX";
  char *dir = mkdtemp(template);
  cr_assert(dir != NULL, "Failed to create temporary watch directory");
  strncpy(test_root, dir, sizeof(test_root) - 1);
  test_root[sizeof(test_root) - 1] = '\0';
}

static void teardown_watch(void) {
  remove_file("a.c");
  remove_file("b.c");
  remove_file("notes.txt");
  rmdir(test_root);
}

// WI-018: created, modified, and deleted files are detected, debounced, and
// coalesced into batches; a no-op write with identical content is suppressed.
Test(watcher, detects_create_modify_delete_and_suppresses_noop, .init = setup_watch,
     .fini = teardown_watch) {
  const char *extensions[] = {"c", NULL};
  ProjectWatcherConfig config = {0};
  ProjectWatcher *watcher;
  ProjectWatchBatch batch = {0};

  config.extensions = extensions;
  config.recursive = true;
  config.debounce_ms = 50;

  write_file("a.c", "int a;\n", 100);
  watcher = project_watcher_create(test_root, &config);
  cr_assert_not_null(watcher, "Watcher should be created");
  cr_assert_eq(project_watcher_prime(watcher), 1, "Baseline should track a.c");

  // No change on the first scan.
  cr_assert(project_watcher_scan(watcher, 1000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 0, "Unchanged baseline should produce no events");
  project_watch_batch_free(&batch);

  // Create b.c: the change is held until the debounce window elapses.
  write_file("b.c", "int b;\n", 200);
  cr_assert(project_watcher_scan(watcher, 2000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 0, "A newer change must wait for the stability window");
  project_watch_batch_free(&batch);

  cr_assert(project_watcher_scan(watcher, 2100, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 1, "Stable create should be emitted");
  cr_assert_eq(batch.events[0].kind, PROJECT_WATCH_EVENT_CREATED, "Event should be a create");
  cr_assert_str_eq(event_basename(&batch.events[0]), "b.c", "Created file should be b.c");
  project_watch_batch_free(&batch);

  // Modify a.c.
  write_file("a.c", "int a2;\n", 300);
  cr_assert(project_watcher_scan(watcher, 3000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 0, "Modify should wait for the stability window");
  project_watch_batch_free(&batch);

  cr_assert(project_watcher_scan(watcher, 3100, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 1, "Stable modify should be emitted");
  cr_assert_eq(batch.events[0].kind, PROJECT_WATCH_EVENT_MODIFIED, "Event should be a modify");
  cr_assert_str_eq(event_basename(&batch.events[0]), "a.c", "Modified file should be a.c");
  cr_assert(batch.events[0].content_hash != 0, "Modified event should carry a content hash");
  project_watch_batch_free(&batch);

  // Delete b.c.
  remove_file("b.c");
  cr_assert(project_watcher_scan(watcher, 4000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 1, "Delete should be emitted immediately");
  cr_assert_eq(batch.events[0].kind, PROJECT_WATCH_EVENT_DELETED, "Event should be a delete");
  cr_assert_str_eq(event_basename(&batch.events[0]), "b.c", "Deleted file should be b.c");
  project_watch_batch_free(&batch);

  // No-op write: same bytes, new mtime must not produce an event.
  write_file("a.c", "int a2;\n", 400);
  cr_assert(project_watcher_scan(watcher, 5000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 0, "No-op write should wait for the stability window");
  project_watch_batch_free(&batch);

  cr_assert(project_watcher_scan(watcher, 5100, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 0, "A write with identical content must not be emitted");
  project_watch_batch_free(&batch);

  project_watcher_free(watcher);
}

// WI-018: files that do not match the configured extensions are ignored.
Test(watcher, ignores_unmatched_extensions, .init = setup_watch, .fini = teardown_watch) {
  const char *extensions[] = {"c", NULL};
  ProjectWatcherConfig config = {0};
  ProjectWatcher *watcher;
  ProjectWatchBatch batch = {0};

  config.extensions = extensions;
  config.recursive = true;
  config.debounce_ms = 0;

  watcher = project_watcher_create(test_root, &config);
  cr_assert_not_null(watcher, "Watcher should be created");
  cr_assert_eq(project_watcher_prime(watcher), 0, "Baseline should track no files");

  write_file("notes.txt", "ignored\n", 100);
  cr_assert(project_watcher_scan(watcher, 1000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 0, "Matching-only scope should ignore notes.txt");
  project_watch_batch_free(&batch);

  write_file("a.c", "int a;\n", 200);
  cr_assert(project_watcher_scan(watcher, 2000, &batch), "Scan should succeed");
  project_watch_batch_free(&batch);
  cr_assert(project_watcher_scan(watcher, 2100, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 1, "Tracked .c file should be reported");
  cr_assert_str_eq(event_basename(&batch.events[0]), "a.c", "Reported file should be a.c");
  project_watch_batch_free(&batch);

  project_watcher_free(watcher);
}

// WI-018: applying a batch updates the project and refreshes its derived state.
Test(watcher, apply_batch_updates_project, .init = setup_watch, .fini = teardown_watch) {
  const char *extensions[] = {"c", NULL};
  ProjectWatcherConfig config = {0};
  ProjectWatcher *watcher;
  ProjectWatchBatch batch = {0};
  ProjectContext *project;
  const ProjectIRSnapshot *ir;
  char a_path[600];
  char b_path[600];
  bool changed = false;
  size_t applied = 0;

  config.extensions = extensions;
  config.recursive = true;
  config.debounce_ms = 50;

  write_file("a.c", "int alpha(void) { return 0; }\n", 100);
  watcher = project_watcher_create(test_root, &config);
  cr_assert_not_null(watcher, "Watcher should be created");
  cr_assert_eq(project_watcher_prime(watcher), 1, "Baseline should track a.c");

  join_path("a.c", a_path, sizeof(a_path));
  join_path("b.c", b_path, sizeof(b_path));

  project = project_context_create(test_root);
  cr_assert_not_null(project, "Project should be created");
  cr_assert(project_update_file(project, a_path, LANG_C, &changed), "a.c should parse");
  cr_assert(project_context_rebuild_ir(project), "Baseline IR should build");

  // Create b.c and apply the resulting batch.
  write_file("b.c", "int beta(void) { return 1; }\n", 200);
  cr_assert(project_watcher_scan(watcher, 2000, &batch), "Scan should succeed");
  project_watch_batch_free(&batch);
  cr_assert(project_watcher_scan(watcher, 2100, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 1, "b.c create should be reported");

  cr_assert(project_watcher_apply_batch(project, &batch, &applied), "Batch should apply");
  cr_assert_eq(applied, 1, "One file should be applied");
  cr_assert_eq(project->num_files, 2, "Project should now hold two files");
  cr_assert_not_null(project_get_file_context(project, b_path), "b.c should be tracked");
  ir = project_context_get_ir(project);
  cr_assert_not_null(ir, "IR should be ready after applying a batch");
  {
    bool saw_a = false;
    bool saw_b = false;
    for (size_t i = 0; i < ir->symbol_count; i++) {
      const char *file_path = ir->symbols[i].file_path;
      if (file_path && strcmp(file_path, a_path) == 0) {
        saw_a = true;
      }
      if (file_path && strcmp(file_path, b_path) == 0) {
        saw_b = true;
      }
    }
    cr_assert(saw_a, "IR should retain a symbol from a.c after an incremental apply");
    cr_assert(saw_b, "IR should include a symbol from the newly applied b.c");
  }
  project_watch_batch_free(&batch);

  // Delete a.c and apply the resulting batch.
  remove_file("a.c");
  cr_assert(project_watcher_scan(watcher, 3000, &batch), "Scan should succeed");
  cr_assert_eq(batch.event_count, 1, "a.c delete should be reported");
  cr_assert_eq(batch.events[0].kind, PROJECT_WATCH_EVENT_DELETED, "Event should be a delete");

  cr_assert(project_watcher_apply_batch(project, &batch, &applied), "Batch should apply");
  cr_assert_eq(applied, 1, "One removal should be applied");
  cr_assert_eq(project->num_files, 1, "Project should hold one file after the delete");
  cr_assert_null(project_get_file_context(project, a_path), "a.c should be gone");
  project_watch_batch_free(&batch);

  project_context_free(project);
  project_watcher_free(watcher);
}

#if defined(__linux__)

static long long native_test_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// Poll the native watcher until an event of the requested kind arrives or the
// timeout elapses. Optionally copies the matching path into out_path.
static bool native_poll_for(ProjectNativeWatcher *watcher, ProjectWatchEventKind kind,
                            char *out_path, size_t out_size, int timeout_ms) {
  long long start = native_test_now_ms();

  while (native_test_now_ms() - start < timeout_ms) {
    ProjectWatchBatch batch = {0};
    bool matched = false;

    if (!project_native_watcher_poll(watcher, 100, &batch)) {
      project_watch_batch_free(&batch);
      return false;
    }
    for (size_t i = 0; i < batch.event_count; i++) {
      if (batch.events[i].kind == kind && batch.events[i].file_path) {
        if (out_path && out_size > 0) {
          snprintf(out_path, out_size, "%s", batch.events[i].file_path);
        }
        matched = true;
        break;
      }
    }
    project_watch_batch_free(&batch);
    if (matched) {
      return true;
    }
  }
  return false;
}

// WI-018: the Linux inotify backend detects create, modify, and delete.
Test(watcher, native_inotify_detects_create_modify_delete, .init = setup_watch,
     .fini = teardown_watch) {
  const char *extensions[] = {"c", NULL};
  ProjectWatcherConfig config = {0};
  ProjectNativeWatcher *watcher;
  char event_path[600] = {0};

  config.extensions = extensions;
  config.recursive = true;
  config.debounce_ms = 0;

  write_file("a.c", "int a;\n", 100);
  watcher = project_native_watcher_create(test_root, &config);
  cr_assert_not_null(watcher, "inotify backend should be available on Linux");
  cr_assert_eq(project_native_watcher_backend(watcher), PROJECT_NATIVE_WATCHER_INOTIFY,
               "backend should be inotify");

  write_file("b.c", "int b;\n", 200);
  cr_assert(native_poll_for(watcher, PROJECT_WATCH_EVENT_CREATED, event_path, sizeof(event_path),
                            3000),
            "inotify should detect a created file");
  cr_assert(strstr(event_path, "b.c") != NULL, "created path should be b.c");

  write_file("a.c", "int a2;\n", 300);
  cr_assert(native_poll_for(watcher, PROJECT_WATCH_EVENT_MODIFIED, event_path, sizeof(event_path),
                            3000),
            "inotify should detect a modified file");
  cr_assert(strstr(event_path, "a.c") != NULL, "modified path should be a.c");

  remove_file("b.c");
  cr_assert(native_poll_for(watcher, PROJECT_WATCH_EVENT_DELETED, event_path, sizeof(event_path),
                            3000),
            "inotify should detect a deleted file");
  cr_assert(strstr(event_path, "b.c") != NULL, "deleted path should be b.c");

  project_native_watcher_free(watcher);
}

// WI-018: the native backend reuses the same no-op gating as the polling scan.
Test(watcher, native_inotify_suppresses_noop_write, .init = setup_watch, .fini = teardown_watch) {
  const char *extensions[] = {"c", NULL};
  ProjectWatcherConfig config = {0};
  ProjectNativeWatcher *watcher;

  config.extensions = extensions;
  config.recursive = true;
  config.debounce_ms = 0;

  write_file("a.c", "int a;\n", 100);
  watcher = project_native_watcher_create(test_root, &config);
  cr_assert_not_null(watcher, "inotify backend should be available on Linux");

  write_file("a.c", "int a;\n", 400);
  cr_assert_not(native_poll_for(watcher, PROJECT_WATCH_EVENT_MODIFIED, NULL, 0, 800),
                "a write with identical content must not be reported");

  project_native_watcher_free(watcher);
}

// WI-018: a native batch applies to a project like a polling batch.
Test(watcher, native_inotify_apply_updates_project, .init = setup_watch, .fini = teardown_watch) {
  const char *extensions[] = {"c", NULL};
  ProjectWatcherConfig config = {0};
  ProjectNativeWatcher *watcher;
  ProjectWatchBatch batch = {0};
  ProjectContext *project;
  char a_path[600];
  bool changed = false;
  size_t applied = 0;

  config.extensions = extensions;
  config.recursive = true;
  config.debounce_ms = 0;

  write_file("a.c", "int alpha(void) { return 0; }\n", 100);
  join_path("a.c", a_path, sizeof(a_path));

  project = project_context_create(test_root);
  cr_assert_not_null(project, "Project should be created");
  cr_assert(project_update_file(project, a_path, LANG_C, &changed), "a.c should parse");

  watcher = project_native_watcher_create(test_root, &config);
  cr_assert_not_null(watcher, "inotify backend should be available on Linux");

  write_file("b.c", "int beta(void) { return 1; }\n", 200);

  {
    long long start = native_test_now_ms();
    while (batch.event_count == 0 && native_test_now_ms() - start < 3000) {
      project_watch_batch_free(&batch);
      cr_assert(project_native_watcher_poll(watcher, 100, &batch), "Poll should succeed");
    }
    cr_assert(batch.event_count >= 1, "A committed create event should be available");
    cr_assert(project_watcher_apply_batch(project, &batch, &applied), "Batch should apply");
    cr_assert_eq(project->num_files, 2, "Project should hold two files");
    project_watch_batch_free(&batch);
  }

  project_context_free(project);
  project_native_watcher_free(watcher);
}

#endif /* __linux__ */
