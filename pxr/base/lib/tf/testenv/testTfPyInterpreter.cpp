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

#include "pxr/base/tf/pyInterpreter.h"

#include <boost/python/handle.hpp>

#include <string>

using namespace boost::python;

static unsigned int
testInterpreter(bool verbose)
{
    unsigned int numErrors = 0;
    
    /*
    putenv("PYTHONSTARTUP=/tmp/testTfPyInterpreter_Startup.py");
    NSString *startupStr = @"import sys\n";
    [[NSFileManager defaultManager] removeFileAtPath:@"/tmp/testTfPyInterpreter_Startup.py" handler:nil];
    [startupStr writeToFile:@"/tmp/testTfPyInterpreter_Startup.py" atomically:YES];
    */
    
    TfPyInitialize();
    TfPyRunSimpleString("2+2");
    
    handle<> result = TfPyRunString("'hello'\n", Py_eval_input);
    if (!result || !PyString_Check(result.get()) ||
        (strcmp(PyString_AsString(result.get()), "hello") != 0)) {
        if (!result) {
            printf("ERROR: TfPyRunString, no result.\n");
        } else if (result.get() == Py_None) {
            printf("ERROR: TfPyRunString, result is None.\n");
        } else if (!PyString_Check(result.get())) {
            printf("ERROR: TfPyRunString, result not a string.\n");
        } else if (strcmp(PyString_AsString(result.get()), "hello") != 0) {
            printf("ERROR: TfPyRunString, string not expected (%s).\n", PyString_AsString(result.get()));
        }
        //PyObject_Print(result, fdopen(STDOUT_FILENO, "w"), 0);
        numErrors++;
    } else if (verbose) {
        printf("TfPyRunString, seems good.\n");
    }
    
    std::string modPath = TfPyGetModulePath("__main__");
    if (modPath != "__main__") {
        printf("ERROR: TfPyGetModulePath, no path returned.\n");
        numErrors++;
    } else if (verbose) {
        printf("TfPyGetModulePath, module at path '%s', good.\n", modPath.c_str());
    }

    modPath = TfPyGetModulePath("badmodule");
    if (not modPath.empty()) {
        printf("ERROR: TfPyGetModulePath, bad module name returned result '%s'.\n", modPath.c_str());
        numErrors++;
    } else if (verbose) {
        printf("TfPyGetModulePath, bad module name returned nil, good\n");
    }
    
    /*
    [[NSFileManager defaultManager] removeFileAtPath:@"/tmp/testTfPyInterpreter_Startup.py" handler:nil];
    */

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
