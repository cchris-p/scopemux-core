/**
 * @file module.c
 * @brief Python bindings module implementation for ScopeMux
 * 
 * This file contains the implementation of the Python bindings module
 * for ScopeMux. It uses pybind11 to expose the C API to Python.
 */

#include <stdlib.h>
#include <stdio.h>

/* This include is REQUIRED - it provides Python.h with PyObject and Python API
 * Clangd incorrectly flags this as unused, but removing it causes compile errors
 * DO NOT REMOVE even if linter suggests it's unused
 */
#include "../../include/scopemux/python_utils.h"
#include "../../include/scopemux/python_bindings.h"

// Include forward declarations for binding initialization functions
extern void init_parser_bindings(void* m);
extern void init_context_engine_bindings(void* m);
extern void init_tree_sitter_bindings(void* m);

/**
 * @brief Module documentation string
 */
static const char module_docstring[] = 
    "ScopeMux C bindings\n"
    "\n"
    "This module provides high-performance C implementations of the ScopeMux\n"
    "core functionality, including parsing, IR generation, and context management.\n";

/**
 * @brief Initialize the Python module
 * 
 * This function is called by pybind11 when the module is imported.
 * It sets up the module and registers all the classes and functions.
 * 
 * @param m Pointer to the Python module
 */
void init_scopemux_module(void* m) {
    PyObject* module = (PyObject*)m;
    
    // Set module docstring
    PyModule_AddObject(module, "__doc__", PyUnicode_FromString(module_docstring));
    
    // Initialize the parser bindings
    init_parser_bindings(m);
    
    // Initialize the context engine bindings
    init_context_engine_bindings(m);
    
    // Initialize the tree-sitter bindings
    init_tree_sitter_bindings(m);
    
    // Add version information
    PyModule_AddStringConstant(module, "__version__", "0.1.0");
    
    // Add additional module-level constants or functions as needed
    PyModule_AddIntConstant(module, "DEFAULT_TOKEN_BUDGET", 2048);
}

/**
 * @brief Forward declaration of module methods
 */
extern PyMethodDef module_methods[];

/**
 * @brief Python module definition
 */
static PyModuleDef scopemux_module = {
    PyModuleDef_HEAD_INIT,
    "scopemux_core",   /* name of module */
    module_docstring,  /* module documentation */
    -1,                /* size of per-interpreter state of the module,
                          or -1 if the module keeps state in global variables. */
    module_methods,    /* module methods */
    NULL,              /* m_slots */
    NULL,              /* m_traverse */
    NULL,              /* m_clear */
    NULL,              /* m_free */
};

/**
 * @brief Python module initialization function
 * 
 * This function is called when the module is imported in Python.
 * 
 * @return PyObject* The module object
 */
PyMODINIT_FUNC PyInit_scopemux_core(void) {
    PyObject* m = PyModule_Create(&scopemux_module);
    if (m == NULL) {
        return NULL;
    }
    
    // Initialize the module
    init_scopemux_module(m);
    
    return m;
}
