/**
 * @file python_utils.h
 * @brief Utility header for Python C API integration
 *
 * This header wraps Python C API includes and provides necessary definitions
 * for Python C extension modules. It ensures proper integration with Python
 */

#ifndef SCOPEMUX_PYTHON_UTILS_H
#define SCOPEMUX_PYTHON_UTILS_H

/**
 * @brief Python C API utility header
 *
 * This header provides the necessary type definitions and utilities for
 * interfacing ScopeMux with the Python C API. It ensures consistent
 * struct definitions and Python object handling across all binding files.
 */

/* PY_SSIZE_T_CLEAN is already defined by the build system in CMakeLists.txt */
/* No need to define it here, removed to avoid redefinition warnings */

/* Include Python.h first before using any Python macros */
#include <Python.h>
#include <structmember.h>

/* Common standard C headers that will be needed by all modules */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for ScopeMux types - only define if not already defined */
#ifndef PARSER_H_INCLUDED
struct ParserContext;
#endif

#ifndef CONTEXT_ENGINE_H_INCLUDED
struct ContextEngine;
struct InfoBlock;
#endif

/**
 * @brief Python wrapper object for ASTNode
 */
typedef struct {
  PyObject_HEAD
      /* The C ASTNode instance */
      struct ASTNode *node;
  /* Flag to indicate if we own the ASTNode memory */
  int owned;
} ASTNodeObject;

/**
 * @brief Python wrapper object for CSTNode
 */
typedef struct {
  PyObject_HEAD
      /* The C CSTNode instance */
      struct CSTNode *node;
  /* Flag to indicate if we own the CSTNode memory */
  int owned;
} CSTNodeObject;

/**
 * @brief Python wrapper object for ParserContext
 */
typedef struct {
  PyObject_HEAD
      /* The C ParserContext instance */
      struct ParserContext *context;
} ParserContextObject;

/**
 * @brief Python wrapper object for ContextEngine
 */
typedef struct {
  PyObject_HEAD
      /* The C ContextEngine instance */
      struct ContextEngine *engine;
} ContextEngineObject;

/**
 * @brief Python wrapper object for InfoBlock
 */
typedef struct {
  PyObject_HEAD
      /* The C InfoBlock instance */
      struct InfoBlock *block;
  /* Keep reference to parent engine */
  ContextEngineObject *engine_obj;
  /* Flag to indicate if we own the InfoBlock memory */
  int owned;
} InfoBlockObject;

/* Forward declarations of PyTypeObject for nodes */
extern PyTypeObject ASTNodePyType;
extern PyTypeObject CSTNodePyType;

#endif /* SCOPEMUX_PYTHON_UTILS_H */
