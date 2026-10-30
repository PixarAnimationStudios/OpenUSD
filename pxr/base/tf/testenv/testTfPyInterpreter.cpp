//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// testTfPyInterpreter.mm

#include "pxr/pxr.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"

#include <nanobind/nanobind.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static unsigned int
testInterpreter(bool verbose)
{
    unsigned int numErrors = 0;
    
    TfPyInitialize();
    TfPyLock pyLock;
    TfPyRunSimpleString("2+2");
    
    nanobind::object result = TfPyRunString("'hello'\n", Py_eval_input);
    if (!result.is_valid() || !(PyBytes_Check(result.ptr()) || 
                        PyUnicode_Check(result.ptr())) || 
                    (strcmp(PyUnicode_AsUTF8(result.ptr()), "hello") != 0)) {
        if (!result.is_valid()) {
            printf("ERROR: TfPyRunString, no result.\n");
        } else if (result.ptr() == Py_None) {
            printf("ERROR: TfPyRunString, result is None.\n");
        } else if (!(PyBytes_Check(result.ptr()) || 
                    PyUnicode_Check(result.ptr()))) {
            printf("ERROR: TfPyRunString, result not a string.\n");
        } else if (strcmp(PyUnicode_AsUTF8(result.ptr()), "hello") != 0) {
            printf("ERROR: TfPyRunString, string not expected (%s).\n", 
                    PyUnicode_AsUTF8(result.ptr()));
        }
        //PyObject_Print(result, fdopen(STDOUT_FILENO, "w"), 0);
        numErrors++;
    } else if (verbose) {
        printf("TfPyRunString, seems good.\n");
    }
    
    return numErrors;
}



int
main(int argc, char **argv)
{
    bool verbose = ((argc == 2) && (strcmp(argv[1], "-v") == 0));
    unsigned int numErrors = 0;
    
    numErrors += testInterpreter(verbose);

    // Print status
    if (numErrors > 0) {
        printf("\nTest FAILED\n");
    } else if (verbose) {
        printf("\nTest SUCCEEDED\n");
    }
    
    return numErrors;
}
