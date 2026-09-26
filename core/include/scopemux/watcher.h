/**
 * @file watcher.h
 * @brief Filesystem watcher and change batching for incremental indexing (WI-018).
 *
 * The watcher performs recursive directory scans, debounces rapid writes, and
 * gates on file mtime/size plus a content hash so a no-op write (same content)
 * never triggers re-indexing. A scan produces a batch of create, modify, and
 * delete events; applying a batch updates the project and refreshes its derived
 * state incrementally.
 *
 * The current implementation is a portable polling scanner. The event-source
 * seam (`project_watcher_scan`) is deliberately independent of the OS so
 * inotify/FSEvents/ReadDirectoryChangesW sources can replace the scan later
 * without changing consumers.
 */

#ifndef SCOPEMUX_WATCHER_H
#define SCOPEMUX_WATCHER_H

#include "project_context.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Kind of filesystem change observed for a tracked file.
 */
typedef enum {
  PROJECT_WATCH_EVENT_CREATED = 0, ///< File appeared within the watched scope
  PROJECT_WATCH_EVENT_MODIFIED,    ///< File content changed
  PROJECT_WATCH_EVENT_DELETED,     ///< File disappeared from the watched scope
} ProjectWatchEventKind;

/**
 * @brief One debounced filesystem change.
 *
 * `file_path` is owned by the batch and freed by `project_watch_batch_free`.
 */
typedef struct {
  char *file_path;              ///< Path as reported by the scan
  ProjectWatchEventKind kind;   ///< Create, modify, or delete
  uint64_t content_hash;        ///< FNV-1a content hash (0 for deletions)
  long long mtime_ms;           ///< Last-modified time in milliseconds (0 for deletions)
  long size;                    ///< Size in bytes (0 for deletions)
} ProjectWatchEvent;

/**
 * @brief A batch of coalesced change events produced by one scan.
 */
typedef struct {
  ProjectWatchEvent *events; ///< Owned event array
  size_t event_count;        ///< Number of events in the batch
  size_t capacity;           ///< Allocated capacity
} ProjectWatchBatch;

/**
 * @brief Watcher configuration.
 *
 * `extensions` is a NULL-terminated list of lowercase extensions without the
 * leading dot (for example `{"c", "h", NULL}`). A NULL list tracks every
 * regular file. `debounce_ms` is the stability window: a change is only
 * emitted once its mtime/size signature has been unchanged for at least this
 * long, so a write burst collapses into one event.
 */
typedef struct {
  const char **extensions;
  bool recursive;
  long long debounce_ms;
} ProjectWatcherConfig;

/**
 * @brief Opaque watcher state.
 */
typedef struct ProjectWatcher ProjectWatcher;

/**
 * @brief Create a watcher rooted at a directory.
 *
 * @param root_directory Directory to scan (must be absolute or resolvable)
 * @param config Configuration, or NULL for defaults (all files, recursive, no debounce)
 * @return ProjectWatcher* Watcher, or NULL on invalid input or allocation failure
 */
ProjectWatcher *project_watcher_create(const char *root_directory,
                                       const ProjectWatcherConfig *config);

/**
 * @brief Free a watcher and its tracked-file state.
 *
 * @param watcher Watcher to free (NULL is a no-op)
 */
void project_watcher_free(ProjectWatcher *watcher);

/**
 * @brief Record the current files as the baseline without emitting events.
 *
 * Call once after creating the watcher so subsequent scans report only changes
 * that happen after baseline.
 *
 * @param watcher Watcher
 * @return size_t Number of files tracked, or 0 on failure
 */
size_t project_watcher_prime(ProjectWatcher *watcher);

/**
 * @brief Scan the watched scope and emit debounced change events.
 *
 * Uses the supplied wall-clock time for the debounce window so callers control
 * timing deterministically. A changed or created file is first held pending and
 * only emitted on a later scan whose `now_ms` is at least `debounce_ms` past the
 * time the change was first seen; a write with unchanged content is suppressed.
 *
 * @param watcher Watcher
 * @param now_ms Current time in milliseconds
 * @param out_batch Output batch; caller frees with `project_watch_batch_free`
 * @return bool True on success, false on failure
 */
bool project_watcher_scan(ProjectWatcher *watcher, long long now_ms,
                          ProjectWatchBatch *out_batch);

/**
 * @brief Free the owned storage of a batch.
 *
 * @param batch Batch to clear
 */
void project_watch_batch_free(ProjectWatchBatch *batch);

/**
 * @brief Apply a batch to a project and refresh derived state.
 *
 * Create/modify events re-parse the file in place; delete events remove it.
 * The project is then rebuilt (incrementally when the dirty set is retained).
 *
 * @param project Project context
 * @param batch Batch to apply
 * @param out_applied Optional count of files updated or removed
 * @return bool True on success, false on a fatal update/removal failure
 */
bool project_watcher_apply_batch(ProjectContext *project, const ProjectWatchBatch *batch,
                                 size_t *out_applied);

#ifdef __cplusplus
}
#endif

#endif /* SCOPEMUX_WATCHER_H */
