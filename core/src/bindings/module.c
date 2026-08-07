/**
 * @file module.c
 * @brief Python bindings module implementation for ScopeMux
 *
 * This file contains the implementation of the Python bindings module
 * for ScopeMux. It uses pybind11 to expose the C API to Python.
 */

#include "../../core/include/scopemux/crash_handler.h"
#include "../../core/include/scopemux/lang_compliance.h"
#include "../../core/include/scopemux/memory_debug.h"
#include <stdio.h>
#include <stdlib.h>

/* Include necessary headers for module initialization */
#include <Python.h>
#include <dlfcn.h>

/* Include logging header */
#include "../../core/include/scopemux/logging.h"

/* Include module bindings */
#include "context_engine_bindings.h"
#include "parser_bindings.h"
#include "test_processor_bindings.h"

/* Include signal handling and crash recovery */
#include "../parser/memory_tracking.h"

// Forward declaration of the signal handling initialization function
void py_init_signal_handling(void);

/* Define logging_enabled variable for module */
int logging_enabled = 0;

/* This include is REQUIRED - it provides Python.h with PyObject and Python API
 * Clangd incorrectly flags this as unused, but removing it causes compile errors
 * DO NOT REMOVE even if linter suggests it's unused
 */
#include "../../core/include/scopemux/python_bindings.h"
#include "../../core/include/scopemux/python_utils.h"

// Include forward declarations for binding initialization functions
extern void init_parser_bindings(void *m);
extern void init_context_engine_bindings(void *m);
extern void register_test_processor(void *m);

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
void init_scopemux_module(void *m) {
  // Memory debugging is already initialized in PyInit_scopemux_core
  log_debug("Setting up module components");
  PyObject *module = (PyObject *)m;

  // Register all language compliance adapters
  register_all_language_compliance();

  // Set module docstring
  PyModule_AddObject(module, "__doc__", PyUnicode_FromString(module_docstring));

  // Initialize the parser bindings
  init_parser_bindings(m);

  // Initialize the context engine bindings
  init_context_engine_bindings(m);

  // Initialize test processor bindings
  register_test_processor(m);

  // Initialize signal handling for the Python module
  py_init_signal_handling();

  // Add version information
  PyModule_AddStringConstant(m, "__version__", "0.1.0");

  // Add additional module-level constants or functions as needed
  PyModule_AddIntConstant(m, "DEFAULT_TOKEN_BUDGET", 2048);

  // Create a capsule object to expose segfault_handler for linking
  PyObject *capsule = PyCapsule_New((void *)segfault_handler, "segfault_handler", NULL);
  PyModule_AddObject(m, "_segfault_handler", capsule);
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
    "scopemux_core",  /* name of module */
    module_docstring, /* module documentation */
    -1,               /* size of per-interpreter state of the module,
                         or -1 if the module keeps state in global variables. */
    module_methods,   /* module methods */
    NULL,             /* m_slots */
    NULL,             /* m_traverse */
    NULL,             /* m_clear */
    NULL,             /* m_free */
};

/**
 * @brief Memory debugging cleanup function for atexit
 */
static void memory_debug_atexit_cleanup(void) {
  log_info("Performing memory debugging cleanup at exit");
  memory_debug_print_stats();
  memory_debug_dump_allocations();
  memory_debug_cleanup();
}

/**
 * @brief Module initialization function
 *
 * This is the entry point for Python when importing the module.
 * Also loads the tree-sitter shared libraries with RTLD_GLOBAL to make symbols available.
 */
PyMODINIT_FUNC PyInit_scopemux_core(void) {
  // Initialize logging first
  log_set_level(LOG_DEBUG);
  log_info("Initializing scopemux_core Python module");

  // Initialize memory debugging as early as possible
  log_info("Setting up memory debugging");
  memory_debug_configure(true, true, true);
  memory_debug_init();

  // Register cleanup function to run at interpreter exit
  atexit(memory_debug_atexit_cleanup);

  // Load tree-sitter libraries with RTLD_GLOBAL flag to make symbols available to dependent
  // libraries
  log_debug("Loading Tree-sitter shared libraries");
  dlopen("libtree-sitter.so", RTLD_NOW | RTLD_GLOBAL);
  dlopen("libtree-sitter-c.so", RTLD_NOW | RTLD_GLOBAL);
  dlopen("libtree-sitter-cpp.so", RTLD_NOW | RTLD_GLOBAL);
  dlopen("libtree-sitter-python.so", RTLD_NOW | RTLD_GLOBAL);
  dlopen("libtree-sitter-javascript.so", RTLD_NOW | RTLD_GLOBAL);
  dlopen("libtree-sitter-typescript.so", RTLD_NOW | RTLD_GLOBAL);

  // Create the module
  log_debug("Creating Python module");
  PyObject *m = PyModule_Create(&scopemux_module);
  if (m == NULL) {
    log_error("Failed to create Python module");
    return NULL;
  }

  // Initialize module components
  log_debug("Initializing module components");
  init_scopemux_module(m);

  log_info("scopemux_core module initialization complete");
  return m;
}
