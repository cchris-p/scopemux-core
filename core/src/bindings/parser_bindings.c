/**
 * @file parser_bindings.c
 * @brief Python bindings for the ScopeMux parser
 *
 * This file contains the implementation of the Python bindings for the
 * ScopeMux parser. It exposes the parser API to Python using pybind11.
 */

/* Python header includes must come first */
#define PY_SSIZE_T_CLEAN /* Make sure PY_SSIZE_T_CLEAN is defined before including Python.h */
#include <Python.h>
#include <structmember.h>

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ScopeMux header includes */
#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/python_bindings.h"
#include "../../include/scopemux/python_utils.h"

// These type definitions are now in python_utils.h
// ParserContextObject - Python wrapper for ParserContext
// IRNodeObject - Python wrapper for IRNode

/**
 * @brief Deallocation function for ParserContextObject
 */
static void ParserContext_dealloc(ParserContextObject *self) {
  if (self->context) {
    parser_free(self->context);
    self->context = NULL;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Deallocation function for IRNodeObject
 */
static void IRNode_dealloc(IRNodeObject *self) {
  if (self->node && self->owned) {
    ir_node_free(self->node);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Create a new ParserContextObject
 */
static PyObject *ParserContext_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  ParserContextObject *self;
  self = (ParserContextObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->context = NULL;
  }
  return (PyObject *)self;
}

/**
 * @brief Initialize a ParserContextObject
 */
static int ParserContext_init(ParserContextObject *self, PyObject *args, PyObject *kwds) {
  self->context = parser_init();
  if (!self->context) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to initialize parser context");
    return -1;
  }
  return 0;
}

/**
 * @brief Parse a file
 */
static PyObject *ParserContext_parse_file(ParserContextObject *self, PyObject *args,
                                          PyObject *kwds) {
  const char *filename;
  int language = LANG_UNKNOWN;
  static char *kwlist[] = {"filename", "language", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwlist, &filename, &language)) {
    return NULL;
  }

  bool success = parser_parse_file(self->context, filename, (LanguageType)language);
  if (!success) {
    PyErr_SetString(PyExc_RuntimeError, parser_get_last_error(self->context));
    return NULL;
  }

  Py_RETURN_NONE;
}

/**
 * @brief Parse a string
 */
static PyObject *ParserContext_parse_string(ParserContextObject *self, PyObject *args,
                                            PyObject *kwds) {
  const char *content;
  Py_ssize_t content_length;
  const char *filename = NULL;
  int language = LANG_UNKNOWN;
  static char *kwlist[] = {"content", "filename", "language", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s#|si", kwlist, &content, &content_length,
                                   &filename, &language)) {
    return NULL;
  }

  bool success =
      parser_parse_string(self->context, content, content_length, filename, (LanguageType)language);
  if (!success) {
    PyErr_SetString(PyExc_RuntimeError, parser_get_last_error(self->context));
    return NULL;
  }

  Py_RETURN_NONE;
}

/**
 * @brief Get a node by qualified name
 */
static PyObject *ParserContext_get_node(ParserContextObject *self, PyObject *args, PyObject *kwds) {
  const char *qualified_name;
  static char *kwlist[] = {"qualified_name", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &qualified_name)) {
    return NULL;
  }

  const IRNode *node = parser_get_node(self->context, qualified_name);
  if (!node) {
    Py_RETURN_NONE;
  }

  // Create a PyObject for the node
  PyTypeObject *ir_node_type =
      (PyTypeObject *)PyObject_GetAttrString(PyImport_ImportModule("scopemux_core"), "IRNode");
  if (!ir_node_type) {
    return NULL;
  }

  IRNodeObject *py_node = (IRNodeObject *)ir_node_type->tp_alloc(ir_node_type, 0);
  if (!py_node) {
    Py_DECREF(ir_node_type);
    return NULL;
  }

  py_node->node = (IRNode *)node; // Cast away const, but mark as non-owned
  py_node->owned = 0;             // This is a reference, not owned

  Py_DECREF(ir_node_type);
  return (PyObject *)py_node;
}

/**
 * @brief Get all nodes of a specific type
 */
static PyObject *ParserContext_get_nodes_by_type(ParserContextObject *self, PyObject *args,
                                                 PyObject *kwds) {
  int node_type;
  static char *kwlist[] = {"node_type", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &node_type)) {
    return NULL;
  }

  // First, get the count
  size_t count = parser_get_nodes_by_type(self->context, (NodeType)node_type, NULL, 0);

  // Then, allocate an array and get the nodes
  const IRNode **nodes = (const IRNode **)malloc(count * sizeof(IRNode *));
  if (!nodes) {
    PyErr_NoMemory();
    return NULL;
  }

  parser_get_nodes_by_type(self->context, (NodeType)node_type, nodes, count);

  // Create a Python list to hold the nodes
  PyObject *py_list = PyList_New(count);
  if (!py_list) {
    free(nodes);
    return NULL;
  }

  // Get the IRNode type
  PyTypeObject *ir_node_type =
      (PyTypeObject *)PyObject_GetAttrString(PyImport_ImportModule("scopemux_core"), "IRNode");
  if (!ir_node_type) {
    free(nodes);
    Py_DECREF(py_list);
    return NULL;
  }

  // Fill the list with IRNodeObjects
  for (size_t i = 0; i < count; i++) {
    IRNodeObject *py_node = (IRNodeObject *)ir_node_type->tp_alloc(ir_node_type, 0);
    if (!py_node) {
      Py_DECREF(ir_node_type);
      Py_DECREF(py_list);
      free(nodes);
      return NULL;
    }

    py_node->node = (IRNode *)nodes[i]; // Cast away const, but mark as non-owned
    py_node->owned = 0;                 // This is a reference, not owned

    PyList_SET_ITEM(py_list, i, (PyObject *)py_node); // Steals reference to py_node
  }

  Py_DECREF(ir_node_type);
  free(nodes);
  return py_list;
}

/**
 * @brief Method definitions for ParserContext
 */
static PyMethodDef ParserContext_methods[] = {
    {"parse_file", (PyCFunction)ParserContext_parse_file, METH_VARARGS | METH_KEYWORDS,
     "Parse a file and generate IR"},
    {"parse_string", (PyCFunction)ParserContext_parse_string, METH_VARARGS | METH_KEYWORDS,
     "Parse a string and generate IR"},
    {"get_node", (PyCFunction)ParserContext_get_node, METH_VARARGS | METH_KEYWORDS,
     "Get a node by qualified name"},
    {"get_nodes_by_type", (PyCFunction)ParserContext_get_nodes_by_type,
     METH_VARARGS | METH_KEYWORDS, "Get all nodes of a specific type"},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for ParserContext
 */
static PyTypeObject ParserContextType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.ParserContext",
    .tp_doc = "Parser context for ScopeMux",
    .tp_basicsize = sizeof(ParserContextObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = ParserContext_new,
    .tp_init = (initproc)ParserContext_init,
    .tp_dealloc = (destructor)ParserContext_dealloc,
    .tp_methods = ParserContext_methods,
};

/**
 * @brief Property getters for IRNode
 */

static PyObject *IRNode_get_name(IRNodeObject *self, void *closure) {
  if (!self->node || !self->node->name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->name);
}

static PyObject *IRNode_get_qualified_name(IRNodeObject *self, void *closure) {
  if (!self->node || !self->node->qualified_name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->qualified_name);
}

static PyObject *IRNode_get_signature(IRNodeObject *self, void *closure) {
  if (!self->node || !self->node->signature) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->signature);
}

static PyObject *IRNode_get_docstring(IRNodeObject *self, void *closure) {
  if (!self->node || !self->node->docstring) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->docstring);
}

static PyObject *IRNode_get_content(IRNodeObject *self, void *closure) {
  if (!self->node || !self->node->raw_content) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->raw_content);
}

static PyObject *IRNode_get_type(IRNodeObject *self, void *closure) {
  if (!self->node) {
    Py_RETURN_NONE;
  }
  return PyLong_FromLong(self->node->type);
}

static PyObject *IRNode_get_range(IRNodeObject *self, void *closure) {
  if (!self->node) {
    Py_RETURN_NONE;
  }

  PyObject *range_dict = PyDict_New();
  if (!range_dict) {
    return NULL;
  }

  // Start location
  PyObject *start_dict = PyDict_New();
  if (!start_dict) {
    Py_DECREF(range_dict);
    return NULL;
  }

  PyDict_SetItemString(start_dict, "line", PyLong_FromUnsignedLong(self->node->range.start.line));
  PyDict_SetItemString(start_dict, "column",
                       PyLong_FromUnsignedLong(self->node->range.start.column));
  PyDict_SetItemString(start_dict, "offset",
                       PyLong_FromUnsignedLong(self->node->range.start.offset));

  // End location
  PyObject *end_dict = PyDict_New();
  if (!end_dict) {
    Py_DECREF(range_dict);
    Py_DECREF(start_dict);
    return NULL;
  }

  PyDict_SetItemString(end_dict, "line", PyLong_FromUnsignedLong(self->node->range.end.line));
  PyDict_SetItemString(end_dict, "column", PyLong_FromUnsignedLong(self->node->range.end.column));
  PyDict_SetItemString(end_dict, "offset", PyLong_FromUnsignedLong(self->node->range.end.offset));

  // Add start and end to range
  PyDict_SetItemString(range_dict, "start", start_dict);
  PyDict_SetItemString(range_dict, "end", end_dict);

  Py_DECREF(start_dict);
  Py_DECREF(end_dict);

  return range_dict;
}

/**
 * @brief Property definition for IRNode
 */
static PyGetSetDef IRNode_getsetters[] = {
    {"name", (getter)IRNode_get_name, NULL, "Node name", NULL},
    {"qualified_name", (getter)IRNode_get_qualified_name, NULL, "Fully qualified name", NULL},
    {"signature", (getter)IRNode_get_signature, NULL, "Function/method signature", NULL},
    {"docstring", (getter)IRNode_get_docstring, NULL, "Associated documentation", NULL},
    {"content", (getter)IRNode_get_content, NULL, "Raw source code content", NULL},
    {"type", (getter)IRNode_get_type, NULL, "Node type", NULL},
    {"range", (getter)IRNode_get_range, NULL, "Source range", NULL},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for IRNode
 */
static PyTypeObject IRNodeType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.IRNode",
    .tp_doc = "IR node representing a parsed entity",
    .tp_basicsize = sizeof(IRNodeObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)IRNode_dealloc,
    .tp_getset = IRNode_getsetters,
};

/**
 * @brief Detect language from filename
 */
static PyObject *detect_language(PyObject *self, PyObject *args, PyObject *kwds) {
  const char *filename;
  const char *content = NULL;
  Py_ssize_t content_length = 0;
  static char *kwlist[] = {"filename", "content", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s#", kwlist, &filename, &content,
                                   &content_length)) {
    return NULL;
  }

  LanguageType language = parser_detect_language(filename, content, content_length);
  return PyLong_FromLong((long)language);
}

/**
 * @brief Module methods
 */
static PyMethodDef module_methods[] = {
    {"detect_language", (PyCFunction)detect_language, METH_VARARGS | METH_KEYWORDS,
     "Detect language from filename and optional content"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

/**
 * @brief Initialize the parser bindings
 *
 * @param m Python module
 */
void init_parser_bindings(void *m) {
  PyObject *module = (PyObject *)m;

  // Finalize the types
  if (PyType_Ready(&ParserContextType) < 0) {
    return;
  }

  if (PyType_Ready(&IRNodeType) < 0) {
    return;
  }

  // Add the types to the module
  Py_INCREF(&ParserContextType);
  PyModule_AddObject(module, "ParserContext", (PyObject *)&ParserContextType);

  Py_INCREF(&IRNodeType);
  PyModule_AddObject(module, "IRNode", (PyObject *)&IRNodeType);

  // Add module methods
  PyModule_AddFunctions(module, module_methods);

  // Add language type constants
  PyModule_AddIntConstant(module, "LANG_UNKNOWN", LANG_UNKNOWN);
  PyModule_AddIntConstant(module, "LANG_C", LANG_C);
  PyModule_AddIntConstant(module, "LANG_CPP", LANG_CPP);
  PyModule_AddIntConstant(module, "LANG_PYTHON", LANG_PYTHON);
  PyModule_AddIntConstant(module, "LANG_JAVASCRIPT", LANG_JAVASCRIPT);
  PyModule_AddIntConstant(module, "LANG_TYPESCRIPT", LANG_TYPESCRIPT);
  PyModule_AddIntConstant(module, "LANG_RUST", LANG_RUST);

  // Add node type constants
  PyModule_AddIntConstant(module, "NODE_UNKNOWN", NODE_UNKNOWN);
  PyModule_AddIntConstant(module, "NODE_FUNCTION", NODE_FUNCTION);
  PyModule_AddIntConstant(module, "NODE_METHOD", NODE_METHOD);
  PyModule_AddIntConstant(module, "NODE_CLASS", NODE_CLASS);
  PyModule_AddIntConstant(module, "NODE_STRUCT", NODE_STRUCT);
  PyModule_AddIntConstant(module, "NODE_ENUM", NODE_ENUM);
  PyModule_AddIntConstant(module, "NODE_INTERFACE", NODE_INTERFACE);
  PyModule_AddIntConstant(module, "NODE_NAMESPACE", NODE_NAMESPACE);
  PyModule_AddIntConstant(module, "NODE_MODULE", NODE_MODULE);
  PyModule_AddIntConstant(module, "NODE_COMMENT", NODE_COMMENT);
  PyModule_AddIntConstant(module, "NODE_DOCSTRING", NODE_DOCSTRING);
}
