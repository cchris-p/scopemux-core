/**
 * @file parser_bindings.c
 * @brief Python bindings for the ScopeMux parser
 *
 * This file contains the implementation of the Python bindings for the
 * ScopeMux parser. It exposes the parser API to Python using pybind11.
 */

/* Python header includes must come first */
#ifndef PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN
#endif
#include <Python.h>
#include <structmember.h>

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ScopeMux header includes */
#include "../../core/include/scopemux/ast.h"
#include "../../core/include/scopemux/parser.h"
#include "../../core/include/scopemux/python_bindings.h"
#include "../../core/include/scopemux/python_utils.h"
#include "../../core/include/tree_sitter/api.h"

/* Tree-sitter function declarations */
void ts_parser_delete(TSParser *parser);

/* Type definitions for nodes */
PyTypeObject ASTNodePyType;
PyTypeObject CSTNodePyType;

// These type definitions are now in python_utils.h
// ParserContextObject - Python wrapper for ParserContext

/**
 * @brief Deallocation function for ParserContextObject
 * Ensures proper cleanup of parser context and CST nodes
 * This function is critical for preventing memory leaks and double free issues
 */
static void ParserContext_dealloc(ParserContextObject *self) {
  printf("[PYTHON_DEALLOC] ENTER: self=%p\n", (void *)self);
  fflush(stdout);

  // Guard against NULL self pointer
  if (!self) {
    printf("[PYTHON_DEALLOC] EXIT: self is NULL\n");
    fflush(stdout);
    return;
  }

  printf("[PYTHON_DEALLOC] Python object type: %s\n", Py_TYPE(self)->tp_name);
  fflush(stdout);

  // Check if we have a valid context to clean up
  if (self->context) {
    printf("[PYTHON_DEALLOC] Starting cleanup of parser context at %p\n", (void *)self->context);
    fflush(stdout);

    // CRITICAL: First ensure CST root is cleared before freeing the parser context
    // This prevents double free issues when the parser context is freed
    printf("[PYTHON_DEALLOC] Clearing CST root before freeing parser context\n");
    fflush(stdout);

    // Use parser_set_cst_root with NULL to safely clear the CST root
    // This function handles proper cleanup of any existing CST root
    parser_set_cst_root(self->context, NULL);

    // Now that CST root is safely cleared, free the parser context
    printf("[PYTHON_DEALLOC] Now freeing parser context at %p\n", (void *)self->context);
    fflush(stdout);

    // parser_free will handle all remaining cleanup including AST nodes
    parser_free(self->context);

    // Clear the pointer to avoid any potential use-after-free
    self->context = NULL;

    printf("[PYTHON_DEALLOC] Parser context cleanup complete\n");
    fflush(stdout);
  } else {
    printf("[PYTHON_DEALLOC] No parser context to free (already NULL)\n");
    fflush(stdout);
  }

  // Free the Python object
  printf("[PYTHON_DEALLOC] About to free Python object at %p\n", (void *)self);
  fflush(stdout);

  PyTypeObject *type = Py_TYPE(self);
  printf("[PYTHON_DEALLOC] Using tp_free function at %p from type %s\n", (void *)type->tp_free,
         type->tp_name);
  fflush(stdout);

  type->tp_free((PyObject *)self);

  printf("[PYTHON_DEALLOC] EXIT: Python object freed successfully\n");
  fflush(stdout);
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

  bool success = parser_parse_file(self->context, filename, (Language)language);
  if (!success) {
    PyErr_SetString(PyExc_RuntimeError, parser_get_last_error(self->context));
    return NULL;
  }

  Py_RETURN_TRUE;
}

/**
 * @brief Parse a string
 */
static PyObject *ParserContext_parse_string(PyObject *self_obj, PyObject *args, PyObject *kwds) {
  ParserContextObject *self = (ParserContextObject *)self_obj;
  const char *content;
  Py_ssize_t content_length;
  const char *filename = NULL;
  PyObject *language_obj = NULL;
  static char *kwlist[] = {"content", "filename", "language", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s#|sO", kwlist, &content, &content_length,
                                   &filename, &language_obj)) {
    return NULL;
  }

  Language language = LANG_UNKNOWN;

  // Handle language parameter - can be string or integer
  if (language_obj != NULL) {
    if (PyUnicode_Check(language_obj)) {
      const char *lang_str = PyUnicode_AsUTF8(language_obj);
      if (lang_str) {
        if (strcmp(lang_str, "c") == 0) {
          language = LANG_C;
        } else if (strcmp(lang_str, "cpp") == 0 || strcmp(lang_str, "c++") == 0) {
          language = LANG_CPP;
        } else if (strcmp(lang_str, "python") == 0 || strcmp(lang_str, "py") == 0) {
          language = LANG_PYTHON;
        } else if (strcmp(lang_str, "javascript") == 0 || strcmp(lang_str, "js") == 0) {
          language = LANG_JAVASCRIPT;
        } else if (strcmp(lang_str, "typescript") == 0 || strcmp(lang_str, "ts") == 0) {
          language = LANG_TYPESCRIPT;
        } else {
          PyErr_SetString(PyExc_ValueError, "Unsupported language string");
          return NULL;
        }
      }
    } else if (PyLong_Check(language_obj)) {
      long lang_int = PyLong_AsLong(language_obj);
      if (lang_int >= LANG_UNKNOWN && lang_int <= LANG_TYPESCRIPT) {
        language = (Language)lang_int;
      } else {
        PyErr_SetString(PyExc_ValueError, "Invalid language integer value");
        return NULL;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Language must be a string or integer");
      return NULL;
    }
  }

  // If language is still unknown, try to detect from filename
  if (language == LANG_UNKNOWN && filename) {
    language = parser_detect_language(filename, content, content_length);
  }

  bool success = parser_parse_string(self->context, content, content_length, filename, language);
  if (!success) {
    // Make sure we have a valid error message in case of failure
    const char *safe_error_msg = "Unknown parser error";
    const char *error_msg = parser_get_last_error(self->context);
    if (error_msg != NULL) {
      safe_error_msg = error_msg;
    }
    PyErr_SetString(PyExc_RuntimeError, safe_error_msg);

    // If we have a parser that failed, make sure it's properly cleaned up
    // This prevents potential double-free or use-after-free issues
    if (self->context && self->context->ts_parser) {
      ts_parser_delete(self->context->ts_parser);
      self->context->ts_parser = NULL;
    }

    return NULL;
  }

  Py_RETURN_TRUE;
}

/**
 * @brief Get the last error message from the parser context
 */
static PyObject *ParserContext_get_last_error(PyObject *self_obj, PyObject *Py_UNUSED(args)) {
  ParserContextObject *self = (ParserContextObject *)self_obj;
  const char *error = parser_get_last_error(self->context);

  if (error == NULL) {
    Py_RETURN_NONE;
  }

  return PyUnicode_FromString(error);
}

/**
 * @brief Get the AST root node from the parsed file
 */
static PyObject *ParserContext_get_ast_root(PyObject *self_obj, PyObject *Py_UNUSED(args)) {
  ParserContextObject *self = (ParserContextObject *)self_obj;
  if (!self->context) {
    PyErr_SetString(PyExc_RuntimeError, "Parser context is not initialized");
    return NULL;
  }

  const ASTNode *ast_root_const = parser_get_ast_root(self->context);
  ASTNode *ast_root = (ASTNode *)ast_root_const; // Safe cast to remove const qualifier
  if (!ast_root) {
    PyErr_SetString(PyExc_RuntimeError,
                    "Failed to get AST root node. Make sure the file is parsed successfully.");
    return NULL;
  }

  // Create a Python AST node object
  PyTypeObject *type = &ASTNodePyType;
  ASTNodeObject *py_node = (ASTNodeObject *)type->tp_alloc(type, 0);
  if (!py_node) {
    PyErr_NoMemory();
    return NULL;
  }

  // We don't need to copy the node since the Python object will take ownership
  // of the AST node. The node will be freed when the Python object is deallocated.
  py_node->node = (ASTNode *)ast_root;
  py_node->owned = true; // Set ownership flag

  return (PyObject *)py_node;
}

/**
 * @brief Convert a CSTNode to a Python dictionary directly
 * This creates a complete deep copy of the CST node structure without maintaining
 * any references to the original C structures, avoiding memory management issues.
 */
static PyObject *cst_node_to_py_dict(const CSTNode *node) {
  if (!node) {
    Py_RETURN_NONE;
  }

  printf("[PYTHON] Converting CST node at %p (type=%s) to Python dictionary\n", (void *)node,
         node->type);
  fflush(stdout);

  // Create a new dictionary to hold the node data
  PyObject *dict = PyDict_New();
  if (!dict) {
    return NULL;
  }

  // Create a new dictionary subclass that adds the required methods
  PyObject *types_module = PyImport_ImportModule("types");
  if (!types_module) {
    Py_DECREF(dict);
    return NULL;
  }

  // Create a new type with methods
  PyObject *type_dict = PyDict_New();
  if (!type_dict) {
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }

  // Add basic data
  PyObject *type_str =
      node->type ? PyUnicode_FromString(node->type) : PyUnicode_FromString("UNKNOWN");
  PyObject *content_str =
      node->content ? PyUnicode_FromString(node->content) : PyUnicode_FromString("");

  if (!type_str || !content_str) {
    Py_XDECREF(type_str);
    Py_XDECREF(content_str);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }

  if (PyDict_SetItemString(dict, "type", type_str) < 0 ||
      PyDict_SetItemString(dict, "content", content_str) < 0) {
    Py_DECREF(type_str);
    Py_DECREF(content_str);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }
  Py_DECREF(type_str);
  Py_DECREF(content_str);

  // Add range information
  PyObject *range_dict = PyDict_New();
  if (!range_dict) {
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }

  PyObject *start_dict = PyDict_New();
  PyObject *end_dict = PyDict_New();
  if (!start_dict || !end_dict) {
    Py_XDECREF(start_dict);
    Py_XDECREF(end_dict);
    Py_DECREF(range_dict);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }

  // Add start position
  PyObject *start_line = PyLong_FromLong(node->range.start.line);
  PyObject *start_column = PyLong_FromLong(node->range.start.column);
  if (!start_line || !start_column || PyDict_SetItemString(start_dict, "line", start_line) < 0 ||
      PyDict_SetItemString(start_dict, "column", start_column) < 0) {
    Py_XDECREF(start_line);
    Py_XDECREF(start_column);
    Py_DECREF(start_dict);
    Py_DECREF(end_dict);
    Py_DECREF(range_dict);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }
  Py_DECREF(start_line);
  Py_DECREF(start_column);

  // Add end position
  PyObject *end_line = PyLong_FromLong(node->range.end.line);
  PyObject *end_column = PyLong_FromLong(node->range.end.column);
  if (!end_line || !end_column || PyDict_SetItemString(end_dict, "line", end_line) < 0 ||
      PyDict_SetItemString(end_dict, "column", end_column) < 0) {
    Py_XDECREF(end_line);
    Py_XDECREF(end_column);
    Py_DECREF(start_dict);
    Py_DECREF(end_dict);
    Py_DECREF(range_dict);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }
  Py_DECREF(end_line);
  Py_DECREF(end_column);

  // Set start and end in range_dict
  if (PyDict_SetItemString(range_dict, "start", start_dict) < 0 ||
      PyDict_SetItemString(range_dict, "end", end_dict) < 0) {
    Py_DECREF(start_dict);
    Py_DECREF(end_dict);
    Py_DECREF(range_dict);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }
  Py_DECREF(start_dict);
  Py_DECREF(end_dict);

  // Add range to main dict
  if (PyDict_SetItemString(dict, "range", range_dict) < 0) {
    Py_DECREF(range_dict);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }
  Py_DECREF(range_dict);

  // Add children
  PyObject *children = PyList_New(0);
  if (!children) {
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }

  // Process children recursively
  for (unsigned int i = 0; i < node->children_count; i++) {
    PyObject *child_dict = cst_node_to_py_dict(node->children[i]);
    if (!child_dict) {
      Py_DECREF(children);
      Py_DECREF(type_dict);
      Py_DECREF(types_module);
      Py_DECREF(dict);
      return NULL;
    }

    if (PyList_Append(children, child_dict) < 0) {
      Py_DECREF(child_dict);
      Py_DECREF(children);
      Py_DECREF(type_dict);
      Py_DECREF(types_module);
      Py_DECREF(dict);
      return NULL;
    }
    Py_DECREF(child_dict);
  }

  if (PyDict_SetItemString(dict, "children", children) < 0) {
    Py_DECREF(children);
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }
  Py_DECREF(children);

  // Add method implementations directly to the dictionary
  PyObject *methods = PyDict_New();
  if (!methods) {
    Py_DECREF(type_dict);
    Py_DECREF(types_module);
    Py_DECREF(dict);
    return NULL;
  }

  // Add method accessors that don't create circular references
  PyObject *type_ref = PyDict_GetItemString(dict, "type");
  PyObject *content_ref = PyDict_GetItemString(dict, "content");
  PyObject *range_ref = PyDict_GetItemString(dict, "range");
  PyObject *children_ref = PyDict_GetItemString(dict, "children");

  // PyDict_GetItemString returns borrowed references, don't need to incref/decref
  // We simply store pointers to existing dictionary values
  PyDict_SetItemString(dict, "get_type", type_ref);
  PyDict_SetItemString(dict, "get_content", content_ref);
  PyDict_SetItemString(dict, "get_range", range_ref);
  PyDict_SetItemString(dict, "get_children", children_ref);

  // Clean up
  Py_DECREF(methods);
  Py_DECREF(type_dict);
  Py_DECREF(types_module);

  return dict;
}

/**
 * @brief Get the CST root node from the parsed file
 * This function creates a deep copy of the CST tree as a Python dictionary
 * and then clears the CST root in the parser context to prevent double free.
 */
static PyObject *ParserContext_get_cst_root(PyObject *self_obj, PyObject *Py_UNUSED(args)) {
  printf("[GET_CST_ROOT] ENTER: self_obj=%p\n", (void *)self_obj);
  fflush(stdout);

  ParserContextObject *self = (ParserContextObject *)self_obj;
  printf("[GET_CST_ROOT] Cast to ParserContextObject: self=%p\n", (void *)self);
  fflush(stdout);

  if (!self->context) {
    printf("[GET_CST_ROOT] ERROR: Parser context is not initialized\n");
    fflush(stdout);
    PyErr_SetString(PyExc_RuntimeError, "Parser context is not initialized");
    return NULL;
  }

  printf("[GET_CST_ROOT] Parser context at %p\n", (void *)self->context);
  fflush(stdout);

  // Get the CST root node from the parser context
  const CSTNode *cst_root_const = parser_get_cst_root(self->context);
  if (!cst_root_const) {
    printf("[GET_CST_ROOT] ERROR: Failed to get CST root node\n");
    fflush(stdout);
    PyErr_SetString(PyExc_RuntimeError,
                    "Failed to get CST root node. Make sure the file is parsed successfully.");
    return NULL;
  }

  printf("[GET_CST_ROOT] Retrieved CST root at %p (type=%s)\n", (void *)cst_root_const,
         cst_root_const->type);
  fflush(stdout);

  // Create a pure Python dictionary directly from the CST node
  // This creates a deep copy without maintaining references to C structures
  printf("[GET_CST_ROOT] Converting CST node to Python dictionary...\n");
  fflush(stdout);

  PyObject *py_dict = cst_node_to_py_dict(cst_root_const);
  if (!py_dict) {
    printf("[GET_CST_ROOT] ERROR: Failed to convert CST node to Python dictionary\n");
    fflush(stdout);
    PyErr_SetString(PyExc_MemoryError, "Failed to convert CST node to Python dictionary");
    return NULL;
  }

  printf("[GET_CST_ROOT] Successfully converted CST node to Python dictionary at %p\n",
         (void *)py_dict);
  fflush(stdout);

  // CRITICAL FIX: After converting to Python dictionary, set the CST root to NULL
  // This prevents double-free when the parser context is cleaned up
  printf("[GET_CST_ROOT] Transferring ownership: Setting CST root to NULL after conversion\n");
  fflush(stdout);

  // Set the CST root to NULL in the parser context to prevent double-free
  // This transfers ownership of the CST node to Python
  printf("[GET_CST_ROOT] Before transfer: CST root at %p\n",
         (void *)parser_get_cst_root(self->context));
  fflush(stdout);

  parser_set_cst_root(self->context, NULL);

  printf("[GET_CST_ROOT] After transfer: CST root at %p\n",
         (void *)parser_get_cst_root(self->context));
  fflush(stdout);

  printf("[GET_CST_ROOT] EXIT: Returning Python dictionary at %p\n", (void *)py_dict);
  fflush(stdout);

  return py_dict;
}

static const PyMethodDef ParserContext_methods[] = {
    {"parse_file", (PyCFunction)ParserContext_parse_file, METH_VARARGS | METH_KEYWORDS,
     "Parse a file"},
    {"parse_string", (PyCFunction)ParserContext_parse_string, METH_VARARGS | METH_KEYWORDS,
     "Parse a string"},
    {"get_last_error", (PyCFunction)ParserContext_get_last_error, METH_NOARGS,
     "Get the last error message"},
    {"get_ast_root", (PyCFunction)ParserContext_get_ast_root, METH_NOARGS, "Get the AST root node"},
    {"get_cst_root", (PyCFunction)ParserContext_get_cst_root, METH_NOARGS, "Get the CST root node"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

/**
 * @brief Type definition for ParserContext
 */
static PyTypeObject ParserContextType = {PyVarObject_HEAD_INIT(NULL, 0).tp_name =
                                             "scopemux_core.ParserContext",
                                         .tp_basicsize = sizeof(ParserContextObject),
                                         .tp_itemsize = 0,
                                         .tp_dealloc = (destructor)ParserContext_dealloc,
                                         .tp_flags = Py_TPFLAGS_DEFAULT,
                                         .tp_doc = "Parser context object",
                                         .tp_methods = ParserContext_methods,
                                         .tp_init = (initproc)ParserContext_init,
                                         .tp_new = ParserContext_new};

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
  // Return the canonical string representation of the node type
  return PyUnicode_FromString(ast_node_type_to_string(self->node->type));
}

/**
 * Method-style getters for ASTNode (for compatibility with generate_expected_json.py)
 */
static PyObject *ASTNode_method_get_type(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  if (!self->node) {
    Py_RETURN_NONE;
  }
  // Return the canonical string representation of the node type
  return PyUnicode_FromString(ast_node_type_to_string(self->node->type));
}

static PyObject *ASTNode_method_get_name(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  if (!self->node || !self->node->name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->name);
}

static PyObject *ASTNode_method_get_qualified_name(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  if (!self->node || !self->node->qualified_name) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->qualified_name);
}

static PyObject *ASTNode_method_get_signature(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  if (!self->node || !self->node->signature) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->signature);
}

static PyObject *ASTNode_method_get_docstring(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  if (!self->node || !self->node->docstring) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->docstring);
}

static PyObject *ASTNode_method_get_path(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  (void)self; // ASTNode doesn't have a path field
  // Path information is not available in the ASTNode structure
  Py_RETURN_NONE;
}

static PyObject *ASTNode_method_is_system(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  (void)self; // ASTNode doesn't have an is_system field
  // System flag is not available in the ASTNode structure
  Py_RETURN_FALSE;
}

static PyObject *ASTNode_method_get_parameters(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  (void)self; // Parameters not implemented yet
  // Return an empty list as parameters are not yet implemented
  return PyList_New(0);
}

static PyObject *ASTNode_method_get_return_type(ASTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  // Return type is not yet implemented in the ASTNode structure
  Py_RETURN_NONE;
}

/**
 * Method definitions for ASTNode
 */
static const PyMethodDef ASTNode_methods[] = {
    {"get_type", (PyCFunction)ASTNode_method_get_type, METH_NOARGS, "Get the node type"},
    {"get_name", (PyCFunction)ASTNode_method_get_name, METH_NOARGS, "Get the node name"},
    {"get_qualified_name", (PyCFunction)ASTNode_method_get_qualified_name, METH_NOARGS,
     "Get the node qualified name"},
    {"get_signature", (PyCFunction)ASTNode_method_get_signature, METH_NOARGS,
     "Get the node signature"},
    {"get_docstring", (PyCFunction)ASTNode_method_get_docstring, METH_NOARGS,
     "Get the node docstring"},
    {"get_path", (PyCFunction)ASTNode_method_get_path, METH_NOARGS, "Get the node path"},
    {"is_system", (PyCFunction)ASTNode_method_is_system, METH_NOARGS,
     "Check if the node is a system node"},
    {"get_parameters", (PyCFunction)ASTNode_method_get_parameters, METH_NOARGS,
     "Get the node parameters"},
    {"get_return_type", (PyCFunction)ASTNode_method_get_return_type, METH_NOARGS,
     "Get the node return type"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

/*************************
 * CSTNode Implementation *
 *************************/

/**
 * @brief Deallocation function for CSTNodeObject
 */
static void CSTNode_dealloc(CSTNodeObject *self) {
  // Add safety check - make sure we have a valid self pointer
  if (!self) {
    return;
  }

  // Use a local variable to avoid potential use-after-free
  CSTNode *node_to_free = self->node;

  // Clear the pointer first to prevent potential double-free issues
  self->node = NULL;

  // Only free if we owned it and it's not NULL
  if (node_to_free && self->owned) {
    cst_node_free(node_to_free);
  }

  // Let Python free the actual object
  Py_TYPE(self)->tp_free((PyObject *)self);
}

/**
 * @brief Create a new CSTNodeObject
 */
static PyObject *CSTNode_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  (void)args; // Silence unused parameter warning
  (void)kwds; // Silence unused parameter warning

  CSTNodeObject *self = (CSTNodeObject *)type->tp_alloc(type, 0);
  if (self != NULL) {
    self->node = NULL;
    self->owned = false;
  }
  return (PyObject *)self;
}

/**
 * @brief Initialize a CSTNodeObject
 */
static int CSTNode_init(CSTNodeObject *self, PyObject *args, PyObject *kwds) {
  (void)args; // Silence unused parameter warning
  (void)kwds; // Silence unused parameter warning

  // The CSTNode will be set externally, nothing to initialize
  // Just make sure we start with no ownership
  self->owned = false;
  return 0;
}

/* Getters for CSTNode properties */

static PyObject *CSTNode_get_type(CSTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning

  if (!self->node || !self->node->type) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->type);
}

static PyObject *CSTNode_get_content(CSTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning

  if (!self->node || !self->node->content) {
    Py_RETURN_NONE;
  }
  return PyUnicode_FromString(self->node->content);
}

static PyObject *CSTNode_get_range(CSTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning

  if (!self->node) {
    Py_RETURN_NONE;
  }

  // Create range dictionary with start and end positions
  PyObject *range_dict = PyDict_New();
  PyObject *start_dict = PyDict_New();
  PyObject *end_dict = PyDict_New();

  PyDict_SetItemString(start_dict, "line", PyLong_FromLong(self->node->range.start.line));
  PyDict_SetItemString(start_dict, "column", PyLong_FromLong(self->node->range.start.column));

  PyDict_SetItemString(end_dict, "line", PyLong_FromLong(self->node->range.end.line));
  PyDict_SetItemString(end_dict, "column", PyLong_FromLong(self->node->range.end.column));

  PyDict_SetItemString(range_dict, "start", start_dict);
  PyDict_SetItemString(range_dict, "end", end_dict);

  // Clean up references
  Py_DECREF(start_dict);
  Py_DECREF(end_dict);

  return range_dict;
}

static PyObject *CSTNode_get_children(CSTNodeObject *self, void *closure) {
  (void)closure; // Silence the unused parameter warning

  if (!self->node) {
    return PyList_New(0);
  }

  PyObject *children_list = PyList_New(0);
  if (!children_list) {
    return NULL; // Memory error
  }

  for (size_t i = 0; i < self->node->children_count; i++) {
    CSTNode *child = self->node->children[i];
    if (!child) {
      continue;
    }

    // Create a new Python wrapper for this child
    PyTypeObject *type = &CSTNodePyType;
    CSTNodeObject *py_child = (CSTNodeObject *)type->tp_alloc(type, 0);
    if (!py_child) {
      Py_DECREF(children_list);
      PyErr_NoMemory();
      return NULL;
    }

    // We don't take ownership of the child nodes since they're owned by the parent
    py_child->node = child;
    py_child->owned = false;

    // Add to the list
    PyList_Append(children_list, (PyObject *)py_child);
    Py_DECREF(py_child); // PyList_Append increases refcount, so we can decref
  }

  return children_list;
}

/* Method-style getters for CSTNode (for compatibility with generate_expected_json.py) */

static PyObject *CSTNode_method_get_type(CSTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  return CSTNode_get_type(self, NULL);
}

static PyObject *CSTNode_method_get_content(CSTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  return CSTNode_get_content(self, NULL);
}

static PyObject *CSTNode_method_get_range(CSTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  return CSTNode_get_range(self, NULL);
}

static PyObject *CSTNode_method_get_children(CSTNodeObject *self, PyObject *args) {
  (void)args; // Silence unused parameter warning
  return CSTNode_get_children(self, NULL);
}

/* Method definitions for CSTNode */
static const PyMethodDef CSTNode_methods[] = {
    {"get_type", (PyCFunction)CSTNode_method_get_type, METH_NOARGS, "Get the node type"},
    {"get_content", (PyCFunction)CSTNode_method_get_content, METH_NOARGS, "Get the node content"},
    {"get_range", (PyCFunction)CSTNode_method_get_range, METH_NOARGS, "Get the node range"},
    {"get_children", (PyCFunction)CSTNode_method_get_children, METH_NOARGS,
     "Get the node children"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

/**
 * @brief Property getters for ASTNode
 */
static const PyGetSetDef ASTNode_getsetters[] = {
    {"name", (getter)ASTNode_get_name, NULL, "Name of the node", NULL},
    {"qualified_name", (getter)ASTNode_get_qualified_name, NULL, "Qualified name of the node",
     NULL},
    {"signature", (getter)ASTNode_get_signature, NULL, "Function/method signature", NULL},
    {"docstring", (getter)ASTNode_get_docstring, NULL, "Associated documentation", NULL},
    {"raw_content", (getter)ASTNode_get_raw_content, NULL, "Raw source code content", NULL},
    {"type", (getter)ASTNode_get_type, NULL, "Type of the node", NULL},
    {NULL} /* Sentinel */
};

static const PyGetSetDef CSTNode_getsetters[] = {
    {"type", (getter)CSTNode_get_type, NULL, "Node type", NULL},
    {"content", (getter)CSTNode_get_content, NULL, "Node content", NULL},
    {"range", (getter)CSTNode_get_range, NULL, "Node source range", NULL},
    {"children", (getter)CSTNode_get_children, NULL, "Child nodes", NULL},
    {NULL} /* Sentinel */
};

/**
 * @brief Type definition for ASTNode
 */
PyTypeObject ASTNodePyType = {PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.ASTNode",
                              .tp_basicsize = sizeof(ASTNodeObject),
                              .tp_itemsize = 0,
                              .tp_dealloc = (destructor)ASTNode_dealloc,
                              .tp_flags = Py_TPFLAGS_DEFAULT,
                              .tp_doc = "AST node representing a parsed semantic entity",
                              .tp_methods = ASTNode_methods,
                              .tp_getset = ASTNode_getsetters,
                              .tp_init = (initproc)ASTNode_init,
                              .tp_new = ASTNode_new};

/**
 * @brief Type definition for CSTNode
 */
PyTypeObject CSTNodePyType = {PyVarObject_HEAD_INIT(NULL, 0).tp_name = "scopemux_core.CSTNode",
                              .tp_basicsize = sizeof(CSTNodeObject),
                              .tp_itemsize = 0,
                              .tp_dealloc = (destructor)CSTNode_dealloc,
                              .tp_flags = Py_TPFLAGS_DEFAULT,
                              .tp_doc = "CST node representing a parsed concrete syntax entity",
                              .tp_methods = CSTNode_methods,
                              .tp_getset = CSTNode_getsetters,
                              .tp_init = (initproc)CSTNode_init,
                              .tp_new = CSTNode_new};

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

  Language language = parser_detect_language(filename, content, content_length);
  return PyLong_FromLong((long)language);
}

/**
 * @brief Module methods
 */
const PyMethodDef module_methods[] = {
    {"detect_language", (PyCFunction)detect_language, METH_VARARGS | METH_KEYWORDS,
     "Detect language from filename and optionally content"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef scopemux_core_module = {
    PyModuleDef_HEAD_INIT,
    "scopemux_core",          // Module name
    "ScopeMux core bindings", // Module docstring
    -1,                       // Size of per-interpreter state (-1 means global)
    module_methods            // Method table
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

  if (PyType_Ready(&CSTNodePyType) < 0) {
    return;
  }

  // Add the types to the module
  Py_INCREF(&ParserContextType);
  PyModule_AddObject(module, "ParserContext", (PyObject *)&ParserContextType);

  Py_INCREF(&ASTNodePyType);
  PyModule_AddObject(module, "ASTNode", (PyObject *)&ASTNodePyType);

  Py_INCREF(&CSTNodePyType);
  PyModule_AddObject(module, "CSTNode", (PyObject *)&CSTNodePyType);

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

// Module initialization is now handled in module.c
