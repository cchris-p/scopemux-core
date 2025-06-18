/**
 * @file parser_bindings.c
 * @brief Python bindings for the ScopeMux parser
 *
 * This file contains the implementation of the Python bindings for the
 * ScopeMux parser. It exposes the parser API to Python using pybind11.
 */

/* Python header includes must come first */
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
 * @brief Create a new ParserContextObject
 */
static PyObject *ParserContext_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  (void)args; // Silence unused parameter warning
  (void)kwds; // Silence unused parameter warning
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
  (void)args; // Silence the unused parameter warning
  (void)kwds; // Silence the unused parameter warning
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
static PyObject *ParserContext_parse_file(PyObject *self_obj, PyObject *args, PyObject *kwds) {
  ParserContextObject *self = (ParserContextObject *)self_obj;
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
static PyObject *ParserContext_parse_string(PyObject *self_obj, PyObject *args, PyObject *kwds) {
  ParserContextObject *self = (ParserContextObject *)self_obj;
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
 * @brief Method definitions for ParserContext
 */
static PyMethodDef ParserContext_methods[] = {
    {"parse_file", (PyCFunction)ParserContext_parse_file, METH_VARARGS | METH_KEYWORDS,
     "Parse a file"},
    {"parse_string", (PyCFunction)ParserContext_parse_string,
     METH_VARARGS | METH_KEYWORDS, "Parse a string of code"},
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
 * @brief Deallocation function for ASTNodeObject
 */
static void ASTNode_dealloc(ASTNodeObject *self) {
  if (self->node && self->owned) {
    ast_node_free(self->node);
    self->node = NULL;
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Create a new ASTNodeObject
 */
static PyObject *ASTNode_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  (void)args; // Silence unused parameter warning
  (void)kwds; // Silence unused parameter warning
  ASTNodeObject *self;
  self = (ASTNodeObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->node = NULL;
    self->owned = 0;
  }
  return (PyObject *)self;
}

/**
 * @brief Initialize an ASTNodeObject
 */
static int ASTNode_init(ASTNodeObject *self, PyObject *args, PyObject *kwds) {
  (void)self; // Silence unused parameter warning
  (void)args; // Silence unused parameter warning
  (void)kwds; // Silence unused parameter warning
  // This is typically initialized from C code, not from Python
  // For now, this is a placeholder that just succeeds
  return 0;
}

// Node property getter functions
static PyObject *ASTNode_get_name(ASTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning
  if (!self->node || !self->node->name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->name);
}

static PyObject *ASTNode_get_qualified_name(ASTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning
  if (!self->node || !self->node->qualified_name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->qualified_name);
}

static PyObject *ASTNode_get_signature(ASTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning
  if (!self->node || !self->node->signature) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->signature);
}

static PyObject *ASTNode_get_docstring(ASTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning
  if (!self->node || !self->node->docstring) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->docstring);
}

static PyObject *ASTNode_get_raw_content(ASTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning
  if (!self->node || !self->node->raw_content) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->raw_content);
}

static PyObject *ASTNode_get_type(ASTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning
  if (!self->node) {
    Py_RETURN_NONE;
  }
  return PyLong_FromLong(self->node->type);
}

/**
 * @brief Property getters for ASTNode
 */
static PyGetSetDef ASTNode_getsetters[] = {
    {"name", (getter)ASTNode_get_name, NULL, "Name of the node", NULL},
    {"qualified_name", (getter)ASTNode_get_qualified_name, NULL, "Qualified name of the node",
     NULL},
    {"signature", (getter)ASTNode_get_signature, NULL, "Function/method signature", NULL},
    {"docstring", (getter)ASTNode_get_docstring, NULL, "Associated documentation", NULL},
    {"raw_content", (getter)ASTNode_get_raw_content, NULL, "Raw source code content", NULL},
    {"type", (getter)ASTNode_get_type, NULL, "Type of the node", NULL},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for ASTNode
 */
static PyTypeObject ASTNodePyType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.ASTNode",
    .tp_doc = "AST node representing a parsed semantic entity",
    .tp_basicsize = sizeof(ASTNodeObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = ASTNode_new,
    .tp_init = (initproc)ASTNode_init,
    .tp_dealloc = (destructor)ASTNode_dealloc,
    .tp_getset = ASTNode_getsetters,
};

/**
 * @brief Detect language from filename
 */
static PyObject *detect_language(PyObject *self, PyObject *args, PyObject *kwds) {
  (void)self; // Silence the unused parameter warning
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
     "Detect language from filename and optionally content"},
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

  if (PyType_Ready(&ASTNodePyType) < 0) {
    return;
  }

  // Add the types to the module
  Py_INCREF(&ParserContextType);
  PyModule_AddObject(module, "ParserContext", (PyObject *)&ParserContextType);

  Py_INCREF(&ASTNodePyType);
  PyModule_AddObject(module, "ASTNode", (PyObject *)&ASTNodePyType);

  // Add module methods
  PyModule_AddFunctions(module, module_methods);

  // Add language type constants
  PyModule_AddIntConstant(module, "LANG_UNKNOWN", LANG_UNKNOWN);
  PyModule_AddIntConstant(module, "LANG_C", LANG_C);
  PyModule_AddIntConstant(module, "LANG_CPP", LANG_CPP);
  PyModule_AddIntConstant(module, "LANG_PYTHON", LANG_PYTHON);
  PyModule_AddIntConstant(module, "LANG_JAVASCRIPT", LANG_JAVASCRIPT);
  PyModule_AddIntConstant(module, "LANG_TYPESCRIPT", LANG_TYPESCRIPT);

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
