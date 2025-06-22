/**
 * @file context_engine_bindings.h
 * @brief Header file for Python context engine bindings 
 */

#ifndef CONTEXT_ENGINE_BINDINGS_H
#define CONTEXT_ENGINE_BINDINGS_H

#include <Python.h>

/**
 * Initialize the context engine bindings for the Python module
 * 
 * @param m Pointer to the Python module
 */
void init_context_engine_bindings(void *m);

#endif /* CONTEXT_ENGINE_BINDINGS_H */
