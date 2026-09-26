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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

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
    struct stat file_stat;
    long long mtime_ms;
    long size;
    int index;
    WatcherEntry *entry;

    if (stat(paths[i], &file_stat) != 0) {
      continue;
    }
    mtime_ms = watcher_stat_mtime_ms(&file_stat);
    size = (long)file_stat.st_size;

    index = watcher_find_entry(watcher, paths[i]);
    if (index < 0) {
      entry = watcher_add_entry(watcher, paths[i]);
      if (!entry) {
        ok = false;
        break;
      }
    } else {
      entry = &watcher->entries[index];
    }

    if (entry->seen_before && entry->mtime_ms == mtime_ms && entry->size == size) {
      entry->pending = false;
      continue;
    }

    if (entry->pending && entry->pending_mtime_ms == mtime_ms && entry->pending_size == size) {
      if (now_ms - entry->pending_since_ms >= watcher->debounce_ms) {
        uint64_t hash = watcher_hash_file(paths[i]);
        if (entry->seen_before && hash == entry->hash) {
          // Same bytes: a no-op write (for example an mtime-only touch).
          entry->mtime_ms = mtime_ms;
          entry->size = size;
          entry->pending = false;
        } else {
          ProjectWatchEventKind kind =
              entry->seen_before ? PROJECT_WATCH_EVENT_MODIFIED : PROJECT_WATCH_EVENT_CREATED;
          if (!watcher_batch_add(out_batch, paths[i], kind, hash, mtime_ms, size)) {
            ok = false;
            break;
          }
          entry->seen_before = true;
          entry->hash = hash;
          entry->mtime_ms = mtime_ms;
          entry->size = size;
          entry->pending = false;
        }
      }
      continue;
    }

    // New or changed signature: start (or restart) the stability window.
    entry->pending = true;
    entry->pending_mtime_ms = mtime_ms;
    entry->pending_size = size;
    entry->pending_since_ms = now_ms;
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
