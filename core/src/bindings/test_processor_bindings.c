#define _GNU_SOURCE
#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../core/include/scopemux/processors/test_processor.h"
#include "../../core/include/scopemux/parser.h"
#include "../../core/include/scopemux/adapters/language_adapter.h"
#include "../../core/include/scopemux/adapters/adapter_registry.h"

/**
 * Direct implementation of test processor functions needed by the Python module
 * to ensure they are linked properly.
 */

/**
 * We don't need to implement adapt_hello_world_test here anymore
 * since we're including the original implementation from src/processors/test_processor.c
 */

/**
 * Register test processor functions in the Python module
 * 
 * This just adds a simple Python function that wraps the C implementation.
 * No actual implementation is needed here since we're including the
 * original source files that contain the implementation.
 */
void register_test_processor(PyObject *module) {
    // We still need to register this function in the Python module
    // but we don't need to implement anything here since we're directly
    // including test_processor.c in the build
    
    // Add a placeholder for future test processor bindings
    PyModule_AddStringConstant(module, "TEST_PROCESSOR_VERSION", "1.0.0");
}
