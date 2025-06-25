/**
 * @file signal_handler.c
 * @brief Signal handling implementation for the Python bindings
 *
 * This file provides a wrapper for the segfault_handler function
 * to ensure proper linking in the Python module.
 */

#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "../../src/parser/memory_tracking.h"

/**
 * @brief Forward declaration for the segfault_handler from memory_tracking.c
 */
extern void segfault_handler(int sig);

/**
 * @brief Function to initialize signal handling in the Python module
 *
 * This function is called when the Python module is initialized.
 * It ensures that signal handling is properly set up.
 */
__attribute__((visibility("default")))
void py_init_signal_handling(void) {
    /* Nothing to do here - just forces linker to include this file */
    /* The real segfault_handler is defined in memory_tracking.c */
}
