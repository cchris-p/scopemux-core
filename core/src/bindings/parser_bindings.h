/**
 * @file parser_bindings.h
 * @brief Header file for Python parser bindings 
 */

#ifndef PARSER_BINDINGS_H
#define PARSER_BINDINGS_H

#include <Python.h>

/**
 * Initialize the parser bindings for the Python module
 * 
 * @param m Pointer to the Python module
 */
void init_parser_bindings(void *m);

#endif /* PARSER_BINDINGS_H */
