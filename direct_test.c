#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* A simple C function that will be callable from Python */
static PyObject* hello_world(PyObject* self, PyObject* args) {
    return PyUnicode_FromString("Hello from ScopeMux C bindings!");
}

/* Method definition table */
static PyMethodDef HelloMethods[] = {
    {"hello", hello_world, METH_NOARGS, "Return a hello message"},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

/* Module definition */
static struct PyModuleDef hellomodule = {
    PyModuleDef_HEAD_INIT,
    "hello_scopemux",
    "ScopeMux C extension test",
    -1,
    HelloMethods
};

/* Module initialization function */
PyMODINIT_FUNC PyInit_hello_scopemux(void) {
    return PyModule_Create(&hellomodule);
}
