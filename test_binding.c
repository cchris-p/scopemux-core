#define PY_SSIZE_T_CLEAN
#include <Python.h>

static PyObject* hello_world(PyObject* self, PyObject* args) {
    return PyUnicode_FromString("Hello from ScopeMux C bindings!");
}

static PyMethodDef TestMethods[] = {
    {"hello", hello_world, METH_NOARGS, "Return a hello message"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef testmodule = {
    PyModuleDef_HEAD_INIT,
    "test_scopemux",
    "Test ScopeMux C extension",
    -1,
    TestMethods
};

PyMODINIT_FUNC PyInit_test_scopemux(void) {
    return PyModule_Create(&testmodule);
}
