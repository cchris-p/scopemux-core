/**
 * @file watcher.c
 * @brief Portable polling filesystem watcher for incremental indexing (WI-018).
 *
 * Scans a directory tree, detects create/modify/delete by comparing each file's
 * mtime and size against tracked state, debounces write bursts by requiring a
 * stable signature for a configurable window, and gates on a content hash so a
 * rewrite with identical bytes is not reported. The event-source seam is kept
 * independent of the OS so an inotify/FSEvents source can be added later.
 */

#include "scopemux/watcher.h"
#include "scopemux/language.h"
#include "scopemux/logging.h"
#include "scopemux/parser.h"
#include "scopemux/project_context.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

#if defined(__linux__)
#include <poll.h>
#include <sys/inotify.h>
#endif

/**
 * @brief Tracked state for one file within the watched scope.
 */
typedef struct {
  char *path;
  long long mtime_ms;
  long size;
  uint64_t hash;
  bool seen_before;
  bool pending;
  long long pending_mtime_ms;
  long pending_size;
  long long pending_since_ms;
} WatcherEntry;

struct ProjectWatcher {
  char *root_directory;
  bool recursive;
  long long debounce_ms;
  char **extensions;
  size_t extension_count;
  WatcherEntry *entries;
  size_t entry_count;
  size_t entry_capacity;
};

static const char *watcher_basename(const char *path) {
  const char *slash = path ? strrchr(path, '/') : NULL;
  return slash ? slash + 1 : path;
}

static long long watcher_stat_mtime_ms(const struct stat *st) {
  if (!st) {
    return 0;
  }
#if defined(__APPLE__)
  return (long long)st->st_mtimespec.tv_sec * 1000LL + st->st_mtimespec.tv_nsec / 1000000LL;
#else
  return (long long)st->st_mtim.tv_sec * 1000LL + st->st_mtim.tv_nsec / 1000000LL;
#endif
}

static bool watcher_extension_matches(const ProjectWatcher *watcher, const char *path) {
  const char *dot;
  size_t i;

  if (!watcher || !path) {
    return false;
  }
  if (watcher->extension_count == 0) {
    return true;
  }

  dot = strrchr(watcher_basename(path), '.');
  if (!dot || !dot[1]) {
    return false;
  }
  dot++;

  for (i = 0; i < watcher->extension_count; i++) {
#if defined(_WIN32)
    int cmp = _stricmp(watcher->extensions[i], dot);
#else
    int cmp = strcasecmp(watcher->extensions[i], dot);
#endif
    if (cmp == 0) {
      return true;
    }
  }
  return false;
}

static char *watcher_read_file(const char *path, size_t *out_length) {
  FILE *file;
  long size;
  char *buffer;
  size_t read_count;

  if (out_length) {
    *out_length = 0;
  }
  file = fopen(path, "rb");
  if (!file) {
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  size = ftell(file);
  if (size < 0) {
    fclose(file);
    return NULL;
  }
  rewind(file);

  buffer = (char *)malloc((size_t)size + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  read_count = fread(buffer, 1, (size_t)size, file);
  fclose(file);
  buffer[read_count] = '\0';
  if (out_length) {
    *out_length = read_count;
  }
  return buffer;
}

static uint64_t watcher_hash_file(const char *path) {
  size_t length = 0;
  char *content = watcher_read_file(path, &length);
  uint64_t hash;

  if (!content) {
    return 0;
  }
  hash = project_context_hash_content(content, length);
  free(content);
  return hash;
}

static bool watcher_list_add(char ***paths, size_t *count, size_t *capacity, const char *value) {
  if (*count >= *capacity) {
    size_t new_capacity = *capacity > 0 ? *capacity * 2 : 16;
    char **grown = (char **)realloc(*paths, new_capacity * sizeof(char *));
    if (!grown) {
      return false;
    }
    *paths = grown;
    *capacity = new_capacity;
  }
  (*paths)[*count] = strdup(value);
  if (!(*paths)[*count]) {
    return false;
  }
  (*count)++;
  return true;
}

static void watcher_list_free(char **paths, size_t count) {
  for (size_t i = 0; i < count; i++) {
    free(paths[i]);
  }
  free(paths);
}

static bool watcher_list_contains(char **paths, size_t count, const char *value) {
  for (size_t i = 0; i < count; i++) {
    if (paths[i] && strcmp(paths[i], value) == 0) {
      return true;
    }
  }
  return false;
}

static int watcher_find_entry(const ProjectWatcher *watcher, const char *path);
static WatcherEntry *watcher_add_entry(ProjectWatcher *watcher, const char *path);
static void watcher_remove_entry(ProjectWatcher *watcher, size_t index);
static bool watcher_batch_add(ProjectWatchBatch *batch, const char *path, ProjectWatchEventKind kind,
                              uint64_t hash, long long mtime_ms, long size);

#if defined(__linux__)
static void watcher_list_clear(char **paths, size_t *count) {
  if (!paths || !count) {
    return;
  }
  for (size_t i = 0; i < *count; i++) {
    free(paths[i]);
  }
  *count = 0;
}

/**
 * @brief Monotonic wall time in milliseconds.
 */
static long long watcher_now_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    return 0;
  }
  return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}
#endif

/**
 * @brief Evaluate one path against tracked state and emit a gated event.
 *
 * Handles create, modify, delete, debounce, mtime/size gating, and content-hash
 * no-op suppression. Missing paths produce a delete event when previously
 * tracked; paths outside the configured extensions are ignored unless already
 * tracked. Used by both the polling scan and the OS-event backend.
 */
static bool watcher_evaluate_path(ProjectWatcher *watcher, const char *path, long long now_ms,
                                  ProjectWatchBatch *out_batch) {
  struct stat file_stat;
  long long mtime_ms;
  long size;
  int index;
  WatcherEntry *entry;

  if (!watcher || !path || !out_batch) {
    return false;
  }

  if (stat(path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
    index = watcher_find_entry(watcher, path);
    if (index < 0) {
      return true;
    }
    if (!watcher_batch_add(out_batch, path, PROJECT_WATCH_EVENT_DELETED, 0, 0, 0)) {
      return false;
    }
    watcher_remove_entry(watcher, (size_t)index);
    return true;
  }

  index = watcher_find_entry(watcher, path);
  if (index < 0 && !watcher_extension_matches(watcher, path)) {
    return true;
  }

  mtime_ms = watcher_stat_mtime_ms(&file_stat);
  size = (long)file_stat.st_size;

  if (index < 0) {
    entry = watcher_add_entry(watcher, path);
    if (!entry) {
      return false;
    }
  } else {
    entry = &watcher->entries[index];
  }

  if (entry->seen_before && entry->mtime_ms == mtime_ms && entry->size == size) {
    entry->pending = false;
    return true;
  }

  if (entry->pending && entry->pending_mtime_ms == mtime_ms && entry->pending_size == size) {
    if (now_ms - entry->pending_since_ms >= watcher->debounce_ms) {
      uint64_t hash = watcher_hash_file(path);
      if (entry->seen_before && hash == entry->hash) {
        // Same bytes: a no-op write (for example an mtime-only touch).
        entry->mtime_ms = mtime_ms;
        entry->size = size;
        entry->pending = false;
      } else {
        ProjectWatchEventKind kind =
            entry->seen_before ? PROJECT_WATCH_EVENT_MODIFIED : PROJECT_WATCH_EVENT_CREATED;
        if (!watcher_batch_add(out_batch, path, kind, hash, mtime_ms, size)) {
          return false;
        }
        entry->seen_before = true;
        entry->hash = hash;
        entry->mtime_ms = mtime_ms;
        entry->size = size;
        entry->pending = false;
      }
    }
    return true;
  }

  // New or changed signature: start (or restart) the stability window.
  entry->pending = true;
  entry->pending_mtime_ms = mtime_ms;
  entry->pending_size = size;
  entry->pending_since_ms = now_ms;
  return true;
}

static bool watcher_collect_in_dir(const ProjectWatcher *watcher, const char *directory,
                                   char ***paths, size_t *count, size_t *capacity) {
  DIR *dir;
  struct dirent *entry;

  dir = opendir(directory);
  if (!dir) {
    return false;
  }

  while ((entry = readdir(dir)) != NULL) {
    char full_path[4096];
    struct stat file_stat;

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    if (snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name) >=
        (int)sizeof(full_path)) {
      continue;
    }
    if (stat(full_path, &file_stat) != 0) {
      continue;
    }

    if (S_ISDIR(file_stat.st_mode)) {
      if (watcher->recursive) {
        watcher_collect_in_dir(watcher, full_path, paths, count, capacity);
      }
    } else if (S_ISREG(file_stat.st_mode) && watcher_extension_matches(watcher, full_path)) {
      if (!watcher_list_add(paths, count, capacity, full_path)) {
        closedir(dir);
        return false;
      }
    }
  }

  closedir(dir);
  return true;
}

static int watcher_find_entry(const ProjectWatcher *watcher, const char *path) {
  for (size_t i = 0; i < watcher->entry_count; i++) {
    if (watcher->entries[i].path && strcmp(watcher->entries[i].path, path) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static WatcherEntry *watcher_add_entry(ProjectWatcher *watcher, const char *path) {
  WatcherEntry *entry;

  if (watcher->entry_count >= watcher->entry_capacity) {
    size_t new_capacity = watcher->entry_capacity > 0 ? watcher->entry_capacity * 2 : 16;
    WatcherEntry *grown =
        (WatcherEntry *)realloc(watcher->entries, new_capacity * sizeof(WatcherEntry));
    if (!grown) {
      return NULL;
    }
    watcher->entries = grown;
    watcher->entry_capacity = new_capacity;
  }

  entry = &watcher->entries[watcher->entry_count];
  memset(entry, 0, sizeof(*entry));
  entry->path = strdup(path);
  if (!entry->path) {
    return NULL;
  }
  watcher->entry_count++;
  return entry;
}

static void watcher_remove_entry(ProjectWatcher *watcher, size_t index) {
  if (index >= watcher->entry_count) {
    return;
  }
  free(watcher->entries[index].path);
  if (index != watcher->entry_count - 1) {
    watcher->entries[index] = watcher->entries[watcher->entry_count - 1];
  }
  watcher->entry_count--;
}

static bool watcher_batch_add(ProjectWatchBatch *batch, const char *path, ProjectWatchEventKind kind,
                              uint64_t hash, long long mtime_ms, long size) {
  ProjectWatchEvent *event;

  if (batch->event_count >= batch->capacity) {
    size_t new_capacity = batch->capacity > 0 ? batch->capacity * 2 : 8;
    ProjectWatchEvent *grown =
        (ProjectWatchEvent *)realloc(batch->events, new_capacity * sizeof(ProjectWatchEvent));
    if (!grown) {
      return false;
    }
    batch->events = grown;
    batch->capacity = new_capacity;
  }

  event = &batch->events[batch->event_count];
  event->file_path = strdup(path);
  if (!event->file_path) {
    return false;
  }
  event->kind = kind;
  event->content_hash = hash;
  event->mtime_ms = mtime_ms;
  event->size = size;
  batch->event_count++;
  return true;
}

ProjectWatcher *project_watcher_create(const char *root_directory,
                                       const ProjectWatcherConfig *config) {
  ProjectWatcher *watcher;

  if (!root_directory) {
    return NULL;
  }

  watcher = (ProjectWatcher *)malloc(sizeof(ProjectWatcher));
  if (!watcher) {
    return NULL;
  }
  memset(watcher, 0, sizeof(*watcher));

  watcher->root_directory = strdup(root_directory);
  if (!watcher->root_directory) {
    free(watcher);
    return NULL;
  }

  watcher->recursive = config ? config->recursive : true;
  watcher->debounce_ms = config ? config->debounce_ms : 0;

  if (config && config->extensions) {
    for (size_t i = 0; config->extensions[i]; i++) {
      char **grown = (char **)realloc(watcher->extensions,
                                      (watcher->extension_count + 1) * sizeof(char *));
      if (!grown) {
        project_watcher_free(watcher);
        return NULL;
      }
      watcher->extensions = grown;
      watcher->extensions[watcher->extension_count] = strdup(config->extensions[i]);
      if (!watcher->extensions[watcher->extension_count]) {
        project_watcher_free(watcher);
        return NULL;
      }
      watcher->extension_count++;
    }
  }

  return watcher;
}

void project_watcher_free(ProjectWatcher *watcher) {
  if (!watcher) {
    return;
  }
  for (size_t i = 0; i < watcher->extension_count; i++) {
    free(watcher->extensions[i]);
  }
  free(watcher->extensions);
  for (size_t i = 0; i < watcher->entry_count; i++) {
    free(watcher->entries[i].path);
  }
  free(watcher->entries);
  free(watcher->root_directory);
  free(watcher);
}

size_t project_watcher_prime(ProjectWatcher *watcher) {
  char **paths = NULL;
  size_t count = 0;
  size_t capacity = 0;

  if (!watcher) {
    return 0;
  }
  if (!watcher_collect_in_dir(watcher, watcher->root_directory, &paths, &count, &capacity)) {
    watcher_list_free(paths, count);
    return 0;
  }

  for (size_t i = 0; i < count; i++) {
    struct stat file_stat;
    WatcherEntry *entry;

    if (stat(paths[i], &file_stat) != 0) {
      continue;
    }
    entry = watcher_add_entry(watcher, paths[i]);
    if (!entry) {
      continue;
    }
    entry->seen_before = true;
    entry->mtime_ms = watcher_stat_mtime_ms(&file_stat);
    entry->size = (long)file_stat.st_size;
    entry->hash = watcher_hash_file(paths[i]);
  }

  watcher_list_free(paths, count);
  return watcher->entry_count;
}

bool project_watcher_scan(ProjectWatcher *watcher, long long now_ms, ProjectWatchBatch *out_batch) {
  char **paths = NULL;
  size_t count = 0;
  size_t capacity = 0;
  bool ok = true;

  if (!watcher || !out_batch) {
    return false;
  }
  memset(out_batch, 0, sizeof(*out_batch));

  if (!watcher_collect_in_dir(watcher, watcher->root_directory, &paths, &count, &capacity)) {
    watcher_list_free(paths, count);
    return false;
  }

  for (size_t i = 0; i < count; i++) {
    if (!watcher_evaluate_path(watcher, paths[i], now_ms, out_batch)) {
      ok = false;
      break;
    }
  }

  if (ok) {
    for (size_t i = watcher->entry_count; i > 0; i--) {
      size_t index = i - 1;
      WatcherEntry *entry = &watcher->entries[index];
      if (entry->path && !watcher_list_contains(paths, count, entry->path)) {
        if (!watcher_batch_add(out_batch, entry->path, PROJECT_WATCH_EVENT_DELETED, 0, 0, 0)) {
          ok = false;
          break;
        }
        watcher_remove_entry(watcher, index);
      }
    }
  }

  watcher_list_free(paths, count);
  return ok;
}

void project_watch_batch_free(ProjectWatchBatch *batch) {
  if (!batch) {
    return;
  }
  for (size_t i = 0; i < batch->event_count; i++) {
    free(batch->events[i].file_path);
  }
  free(batch->events);
  batch->events = NULL;
  batch->event_count = 0;
  batch->capacity = 0;
}

bool project_watcher_apply_batch(ProjectContext *project, const ProjectWatchBatch *batch,
                                 size_t *out_applied) {
  size_t applied = 0;
  bool ok = true;

  if (out_applied) {
    *out_applied = 0;
  }
  if (!project || !batch) {
    return false;
  }

  for (size_t i = 0; i < batch->event_count; i++) {
    const ProjectWatchEvent *event = &batch->events[i];
    if (!event->file_path) {
      continue;
    }

    if (event->kind == PROJECT_WATCH_EVENT_DELETED) {
      if (project_context_remove_file(project, event->file_path)) {
        applied++;
      }
    } else {
      bool changed = false;
      if (!project_update_file(project, event->file_path, LANG_UNKNOWN, &changed)) {
        ok = false;
        continue;
      }
      applied++;
    }
  }

  if (!project_context_rebuild_ir(project)) {
    ok = false;
  }

  if (out_applied) {
    *out_applied = applied;
  }
  return ok;
}

#if defined(__linux__)

#define NATIVE_WATCH_MASK                                                                          \
  (IN_CREATE | IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB |   \
   IN_DELETE_SELF | IN_MOVE_SELF)

typedef struct {
  int wd;
  char *path;
} NativeWatch;

struct ProjectNativeWatcher {
  ProjectWatcher *tracking;
  int inotify_fd;
  NativeWatch *watches;
  size_t watch_count;
  size_t watch_capacity;
  char **dirty;
  size_t dirty_count;
  size_t dirty_capacity;
  ProjectNativeWatcherBackend backend;
};

static int native_find_watch_index(const ProjectNativeWatcher *watcher, int wd) {
  for (size_t i = 0; i < watcher->watch_count; i++) {
    if (watcher->watches[i].wd == wd) {
      return (int)i;
    }
  }
  return -1;
}

static int native_find_watch_index_by_path(const ProjectNativeWatcher *watcher, const char *path) {
  for (size_t i = 0; i < watcher->watch_count; i++) {
    if (watcher->watches[i].path && strcmp(watcher->watches[i].path, path) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static bool native_watch_add(ProjectNativeWatcher *watcher, const char *dir, int wd) {
  NativeWatch *grown;

  if (watcher->watch_count >= watcher->watch_capacity) {
    size_t new_capacity = watcher->watch_capacity > 0 ? watcher->watch_capacity * 2 : 16;
    grown = (NativeWatch *)realloc(watcher->watches, new_capacity * sizeof(NativeWatch));
    if (!grown) {
      return false;
    }
    watcher->watches = grown;
    watcher->watch_capacity = new_capacity;
  }

  watcher->watches[watcher->watch_count].wd = wd;
  watcher->watches[watcher->watch_count].path = strdup(dir);
  if (!watcher->watches[watcher->watch_count].path) {
    return false;
  }
  watcher->watch_count++;
  return true;
}

static void native_watch_remove_at(ProjectNativeWatcher *watcher, size_t index) {
  if (index >= watcher->watch_count) {
    return;
  }
  free(watcher->watches[index].path);
  if (index != watcher->watch_count - 1) {
    watcher->watches[index] = watcher->watches[watcher->watch_count - 1];
  }
  watcher->watch_count--;
}

static bool native_add_watch_tree(ProjectNativeWatcher *watcher, const char *dir) {
  DIR *directory;
  struct dirent *entry;
  int wd;

  if (native_find_watch_index_by_path(watcher, dir) >= 0) {
    return true;
  }

  wd = inotify_add_watch(watcher->inotify_fd, dir, NATIVE_WATCH_MASK);
  if (wd < 0) {
    return false;
  }
  if (!native_watch_add(watcher, dir, wd)) {
    inotify_rm_watch(watcher->inotify_fd, wd);
    return false;
  }

  directory = opendir(dir);
  if (!directory) {
    return true;
  }
  while ((entry = readdir(directory)) != NULL) {
    char sub_path[4096];
    struct stat file_stat;

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    if (snprintf(sub_path, sizeof(sub_path), "%s/%s", dir, entry->d_name) >=
        (int)sizeof(sub_path)) {
      continue;
    }
    if (stat(sub_path, &file_stat) != 0) {
      continue;
    }
    if (S_ISDIR(file_stat.st_mode)) {
      native_add_watch_tree(watcher, sub_path);
    }
  }
  closedir(directory);
  return true;
}

static void native_remove_watch_tree(ProjectNativeWatcher *watcher, const char *dir) {
  size_t len = strlen(dir);

  for (size_t i = watcher->watch_count; i > 0; i--) {
    size_t index = i - 1;
    const char *path = watcher->watches[index].path;
    if (!path) {
      continue;
    }
    if (strcmp(path, dir) == 0 || (strncmp(path, dir, len) == 0 && path[len] == '/')) {
      inotify_rm_watch(watcher->inotify_fd, watcher->watches[index].wd);
      native_watch_remove_at(watcher, index);
    }
  }
}

static bool native_collect_files_into_dirty(ProjectNativeWatcher *watcher, const char *dir) {
  char **paths = NULL;
  size_t count = 0;
  size_t capacity = 0;
  bool ok = true;

  if (!watcher_collect_in_dir(watcher->tracking, dir, &paths, &count, &capacity)) {
    watcher_list_free(paths, count);
    return true;
  }
  for (size_t i = 0; i < count; i++) {
    if (!watcher_list_add(&watcher->dirty, &watcher->dirty_count, &watcher->dirty_capacity,
                          paths[i])) {
      ok = false;
      break;
    }
  }
  watcher_list_free(paths, count);
  return ok;
}

ProjectNativeWatcher *project_native_watcher_create(const char *root_directory,
                                                    const ProjectWatcherConfig *config) {
  ProjectNativeWatcher *watcher;

  if (!root_directory) {
    return NULL;
  }

  watcher = (ProjectNativeWatcher *)calloc(1, sizeof(*watcher));
  if (!watcher) {
    return NULL;
  }

  watcher->tracking = project_watcher_create(root_directory, config);
  if (!watcher->tracking) {
    free(watcher);
    return NULL;
  }
  project_watcher_prime(watcher->tracking);

  watcher->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (watcher->inotify_fd < 0) {
    project_watcher_free(watcher->tracking);
    free(watcher);
    return NULL;
  }

  if (!native_add_watch_tree(watcher, root_directory)) {
    close(watcher->inotify_fd);
    project_watcher_free(watcher->tracking);
    free(watcher);
    return NULL;
  }

  watcher->backend = PROJECT_NATIVE_WATCHER_INOTIFY;
  return watcher;
}

void project_native_watcher_free(ProjectNativeWatcher *watcher) {
  if (!watcher) {
    return;
  }
  for (size_t i = 0; i < watcher->watch_count; i++) {
    free(watcher->watches[i].path);
  }
  free(watcher->watches);
  watcher_list_clear(watcher->dirty, &watcher->dirty_count);
  free(watcher->dirty);
  if (watcher->inotify_fd >= 0) {
    close(watcher->inotify_fd);
  }
  project_watcher_free(watcher->tracking);
  free(watcher);
}

ProjectNativeWatcherBackend project_native_watcher_backend(const ProjectNativeWatcher *watcher) {
  return watcher ? watcher->backend : PROJECT_NATIVE_WATCHER_UNAVAILABLE;
}

bool project_native_watcher_poll(ProjectNativeWatcher *watcher, int timeout_ms,
                                 ProjectWatchBatch *out_batch) {
  char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
  struct pollfd pfd;
  int poll_result;
  bool ok = true;

  if (!watcher || !out_batch || watcher->backend != PROJECT_NATIVE_WATCHER_INOTIFY) {
    return false;
  }
  memset(out_batch, 0, sizeof(*out_batch));
  watcher_list_clear(watcher->dirty, &watcher->dirty_count);

  pfd.fd = watcher->inotify_fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  poll_result = poll(&pfd, 1, timeout_ms);
  if (poll_result < 0 && errno != EINTR) {
    return false;
  }

  if (poll_result > 0) {
    ssize_t length;
    while ((length = read(watcher->inotify_fd, buffer, sizeof(buffer))) > 0) {
      for (char *cursor = buffer; cursor < buffer + length;) {
        struct inotify_event *event = (struct inotify_event *)cursor;
        int watch_index = native_find_watch_index(watcher, event->wd);
        const char *dir = watch_index >= 0 ? watcher->watches[watch_index].path : NULL;
        char path[4096];

        if (dir && event->len > 0) {
          snprintf(path, sizeof(path), "%s/%s", dir, event->name);
        } else if (dir) {
          snprintf(path, sizeof(path), "%s", dir);
        } else {
          path[0] = '\0';
        }

        if (event->mask & IN_IGNORED) {
          if (watch_index >= 0) {
            native_watch_remove_at(watcher, (size_t)watch_index);
          }
        } else if (event->mask & IN_ISDIR) {
          if (path[0] && (event->mask & (IN_CREATE | IN_MOVED_TO))) {
            native_add_watch_tree(watcher, path);
            native_collect_files_into_dirty(watcher, path);
          } else if (path[0] &&
                     (event->mask & (IN_DELETE | IN_MOVED_FROM | IN_DELETE_SELF | IN_MOVE_SELF))) {
            native_remove_watch_tree(watcher, path);
          }
        } else if (path[0]) {
          if (!watcher_list_add(&watcher->dirty, &watcher->dirty_count, &watcher->dirty_capacity,
                                path)) {
            ok = false;
            break;
          }
        }

        cursor += sizeof(struct inotify_event) + event->len;
      }
      if (!ok) {
        break;
      }
    }
  }

  if (ok) {
    long long now_ms = watcher_now_ms();

    for (size_t i = 0; i < watcher->dirty_count && ok; i++) {
      if (!watcher_evaluate_path(watcher->tracking, watcher->dirty[i], now_ms, out_batch)) {
        ok = false;
      }
    }

    // Advance the debounce window for changes still pending from earlier polls.
    for (size_t i = 0; i < watcher->tracking->entry_count && ok; i++) {
      WatcherEntry *entry = &watcher->tracking->entries[i];
      if (entry->path && entry->pending) {
        if (!watcher_evaluate_path(watcher->tracking, entry->path, now_ms, out_batch)) {
          ok = false;
        }
      }
    }
  }

  watcher_list_clear(watcher->dirty, &watcher->dirty_count);
  return ok;
}

#else /* !__linux__ */

struct ProjectNativeWatcher {
  int unused;
};

ProjectNativeWatcher *project_native_watcher_create(const char *root_directory,
                                                    const ProjectWatcherConfig *config) {
  (void)root_directory;
  (void)config;
  return NULL;
}

void project_native_watcher_free(ProjectNativeWatcher *watcher) { (void)watcher; }

ProjectNativeWatcherBackend project_native_watcher_backend(const ProjectNativeWatcher *watcher) {
  (void)watcher;
  return PROJECT_NATIVE_WATCHER_UNAVAILABLE;
}

bool project_native_watcher_poll(ProjectNativeWatcher *watcher, int timeout_ms,
                                 ProjectWatchBatch *out_batch) {
  (void)watcher;
  (void)timeout_ms;
  (void)out_batch;
  return false;
}

#endif /* __linux__ */
