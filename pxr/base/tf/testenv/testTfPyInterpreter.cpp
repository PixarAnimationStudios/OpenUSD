//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
// testTfPyInterpreter.mm

#include "pxr/pxr.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyLock.h"

#include <boost/python/handle.hpp>

#include <string>

using namespace boost::python;
PXR_NAMESPACE_USING_DIRECTIVE

static unsigned int
testInterpreter(bool verbose)
{
    unsigned int numErrors = 0;
    
    TfPyInitialize();
    TfPyLock pyLock;
    TfPyRunSimpleString("2+2");
    
    handle<> result = TfPyRunString("'hello'\n", Py_eval_input);
    if (!result || !(PyBytes_Check(result.get()) || 
                        PyUnicode_Check(result.get())) || 
                    (strcmp(PyUnicode_AsUTF8(result.get()), "hello") != 0)) {
        if (!result) {
            printf("ERROR: TfPyRunString, no result.\n");
        } else if (result.get() == Py_None) {
            printf("ERROR: TfPyRunString, result is None.\n");
        } else if (!(PyBytes_Check(result.get()) || 
                    PyUnicode_Check(result.get()))) {
            printf("ERROR: TfPyRunString, result not a string.\n");
        } else if (strcmp(PyUnicode_AsUTF8(result.get()), "hello") != 0) {
            printf("ERROR: TfPyRunString, string not expected (%s).\n", 
                    PyUnicode_AsUTF8(result.get()));
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
