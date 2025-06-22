// Test harness for memory debugging, crash handler, and resource manager utilities
// This file demonstrates usage and intentionally triggers errors for diagnostics.

#include "scopemux/memory_debug.h"
#include "scopemux/crash_handler.h"
#include "scopemux/ts_resource_manager.h"
#include <stdio.h>

void crash_callback(void *userdata) {
    fprintf(stderr, "[CALLBACK] Crash occurred! Userdata: %s\n", (const char*)userdata);
}

int main() {
    // Enable memory debugging and crash handler
    memory_debug_init();
    CrashHandlerConfig cfg = crash_handler_get_default_config();
    crash_handler_init(&cfg);
    crash_handler_register_callback(crash_callback, "test");

    // Allocate and free memory
    void *a = SMX_MALLOC(64, "test_a");
    void *b = SMX_MALLOC(128, "test_b");
    SMX_FREE(a);
    // Intentionally leak 'b'

    // Tree-sitter resource manager usage
    TSResourceManager *mgr = ts_resource_manager_create();
    TSParser *parser = ts_resource_manager_create_parser(mgr);
    ts_resource_manager_register_parser(mgr, parser);
    ts_resource_manager_print_stats(mgr);
    ts_resource_manager_destroy(mgr);

    // Print memory stats
    memory_debug_print_stats();
    memory_debug_dump_allocations();

    // Uncomment to intentionally trigger a segfault (for crash diagnostics):
    // int *p = NULL; *p = 42;

    crash_handler_cleanup();
    memory_debug_cleanup();
    return 0;
}
