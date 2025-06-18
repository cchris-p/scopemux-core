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
 * @brief Method definitions for ParserContext
 */
static PyMethodDef ParserContext_methods[] = {
    {"parse_file", (PyCFunction)ParserContext_parse_file, METH_VARARGS | METH_KEYWORDS,
     "Parse a file and generate IR"},
    {"parse_string", (PyCFunction)ParserContext_parse_string, METH_VARARGS | METH_KEYWORDS,
     "Parse a string and generate IR"},
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

// Node property getter functions
static PyObject *ASTNode_get_name(ASTNodeObject *self, void *closure) {
  if (!self->node || !self->node->name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->name);
}

static PyObject *ASTNode_get_qualified_name(ASTNodeObject *self, void *closure) {
  if (!self->node || !self->node->qualified_name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->qualified_name);
}

static PyObject *ASTNode_get_signature(ASTNodeObject *self, void *closure) {
  if (!self->node || !self->node->signature) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->signature);
}

static PyObject *ASTNode_get_docstring(ASTNodeObject *self, void *closure) {
  if (!self->node || !self->node->docstring) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->docstring);
}

static PyObject *ASTNode_get_raw_content(ASTNodeObject *self, void *closure) {
  if (!self->node || !self->node->raw_content) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->raw_content);
}

static PyObject *ASTNode_get_type(ASTNodeObject *self, void *closure) {
  if (!self->node) {
    Py_RETURN_NONE;
  }
  return PyLong_FromLong(self->node->type);
}

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

  return;
}

// Add the types to the module
Py_INCREF(&ParserContextType);
PyModule_AddObject(module, "ParserContext", (PyObject *)&ParserContextType);

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
