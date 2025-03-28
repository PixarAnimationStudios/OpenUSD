/*
 * Copyright 2023 Pixar
 *
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
 */


#define PY_SSIZE_T_CLEAN  /* Make "s#" use Py_ssize_t rather than int. */

// On windows, Python.h will try to force linking against pythonX.YY_d.lib if
// _DEBUG is defined... we only want to do that if PXR_USE_DEBUG_PYTHON is on
#if (defined(_WIN32) || defined(_WIN64)) && defined(_DEBUG) && !defined(PXR_USE_DEBUG_PYTHON)
  #undef _DEBUG
  #include <Python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif

/* Will be defined in testTfPyDllLinkImplementation.dll */
int testTfPyDllLinkImplementation();

static PyObject *
testTfPyDllLinkModule_callImplementation(PyObject *self, PyObject *args)
{
    return PyLong_FromLong(testTfPyDllLinkImplementation());
}

static PyMethodDef testTfPyDllLinkModule_methods[] = {
    /* The cast of the function is necessary since PyCFunction values
     * only take two PyObject* parameters, and keywdarg_parrot() takes
     * three.
     */
    {"call_implementation", testTfPyDllLinkModule_callImplementation, METH_NOARGS,
     "Make a call to a function implemented in another .dll."},
    {NULL, NULL, 0, NULL}   /* sentinel */
};

static struct PyModuleDef testTfPyDllLinkModule = {
    PyModuleDef_HEAD_INIT,
    "_testTfPyDllLink",
    NULL,
    -1,
    testTfPyDllLinkModule_methods
};

PyMODINIT_FUNC
PyInit__testTfPyDllLinkModule(void)
{
    return PyModule_Create(&testTfPyDllLinkModule);
}