/**
 * @file crash_handler.c
 * @brief Implementation of signal handling and crash protection utilities
 *
 * This module provides signal handlers to catch and diagnose crashes
 * like segmentation faults, with optional backtrace and context logging.
 */

#include "../../core/include/scopemux/crash_handler.h"
#include "../../core/include/scopemux/logging.h"
#include "../../core/include/scopemux/memory_debug.h"
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CRASH_CONTEXT 16
#define MAX_CRASH_CALLBACKS 8

static struct {
  CrashHandlerConfig config;
  struct sigaction old_segv;
  struct sigaction old_abrt;
  struct sigaction old_fpe;
  struct sigaction old_bus;
  struct sigaction old_ill;
  char *contexts[MAX_CRASH_CONTEXT];
  uint32_t context_ids[MAX_CRASH_CONTEXT];
  size_t context_count;
  struct {
    void (*callback)(void *);
    void *user_data;
    uint32_t id;
  } callbacks[MAX_CRASH_CALLBACKS];
  size_t callback_count;
  pthread_mutex_t lock;
  bool installed;
  uint32_t next_context_id;
  uint32_t next_callback_id;
} crash_state = {.context_count = 0,
                 .callback_count = 0,
                 .installed = false,
                 .next_context_id = 1,
                 .next_callback_id = 1,
                 .lock = PTHREAD_MUTEX_INITIALIZER};

static void crash_handler_signal(int signo, siginfo_t *info, void *context) {
  // Print signal info
  const char *signame = signo == SIGSEGV   ? "SIGSEGV"
                        : signo == SIGABRT ? "SIGABRT"
                        : signo == SIGFPE  ? "SIGFPE"
                        : signo == SIGBUS  ? "SIGBUS"
                        : signo == SIGILL  ? "SIGILL"
                                           : "UNKNOWN";
  log_error("Caught signal %s (%d) at address %p", SAFE_STR(signame), signo,
            info ? info->si_addr : NULL);

  // Print context stack
  pthread_mutex_lock(&crash_state.lock);
  if (crash_state.context_count > 0) {
    log_error("Crash context stack:");
    for (size_t i = 0; i < crash_state.context_count; i++) {
      log_error("  [%u] %s", crash_state.context_ids[i], SAFE_STR(crash_state.contexts[i]));
    }
  }
  pthread_mutex_unlock(&crash_state.lock);

  // Print backtrace
  if (crash_state.config.log_backtrace) {
    crash_handler_print_backtrace(32);
  }

  // Call registered callbacks
  pthread_mutex_lock(&crash_state.lock);
  for (size_t i = 0; i < crash_state.callback_count; i++) {
    if (crash_state.callbacks[i].callback) {
      crash_state.callbacks[i].callback(crash_state.callbacks[i].user_data);
    }
  }
  pthread_mutex_unlock(&crash_state.lock);

  // Optionally attempt recovery
  if (crash_state.config.attempt_recovery) {
    log_error("Attempting to recover from crash (may be unsafe)");
    return;
  }

  // Otherwise, abort
  _exit(128 + signo);
}

static void install_signal_handler(int signo, struct sigaction *oldact) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = crash_handler_signal;
  sa.sa_flags = SA_SIGINFO | SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(signo, &sa, oldact);
}

bool crash_handler_init(const CrashHandlerConfig *config) {
  pthread_mutex_lock(&crash_state.lock);
  if (crash_state.installed) {
    pthread_mutex_unlock(&crash_state.lock);
    return true;
  }
  if (config) {
    crash_state.config = *config;
  } else {
    crash_state.config = crash_handler_get_default_config();
  }
  if (crash_state.config.handle_segv) {
    install_signal_handler(SIGSEGV, &crash_state.old_segv);
  }
  if (crash_state.config.handle_abrt) {
    install_signal_handler(SIGABRT, &crash_state.old_abrt);
  }
  if (crash_state.config.handle_fpe) {
    install_signal_handler(SIGFPE, &crash_state.old_fpe);
  }
  if (crash_state.config.handle_bus) {
    install_signal_handler(SIGBUS, &crash_state.old_bus);
  }
  if (crash_state.config.handle_ill) {
    install_signal_handler(SIGILL, &crash_state.old_ill);
  }
  crash_state.installed = true;
  pthread_mutex_unlock(&crash_state.lock);
  log_info("Crash handler initialized");
  return true;
}

void crash_handler_cleanup(void) {
  pthread_mutex_lock(&crash_state.lock);
  if (!crash_state.installed) {
    pthread_mutex_unlock(&crash_state.lock);
    return;
  }
  if (crash_state.config.handle_segv) {
    sigaction(SIGSEGV, &crash_state.old_segv, NULL);
  }
  if (crash_state.config.handle_abrt) {
    sigaction(SIGABRT, &crash_state.old_abrt, NULL);
  }
  if (crash_state.config.handle_fpe) {
    sigaction(SIGFPE, &crash_state.old_fpe, NULL);
  }
  if (crash_state.config.handle_bus) {
    sigaction(SIGBUS, &crash_state.old_bus, NULL);
  }
  if (crash_state.config.handle_ill) {
    sigaction(SIGILL, &crash_state.old_ill, NULL);
  }
  crash_state.installed = false;
  pthread_mutex_unlock(&crash_state.lock);
  log_info("Crash handler cleaned up");
}

void crash_handler_print_backtrace(int max_frames) {
  void *buffer[64];
  int nptrs = backtrace(buffer, max_frames > 0 && max_frames < 64 ? max_frames : 64);
  char **strings = backtrace_symbols(buffer, nptrs);
  if (strings) {
    log_error("Backtrace:");
    for (int i = 0; i < nptrs; i++) {
      log_error("  %s", SAFE_STR(strings[i]));
    }
    FREE(strings);
  }
}

bool crash_handler_is_safe_ptr(const void *ptr) {
  // Only basic check: NULL
  return ptr != NULL;
}

void *crash_handler_safe_deref(void *ptr, void *fallback) {
  if (!crash_handler_is_safe_ptr(ptr)) {
    return fallback;
  }
  return ptr;
}

void crash_handler_set_thread_name(const char *name) {
#if defined(__linux__)
  pthread_setname_np(pthread_self(), name);
#endif
}

uint32_t crash_handler_push_context(const char *context_info) {
  pthread_mutex_lock(&crash_state.lock);
  if (crash_state.context_count >= MAX_CRASH_CONTEXT) {
    pthread_mutex_unlock(&crash_state.lock);
    return 0;
  }
  size_t idx = crash_state.context_count++;
  crash_state.contexts[idx] = STRDUP(context_info ? context_info : "(unknown)", "crash_context");
  crash_state.context_ids[idx] = crash_state.next_context_id++;
  uint32_t id = crash_state.context_ids[idx];
  pthread_mutex_unlock(&crash_state.lock);
  return id;
}

void crash_handler_pop_context(uint32_t context_id) {
  pthread_mutex_lock(&crash_state.lock);
  for (size_t i = 0; i < crash_state.context_count; i++) {
    if (crash_state.context_ids[i] == context_id) {
      FREE(crash_state.contexts[i]);
      for (size_t j = i; j < crash_state.context_count - 1; j++) {
        crash_state.contexts[j] = crash_state.contexts[j + 1];
        crash_state.context_ids[j] = crash_state.context_ids[j + 1];
      }
      crash_state.context_count--;
      break;
    }
  }
  pthread_mutex_unlock(&crash_state.lock);
}

uint32_t crash_handler_register_callback(void (*callback)(void *), void *user_data) {
  pthread_mutex_lock(&crash_state.lock);
  if (crash_state.callback_count >= MAX_CRASH_CALLBACKS) {
    pthread_mutex_unlock(&crash_state.lock);
    return 0;
  }
  size_t idx = crash_state.callback_count++;
  crash_state.callbacks[idx].callback = callback;
  crash_state.callbacks[idx].user_data = user_data;
  crash_state.callbacks[idx].id = crash_state.next_callback_id++;
  uint32_t id = crash_state.callbacks[idx].id;
  pthread_mutex_unlock(&crash_state.lock);
  return id;
}

void crash_handler_unregister_callback(uint32_t registration_id) {
  pthread_mutex_lock(&crash_state.lock);
  for (size_t i = 0; i < crash_state.callback_count; i++) {
    if (crash_state.callbacks[i].id == registration_id) {
      for (size_t j = i; j < crash_state.callback_count - 1; j++) {
        crash_state.callbacks[j] = crash_state.callbacks[j + 1];
      }
      crash_state.callback_count--;
      break;
    }
  }
  pthread_mutex_unlock(&crash_state.lock);
}

CrashHandlerConfig crash_handler_get_default_config(void) {
  CrashHandlerConfig cfg = {.log_backtrace = true,
                            .handle_segv = true,
                            .handle_abrt = true,
                            .handle_fpe = false,
                            .handle_bus = true,
                            .handle_ill = true,
                            .attempt_recovery = false,
                            .fail_safety = true,
                            .crash_log_path = NULL};
  return cfg;
}
