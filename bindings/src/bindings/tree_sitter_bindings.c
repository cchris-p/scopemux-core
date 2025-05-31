/**
 * @file tree_sitter_bindings.c
 * @brief Python bindings for the ScopeMux Tree-sitter integration
 *
 * This file contains the implementation of the Python bindings for the
 * ScopeMux Tree-sitter integration. It exposes the tree-sitter integration API
 * to Python using pybind11.
 */

#include "../../include/scopemux/parser.h"
#include "../../include/scopemux/python_bindings.h"
#include "../../include/scopemux/python_utils.h"
#include "../../include/scopemux/tree_sitter_integration.h"

// Type definition for TreeSitterParser wrapper
typedef struct {
  PyObject_HEAD TreeSitterParser *parser;
} TreeSitterParserObject;

/**
 * @brief Deallocation function for TreeSitterParserObject
 */
static void TreeSitterParser_dealloc(TreeSitterParserObject *self) {
  if (self->parser) {
    ts_parser_free(self->parser);
  }
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Create a new TreeSitterParserObject
 */
static PyObject *TreeSitterParser_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  TreeSitterParserObject *self;
  self = (TreeSitterParserObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->parser = NULL;
  }
  return (PyObject *)self;
}

/**
 * @brief Initialize a TreeSitterParserObject
 */
static int TreeSitterParser_init(TreeSitterParserObject *self, PyObject *args, PyObject *kwds) {
  int language = LANG_UNKNOWN;
  static char *kwlist[] = {"language", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &language)) {
    return -1;
  }

  self->parser = ts_parser_init((LanguageType)language);
  if (!self->parser) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to initialize tree-sitter parser");
    return -1;
  }

  return 0;
}

/**
 * @brief Parse a string using Tree-sitter
 */
static PyObject *TreeSitterParser_parse_string(TreeSitterParserObject *self, PyObject *args,
                                               PyObject *kwds) {
  const char *content;
  Py_ssize_t content_length;
  static char *kwlist[] = {"content", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s#", kwlist, &content, &content_length)) {
    return NULL;
  }

  void *tree = ts_parser_parse_string(self->parser, content, content_length);
  if (!tree) {
    PyErr_SetString(PyExc_RuntimeError, ts_parser_get_last_error(self->parser));
    return NULL;
  }

  // We need to wrap the tree in a Python object
  // For now, we'll return a PyCapsule that contains the tree pointer
  PyObject *tree_capsule =
      PyCapsule_New(tree, "scopemux_core.TreeSitterTree", (PyCapsule_Destructor)ts_tree_free);
  if (!tree_capsule) {
    ts_tree_free(tree);
    return NULL;
  }

  return tree_capsule;
}

/**
 * @brief Convert a Tree-sitter syntax tree to ScopeMux IR
 */
static PyObject *TreeSitterParser_tree_to_ir(TreeSitterParserObject *self, PyObject *args,
                                             PyObject *kwds) {
  PyObject *tree_capsule;
  PyObject *parser_ctx_obj;
  static char *kwlist[] = {"tree", "parser_ctx", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &tree_capsule, &parser_ctx_obj)) {
    return NULL;
  }

  // Extract the tree pointer from the capsule
  void *tree = PyCapsule_GetPointer(tree_capsule, "scopemux_core.TreeSitterTree");
  if (!tree) {
    return NULL;
  }

  // Check if parser_ctx_obj is a ParserContextObject
  if (!PyObject_TypeCheck(parser_ctx_obj,
                          (PyTypeObject *)PyObject_GetAttrString(
                              PyImport_ImportModule("scopemux_core"), "ParserContext"))) {
    PyErr_SetString(PyExc_TypeError, "Expected a ParserContext object");
    return NULL;
  }

  // Extract the parser context from the object
  ParserContext *parser_ctx = ((struct { PyObject_HEAD ParserContext *ctx; } *)parser_ctx_obj)->ctx;

  // Convert the tree to IR
  bool success = ts_tree_to_ir(self->parser, tree, parser_ctx);
  if (!success) {
    PyErr_SetString(PyExc_RuntimeError, ts_parser_get_last_error(self->parser));
    return NULL;
  }

  Py_RETURN_NONE;
}

/**
 * @brief Extract comments and docstrings from a Tree-sitter syntax tree
 */
static PyObject *TreeSitterParser_extract_comments(TreeSitterParserObject *self, PyObject *args,
                                                   PyObject *kwds) {
  PyObject *tree_capsule;
  PyObject *parser_ctx_obj;
  static char *kwlist[] = {"tree", "parser_ctx", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &tree_capsule, &parser_ctx_obj)) {
    return NULL;
  }

  // Extract the tree pointer from the capsule
  void *tree = PyCapsule_GetPointer(tree_capsule, "scopemux_core.TreeSitterTree");
  if (!tree) {
    return NULL;
  }

  // Check if parser_ctx_obj is a ParserContextObject
  if (!PyObject_TypeCheck(parser_ctx_obj,
                          (PyTypeObject *)PyObject_GetAttrString(
                              PyImport_ImportModule("scopemux_core"), "ParserContext"))) {
    PyErr_SetString(PyExc_TypeError, "Expected a ParserContext object");
    return NULL;
  }

  // Extract the parser context from the object
  ParserContext *parser_ctx = ((struct { PyObject_HEAD ParserContext *ctx; } *)parser_ctx_obj)->ctx;

  // Extract comments
  size_t num_comments = ts_extract_comments(self->parser, tree, parser_ctx);

  return PyLong_FromSize_t(num_comments);
}

/**
 * @brief Extract functions and methods from a Tree-sitter syntax tree
 */
static PyObject *TreeSitterParser_extract_functions(TreeSitterParserObject *self, PyObject *args,
                                                    PyObject *kwds) {
  PyObject *tree_capsule;
  PyObject *parser_ctx_obj;
  static char *kwlist[] = {"tree", "parser_ctx", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &tree_capsule, &parser_ctx_obj)) {
    return NULL;
  }

  // Extract the tree pointer from the capsule
  void *tree = PyCapsule_GetPointer(tree_capsule, "scopemux_core.TreeSitterTree");
  if (!tree) {
    return NULL;
  }

  // Check if parser_ctx_obj is a ParserContextObject
  if (!PyObject_TypeCheck(parser_ctx_obj,
                          (PyTypeObject *)PyObject_GetAttrString(
                              PyImport_ImportModule("scopemux_core"), "ParserContext"))) {
    PyErr_SetString(PyExc_TypeError, "Expected a ParserContext object");
    return NULL;
  }

  // Extract the parser context from the object
  ParserContext *parser_ctx = ((struct { PyObject_HEAD ParserContext *ctx; } *)parser_ctx_obj)->ctx;

  // Extract functions
  size_t num_functions = ts_extract_functions(self->parser, tree, parser_ctx);

  return PyLong_FromSize_t(num_functions);
}

/**
 * @brief Extract classes and other type definitions from a Tree-sitter syntax tree
 */
static PyObject *TreeSitterParser_extract_classes(TreeSitterParserObject *self, PyObject *args,
                                                  PyObject *kwds) {
  PyObject *tree_capsule;
  PyObject *parser_ctx_obj;
  static char *kwlist[] = {"tree", "parser_ctx", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &tree_capsule, &parser_ctx_obj)) {
    return NULL;
  }

  // Extract the tree pointer from the capsule
  void *tree = PyCapsule_GetPointer(tree_capsule, "scopemux_core.TreeSitterTree");
  if (!tree) {
    return NULL;
  }

  // Check if parser_ctx_obj is a ParserContextObject
  if (!PyObject_TypeCheck(parser_ctx_obj,
                          (PyTypeObject *)PyObject_GetAttrString(
                              PyImport_ImportModule("scopemux_core"), "ParserContext"))) {
    PyErr_SetString(PyExc_TypeError, "Expected a ParserContext object");
    return NULL;
  }

  // Extract the parser context from the object
  ParserContext *parser_ctx = ((struct { PyObject_HEAD ParserContext *ctx; } *)parser_ctx_obj)->ctx;

  // Extract classes
  size_t num_classes = ts_extract_classes(self->parser, tree, parser_ctx);

  return PyLong_FromSize_t(num_classes);
}

/**
 * @brief Get the last error message from a Tree-sitter parser
 */
static PyObject *TreeSitterParser_get_last_error(TreeSitterParserObject *self,
                                                 PyObject *Py_UNUSED(args)) {
  const char *error = ts_parser_get_last_error(self->parser);
  if (!error) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(error);
}

/**
 * @brief Method definitions for TreeSitterParser
 */
static PyMethodDef TreeSitterParser_methods[] = {
    {"parse_string", (PyCFunction)TreeSitterParser_parse_string, METH_VARARGS | METH_KEYWORDS,
     "Parse a string using Tree-sitter"},
    {"tree_to_ir", (PyCFunction)TreeSitterParser_tree_to_ir, METH_VARARGS | METH_KEYWORDS,
     "Convert a Tree-sitter syntax tree to ScopeMux IR"},
    {"extract_comments", (PyCFunction)TreeSitterParser_extract_comments,
     METH_VARARGS | METH_KEYWORDS,
     "Extract comments and docstrings from a Tree-sitter syntax tree"},
    {"extract_functions", (PyCFunction)TreeSitterParser_extract_functions,
     METH_VARARGS | METH_KEYWORDS, "Extract functions and methods from a Tree-sitter syntax tree"},
    {"extract_classes", (PyCFunction)TreeSitterParser_extract_classes, METH_VARARGS | METH_KEYWORDS,
     "Extract classes and other type definitions from a Tree-sitter syntax tree"},
    {"get_last_error", (PyCFunction)TreeSitterParser_get_last_error, METH_NOARGS,
     "Get the last error message from a Tree-sitter parser"},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for TreeSitterParser
 */
static PyTypeObject TreeSitterParserType = {
    PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.TreeSitterParser",
    .tp_doc = "Tree-sitter parser wrapper for ScopeMux",
    .tp_basicsize = sizeof(TreeSitterParserObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = TreeSitterParser_new,
    .tp_init = (initproc)TreeSitterParser_init,
    .tp_dealloc = (destructor)TreeSitterParser_dealloc,
    .tp_methods = TreeSitterParser_methods,
};

/**
 * @brief Initialize the tree-sitter bindings
 *
 * @param m Python module
 */
void init_tree_sitter_bindings(void *m) {
  PyObject *module = (PyObject *)m;

  // Finalize the types
  if (PyType_Ready(&TreeSitterParserType) < 0) {
    return;
  }

  // Add the types to the module
  Py_INCREF(&TreeSitterParserType);
  PyModule_AddObject(module, "TreeSitterParser", (PyObject *)&TreeSitterParserType);
}
