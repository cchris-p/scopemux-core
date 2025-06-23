/**
 * @file context_engine_bindings.c
 * @brief Python bindings for the ScopeMux context engine
 *
 * This file contains the implementation of the Python bindings for the
 * ScopeMux context engine. It exposes the context engine API to Python using pybind11.
 */

/* Python header includes must come first */
#include <Python.h>
#include <structmember.h>

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ScopeMux header includes */
#include "../../core/include/scopemux/context_engine.h"
#include "../../core/include/scopemux/python_bindings.h"
#include "../../core/include/scopemux/python_utils.h"

// These type definitions are now in python_utils.h
// ParserContextObject - Python wrapper for ParserContext

// ContextEngineObject - Python wrapper for ContextEngine
// InfoBlockObject - Python wrapper for InfoBlock

/**
 * @brief Deallocation function for ContextEngineObject
 */
static void ContextEngine_dealloc(ContextEngineObject *self) {
  if (self->engine) {
    context_engine_free(self->engine);
    self->engine = NULL;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Deallocation function for InfoBlockObject
 */
static void InfoBlock_dealloc(InfoBlockObject *self) {
  if (self->block && self->owned) {
    // Note: In a real implementation, we'd need a function to free individual InfoBlocks
    // Right now, the context engine frees all blocks together
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Create a new ContextEngineObject
 */
static PyObject *ContextEngine_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  // Mark unused parameters to avoid compiler warnings
  (void)args;
  (void)kwds;

  ContextEngineObject *self;
  self = (ContextEngineObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->engine = NULL;
  }
  return (PyObject *)self;
}

/**
 * @brief Initialize a ContextEngineObject
 */
static int ContextEngine_init(ContextEngineObject *self, PyObject *args, PyObject *kwds) {
  PyObject *options_dict = NULL;
  static char *kwlist[] = {"options", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &options_dict)) {
    return -1;
  }

  // If options_dict is provided, parse it to create ContextOptions
  ContextOptions options = {
      .max_tokens = 2048,          // Default max tokens
      .recency_weight = 1.0f,      // Default recency weight
      .proximity_weight = 1.0f,    // Default proximity weight
      .similarity_weight = 1.0f,   // Default similarity weight
      .reference_weight = 1.0f,    // Default reference weight
      .user_focus_weight = 2.0f,   // Default user focus weight
      .preserve_structure = true,  // Default preserve structure
      .prioritize_functions = true // Default prioritize functions
  };

  if (options_dict && PyDict_Check(options_dict)) {
    PyObject *value;

    // max_tokens
    value = PyDict_GetItemString(options_dict, "max_tokens");
    if (value && PyLong_Check(value)) {
      options.max_tokens = (size_t)PyLong_AsUnsignedLong(value);
    }

    // recency_weight
    value = PyDict_GetItemString(options_dict, "recency_weight");
    if (value && PyFloat_Check(value)) {
      options.recency_weight = (float)PyFloat_AsDouble(value);
    }

    // proximity_weight
    value = PyDict_GetItemString(options_dict, "proximity_weight");
    if (value && PyFloat_Check(value)) {
      options.proximity_weight = (float)PyFloat_AsDouble(value);
    }

    // similarity_weight
    value = PyDict_GetItemString(options_dict, "similarity_weight");
    if (value && PyFloat_Check(value)) {
      options.similarity_weight = (float)PyFloat_AsDouble(value);
    }

    // reference_weight
    value = PyDict_GetItemString(options_dict, "reference_weight");
    if (value && PyFloat_Check(value)) {
      options.reference_weight = (float)PyFloat_AsDouble(value);
    }

    // user_focus_weight
    value = PyDict_GetItemString(options_dict, "user_focus_weight");
    if (value && PyFloat_Check(value)) {
      options.user_focus_weight = (float)PyFloat_AsDouble(value);
    }

    // preserve_structure
    value = PyDict_GetItemString(options_dict, "preserve_structure");
    if (value) {
      options.preserve_structure = PyObject_IsTrue(value);
    }

    // prioritize_functions
    value = PyDict_GetItemString(options_dict, "prioritize_functions");
    if (value) {
      options.prioritize_functions = PyObject_IsTrue(value);
    }
  }

  self->engine = context_engine_init(&options);
  if (!self->engine) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to initialize context engine");
    return -1;
  }

  return 0;
}

/**
 * @brief Add a parser context to the context engine
 */
static PyObject *ContextEngine_add_parser_context(PyObject *self_obj, PyObject *args,
                                                  PyObject *kwds) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  PyObject *py_parser_ctx;
  static char *kwlist[] = {"parser_ctx", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &py_parser_ctx)) {
    return NULL;
  }

  // Check if py_parser_ctx is a ParserContextObject
  if (!PyObject_TypeCheck(py_parser_ctx,
                          (PyTypeObject *)PyObject_GetAttrString(
                              PyImport_ImportModule("scopemux_core"), "ParserContext"))) {
    PyErr_SetString(PyExc_TypeError, "Expected a ParserContext object");
    return NULL;
  }

  ParserContextObject *parser_ctx_obj = (ParserContextObject *)py_parser_ctx;
  size_t num_added = context_engine_add_parser_context(self->engine, parser_ctx_obj->context);

  return PyLong_FromSize_t(num_added);
}

/**
 * @brief Rank blocks by relevance
 */
static PyObject *ContextEngine_rank_blocks(PyObject *self_obj, PyObject *args, PyObject *kwds) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  const char *cursor_file;
  unsigned int cursor_line;
  unsigned int cursor_column;
  const char *query = NULL;
  static char *kwlist[] = {"cursor_file", "cursor_line", "cursor_column", "query", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "sII|s", kwlist, &cursor_file, &cursor_line,
                                   &cursor_column, &query)) {
    return NULL;
  }

  bool success =
      context_engine_rank_blocks(self->engine, cursor_file, cursor_line, cursor_column, query);
  if (!success) {
    PyErr_SetString(PyExc_RuntimeError, context_engine_get_last_error(self->engine));
    return NULL;
  }

  Py_RETURN_NONE;
}

/**
 * @brief Apply compression to fit within token budget
 */
static PyObject *ContextEngine_compress(PyObject *self_obj, PyObject *ignored) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  (void)ignored; // Unused parameter
  bool success = context_engine_compress(self->engine);
  if (!success) {
    PyErr_SetString(PyExc_RuntimeError, context_engine_get_last_error(self->engine));
    return NULL;
  }

  Py_RETURN_NONE;
}

/**
 * @brief Get compressed context as a string
 */
static PyObject *ContextEngine_get_context(PyObject *self_obj, PyObject *ignored) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  (void)ignored; // Unused parameter
  // First, get the required size
  size_t size = context_engine_get_context(self->engine, NULL, 0);

  // Allocate a buffer
  char *buffer = (char *)malloc(size + 1); // +1 for null terminator
  if (!buffer) {
    PyErr_NoMemory();
    return NULL;
  }

  // Get the context
  context_engine_get_context(self->engine, buffer, size + 1);

  // Create a Python string
  PyObject *py_str = PyUnicode_FromString(buffer);
  free(buffer);

  return py_str;
}

/**
 * @brief Estimate the number of tokens in a string
 */
static PyObject *ContextEngine_estimate_tokens(PyObject *self_obj, PyObject *args, PyObject *kwds) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  const char *text;
  Py_ssize_t text_length;
  static char *kwlist[] = {"text", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s#", kwlist, &text, &text_length)) {
    return NULL;
  }

  size_t tokens = context_engine_estimate_tokens(self->engine, text, text_length);

  return PyLong_FromSize_t(tokens);
}

/**
 * @brief Update user focus for specific blocks
 */
static PyObject *ContextEngine_update_focus(PyObject *self_obj, PyObject *args, PyObject *kwds) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  PyObject *py_node_names;
  float focus_value;
  static char *kwlist[] = {"node_qualified_names", "focus_value", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "Of", kwlist, &py_node_names, &focus_value)) {
    return NULL;
  }

  // Check if py_node_names is a list
  if (!PyList_Check(py_node_names)) {
    PyErr_SetString(PyExc_TypeError, "Expected a list of qualified names");
    return NULL;
  }

  // Convert Python list to C array
  Py_ssize_t num_nodes = PyList_Size(py_node_names);
  const char **node_names = (const char **)malloc(num_nodes * sizeof(char *));
  if (!node_names) {
    PyErr_NoMemory();
    return NULL;
  }

  for (Py_ssize_t i = 0; i < num_nodes; i++) {
    PyObject *py_name = PyList_GetItem(py_node_names, i);
    if (!PyUnicode_Check(py_name)) {
      free(node_names);
      PyErr_SetString(PyExc_TypeError, "Expected a list of strings");
      return NULL;
    }
    node_names[i] = PyUnicode_AsUTF8(py_name);
  }

  // Update focus
  size_t num_updated =
      context_engine_update_focus(self->engine, node_names, num_nodes, focus_value);
  free(node_names);

  return PyLong_FromSize_t(num_updated);
}

/**
 * @brief Reset all compression
 */
static PyObject *ContextEngine_reset_compression(PyObject *self_obj, PyObject *Py_UNUSED(args)) {
  ContextEngineObject *self = (ContextEngineObject *)self_obj;
  context_engine_reset_compression(self->engine);
  Py_RETURN_NONE;
}

/**
 * @brief Method definitions for ContextEngine
 */
static PyMethodDef ContextEngine_methods[] = {
    {"add_parser_context", (PyCFunction)ContextEngine_add_parser_context,
     METH_VARARGS | METH_KEYWORDS, "Add a parser context"},
    {"rank_blocks", (PyCFunction)ContextEngine_rank_blocks, METH_VARARGS | METH_KEYWORDS,
     "Rank blocks by relevance"},
    {"compress", (PyCFunction)ContextEngine_compress, METH_NOARGS,
     "Apply compression to fit within token budget"},
    {"get_context", (PyCFunction)ContextEngine_get_context, METH_NOARGS,
     "Get compressed context as a single string"},
    {"estimate_tokens", (PyCFunction)ContextEngine_estimate_tokens, METH_VARARGS | METH_KEYWORDS,
     "Estimate the number of tokens in a text string"},
    {"update_focus", (PyCFunction)ContextEngine_update_focus, METH_VARARGS | METH_KEYWORDS,
     "Update the user focus for specific blocks"},
    {"reset_compression", (PyCFunction)ContextEngine_reset_compression, METH_NOARGS,
     "Reset all compression to COMPRESSION_NONE"},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for ContextEngine
 */
static PyTypeObject ContextEngineType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.ContextEngine",
    .tp_doc = "Context engine for ScopeMux",
    .tp_basicsize = sizeof(ContextEngineObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = ContextEngine_new,
    .tp_init = (initproc)ContextEngine_init,
    .tp_dealloc = (destructor)ContextEngine_dealloc,
    .tp_methods = ContextEngine_methods,
};

/**
 * @brief Property getters for InfoBlock
 */
static PyObject *InfoBlock_get_original_tokens(InfoBlockObject *self, void *closure) {
  (void)closure; // Silence unused parameter warning
  if (!self->block) {
    Py_RETURN_NONE;
  }
  return PyLong_FromSize_t(self->block->original_tokens);
}

static PyObject *InfoBlock_get_compressed_tokens(InfoBlockObject *self, void *closure) {
  (void)closure; // Silence unused parameter warning
  if (!self->block) {
    Py_RETURN_NONE;
  }
  return PyLong_FromSize_t(self->block->compressed_tokens);
}

static PyObject *InfoBlock_get_compression_level(InfoBlockObject *self, void *closure) {
  (void)closure; // Silence unused parameter warning
  if (!self->block) {
    Py_RETURN_NONE;
  }
  return PyLong_FromLong(self->block->level);
}

static PyObject *InfoBlock_get_relevance(InfoBlockObject *self, void *closure) {
  (void)closure; // Silence unused parameter warning
  if (!self->block) {
    Py_RETURN_NONE;
  }

  PyObject *relevance_dict = PyDict_New();
  if (!relevance_dict) {
    return NULL;
  }

  PyDict_SetItemString(relevance_dict, "recency",
                       PyFloat_FromDouble(self->block->relevance.recency));
  PyDict_SetItemString(relevance_dict, "cursor_proximity",
                       PyFloat_FromDouble(self->block->relevance.cursor_proximity));
  PyDict_SetItemString(relevance_dict, "semantic_similarity",
                       PyFloat_FromDouble(self->block->relevance.semantic_similarity));
  PyDict_SetItemString(relevance_dict, "reference_count",
                       PyFloat_FromDouble(self->block->relevance.reference_count));
  PyDict_SetItemString(relevance_dict, "user_focus",
                       PyFloat_FromDouble(self->block->relevance.user_focus));

  return relevance_dict;
}

static PyObject *InfoBlock_get_compressed_content(InfoBlockObject *self, void *closure) {
  (void)closure; // Silence unused parameter warning
  if (!self->block || !self->block->compressed_content) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->block->compressed_content);
}

// This function was removed as part of IRNode deprecation
// InfoBlock now uses ASTNode instead of IRNode

/**
 * @brief Property definition for InfoBlock
 */
static PyGetSetDef InfoBlock_getsetters[] = {
    {"original_tokens", (getter)InfoBlock_get_original_tokens, NULL, "Original token count", NULL},
    {"compressed_tokens", (getter)InfoBlock_get_compressed_tokens, NULL, "Compressed token count",
     NULL},
    {"compression_level", (getter)InfoBlock_get_compression_level, NULL,
     "Current compression level", NULL},
    {"relevance", (getter)InfoBlock_get_relevance, NULL, "Relevance metrics for ranking", NULL},
    {"compressed_content", (getter)InfoBlock_get_compressed_content, NULL, "Compressed content",
     NULL},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for InfoBlock
 */
static PyTypeObject InfoBlockType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.InfoBlock",
    .tp_doc = "Information Block representing a unit of code or documentation",
    .tp_basicsize = sizeof(InfoBlockObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)InfoBlock_dealloc,
    .tp_getset = InfoBlock_getsetters,
};

/**
 * @brief Initialize the context engine bindings
 *
 * @param m Python module
 */
void init_context_engine_bindings(void *m) {
  PyObject *module = (PyObject *)m;

  // Finalize the types
  if (PyType_Ready(&ContextEngineType) < 0) {
    return;
  }

  if (PyType_Ready(&InfoBlockType) < 0) {
    return;
  }

  // Add the types to the module
  Py_INCREF(&ContextEngineType);
  PyModule_AddObject(module, "ContextEngine", (PyObject *)&ContextEngineType);

  Py_INCREF(&InfoBlockType);
  PyModule_AddObject(module, "InfoBlock", (PyObject *)&InfoBlockType);

  // Add compression level constants
  PyModule_AddIntConstant(module, "COMPRESSION_NONE", COMPRESSION_NONE);
  PyModule_AddIntConstant(module, "COMPRESSION_LIGHT", COMPRESSION_LIGHT);
  PyModule_AddIntConstant(module, "COMPRESSION_MEDIUM", COMPRESSION_MEDIUM);
  PyModule_AddIntConstant(module, "COMPRESSION_HEAVY", COMPRESSION_HEAVY);
  PyModule_AddIntConstant(module, "COMPRESSION_SIGNATURE_ONLY", COMPRESSION_SIGNATURE_ONLY);
}