# ScopeMux Memory Debugging, Crash Handler, and Resource Manager Usage Guide

This guide covers how to enable, configure, and use the memory debugging, crash handler, and Tree-sitter resource manager utilities in ScopeMux.

---

## 1. Enabling Memory Debugging and Crash Protection (CMake)

Add these options to your CMake command:

```sh
cmake -DSCOPEMUX_DEBUG_MEMORY=ON -DSCOPEMUX_VALGRIND_COMPATIBLE=ON ...
```

- `SCOPEMUX_DEBUG_MEMORY=ON`: Enables memory tracking, leak detection, and pointer validation.
- `SCOPEMUX_VALGRIND_COMPATIBLE=ON`: Adds Valgrind-friendly build flags.

---

## 2. Using the Memory Debugger

### Initialization
Call `memory_debug_init()` early in your program:

```c
#include "scopemux/memory_debug.h"

int main() {
    memory_debug_init();
    // ... your code ...
    memory_debug_cleanup();
    return 0;
}
```

### Tracking Allocations
All allocations through `memory_debug_malloc`, `memory_debug_calloc`, `memory_debug_realloc`, and `memory_debug_free` are tracked. You can use macros for easier usage:

```c
void *ptr = SMX_MALLOC(128, "example");
SMX_FREE(ptr);
```

### Leak Detection
Leaks are reported on `memory_debug_cleanup()` if enabled.

---

## 3. Using the Crash Handler

### Initialization
Install the crash handler at startup:

```c
#include "scopemux/crash_handler.h"

int main() {
    CrashHandlerConfig cfg = crash_handler_get_default_config();
    crash_handler_init(&cfg);
    // ... your code ...
    crash_handler_cleanup();
    return 0;
}
```

### Features
- Catches SIGSEGV, SIGABRT, and other fatal signals.
- Logs backtrace and context stack.
- Allows registration of custom crash callbacks.

---

## 4. Using the Tree-sitter Resource Manager

### Initialization
Create and destroy a resource manager:

```c
#include "scopemux/ts_resource_manager.h"

TSResourceManager *mgr = ts_resource_manager_create();
// Use ts_resource_manager_* functions to manage parsers, trees, queries, cursors
// ...
ts_resource_manager_destroy(mgr);
```

### Example
```c
TSParser *parser = ts_resource_manager_create_parser(mgr);
ts_resource_manager_register_parser(mgr, parser);
// ...
ts_resource_manager_unregister_parser(mgr, parser);
```

---

## 5. Example: Enabling All Protections

```c
#include "scopemux/memory_debug.h"
#include "scopemux/crash_handler.h"
#include "scopemux/ts_resource_manager.h"

int main() {
    memory_debug_init();
    CrashHandlerConfig cfg = crash_handler_get_default_config();
    crash_handler_init(&cfg);
    TSResourceManager *mgr = ts_resource_manager_create();
    // ...
    ts_resource_manager_destroy(mgr);
    crash_handler_cleanup();
    memory_debug_cleanup();
    return 0;
}
```
