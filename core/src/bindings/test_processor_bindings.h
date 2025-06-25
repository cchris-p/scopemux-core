/**
 * @file test_processor_bindings.h
 * @brief Header file for Python test processor bindings 
 */

#ifndef TEST_PROCESSOR_BINDINGS_H
#define TEST_PROCESSOR_BINDINGS_H

#include <Python.h>

/**
 * Register the test processor bindings for the Python module
 * 
 * @param m Pointer to the Python module
 */
void register_test_processor(void *m);

#endif /* TEST_PROCESSOR_BINDINGS_H */
