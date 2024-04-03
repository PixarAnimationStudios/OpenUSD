/*
 * Copyright 2023 Pixar
 *
 * Licensed under the Apache License, Version 2.0 (the "Apache License")
 * with the following modification; you may not use this file except in
 * compliance with the Apache License and the following modification to it:
 * Section 6. Trademarks. is deleted and replaced with:
 *
 * 6. Trademarks. This License does not grant permission to use the trade
 *    names, trademarks, service marks, or product names of the Licensor
 *    and its affiliates, except as required to comply with Section 4(c) of
 *    the License and to reproduce the content of the NOTICE file.
 *
 * You may obtain a copy of the Apache License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the Apache License with the above modification is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the Apache License for the specific
 * language governing permissions and limitations under the Apache License.
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