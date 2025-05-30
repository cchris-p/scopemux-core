/**
 * @file python_bindings.h
 * @brief Python bindings interface for ScopeMux
 * 
 * This module provides the interface for Python bindings using pybind11.
 * It exposes the ScopeMux C API to Python, allowing Python code to use
 * the parser and context engine.
 */

#ifndef SCOPEMUX_PYTHON_BINDINGS_H
#define SCOPEMUX_PYTHON_BINDINGS_H



/**
 * @brief Initialize the Python module
 * 
 * This function is called by pybind11 when the module is imported.
 * It sets up the module and registers all the classes and functions.
 * 
 * @param m Pointer to the Python module
 */
void init_scopemux_module(void* m);

/**
 * @brief Initialize the parser bindings
 * 
 * @param m Pointer to the Python module
 */
void init_parser_bindings(void* m);

/**
 * @brief Initialize the context engine bindings
 * 
 * @param m Pointer to the Python module
 */
void init_context_engine_bindings(void* m);

/**
 * @brief Initialize the tree-sitter bindings
 * 
 * @param m Pointer to the Python module
 */
void init_tree_sitter_bindings(void* m);



#endif /* SCOPEMUX_PYTHON_BINDINGS_H */
