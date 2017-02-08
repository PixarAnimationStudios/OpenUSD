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

#include "pxr/pxr.h"
#include "pxr/base/tf/setenv.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/handle.hpp>
#include <boost/python/extract.hpp>

#include <mutex>
#include <string>
#include <cstdio>

using std::string;
using namespace boost::python;
PXR_NAMESPACE_USING_DIRECTIVE

static unsigned int
_CheckResultInEnv(const string & envName, const string & envVal)
{
    string result = TfGetenv(envName);
    if (result != envVal) {
        printf("ERROR: Expected '%s', got '%s'.\n",
               envVal.c_str(),
               result.c_str());

        return 1;
    }

    return 0;
}

static unsigned int
_CheckResultInOsEnviron(const string & envName, const string & envVal)
{
    static std::once_flag once;
    std::call_once(once, [](){
        TfPyRunSimpleString("import os\n");
    });

    TfPyLock lock;

    string cmd = TfStringPrintf("os.environ['%s']", envName.c_str());
    handle<> wrappedResult = TfPyRunString(cmd, Py_eval_input);

    if (!wrappedResult) {
        printf("ERROR: Python returned no result.\n");
        return 1;
    }

    extract<string> getString(wrappedResult.get());

    if (!getString.check()) {
        printf("ERROR: Python returned non-string result.\n");
        return 1;
    }

    string result = getString();
    if (result != envVal) {
        printf("ERROR: Expected '%s', got '%s'.\n",
               envVal.c_str(),
               result.c_str());

        return 1;
    }

    return 0;
}

static unsigned int
_CheckResultNotInOsEnviron(const string & envName)
{
    static std::once_flag once;
    std::call_once(once, [](){
        TfPyRunSimpleString("import os\n");
    });

    TfPyLock lock;

    string cmd = TfStringPrintf("'%s' not in os.environ", envName.c_str());
    handle<> wrappedResult = TfPyRunString(cmd, Py_eval_input);

    if (!wrappedResult) {
        printf("ERROR: Python returned no result.\n");
        return 1;
    }

    extract<bool> getBool(wrappedResult.get());

    if (!getBool.check()) {
        printf("ERROR: Python returned non-string result.\n");
        return 1;
    }

    bool result = getBool();
    if (!result) {
        printf("ERROR: Expected key '%s' not appear in os.environ.\n",
               envName.c_str());

        return 1;
    }

    return 0;
}

static unsigned int
_TestPySetenvNoInit()
{
    // Test that calling TfPySetenv causes an error.

    unsigned int numErrors = 0;

    const string envName = "PY_TEST_ENV_NAME";
    const string envVal = "TestPySetenvNoInit";

    if (TfPyIsInitialized()) {
        numErrors += 1;
        printf("ERROR: Python should not yet be initialized.\n");
        return numErrors;
    }

    {
        TfErrorMark m;
        fprintf(stderr, "===== Expected Error =====\n");
        bool didSet = TfPySetenv(envName, envVal);
        fprintf(stderr, "=== End Expected Error ===\n");
        if (didSet) {
            numErrors += 1;
            printf("ERROR: Calling TfPySetenv with uninitialized Python "
                   "should return false.");
        }

        if (m.IsClean()) {
            numErrors += 1;
            printf("ERROR: Calling TfPySetenv with uninitialized Python "
                   "should produce an error.");
        }
        m.Clear();
    }

    if (TfPyIsInitialized()) {
        numErrors += 1;
        printf("ERROR: Python should not yet be initialized.\n");
        return numErrors;
    }

    numErrors += _CheckResultInEnv(envName, "");

    return numErrors;
}

static unsigned int
_TestPySetenvInit()
{
    // Initialize Python and verify that we can set/unset values and have them
    // appear in os.environ.  Setting them into os.environ will also propagate
    // them to the process environment.

    unsigned int numErrors = 0;

    const string envName = "PY_TEST_ENV_NAME";
    const string envVal = "TestPySetenvInit";

    TfPyInitialize();

    TfPySetenv(envName, envVal);

    numErrors += _CheckResultInEnv(envName, envVal);
    numErrors += _CheckResultInOsEnviron(envName, envVal);

    TfPyUnsetenv(envName);

    numErrors += _CheckResultInEnv(envName, "");
    numErrors += _CheckResultNotInOsEnviron(envName);

    return numErrors;
}

static unsigned int
_TestSetenvNoInit()
{
    // Test that calling TfSetenv/TfUnsetenv without Python initialized still
    // sets/unsets the value into the process environment.

    unsigned int numErrors = 0;

    const string envName = "TEST_ENV_NAME";
    const string envVal = "TestSetenvNoInit";

    if (TfPyIsInitialized()) {
        numErrors += 1;
        printf("ERROR: Python should not yet be initialized.\n");
        return numErrors;
    }

    {
        TfErrorMark m;

        if (!TfSetenv(envName, envVal)) {
            numErrors += 1;
            printf("ERROR: Setenv failed\n");
        }

        // Depend on the Tf error system to ouput any error messages.
        size_t n = 0;
        m.GetBegin(&n);
        numErrors += n;

        m.Clear();
    }

    if (TfPyIsInitialized()) {
        numErrors += 1;
        printf("ERROR: Python should not yet be initialized.\n");
    }

    numErrors += _CheckResultInEnv(envName, envVal);

    if (!TfUnsetenv(envName)) {
        numErrors += 1;
        printf("ERROR: Unsetenv failed\n");
    }

    numErrors += _CheckResultInEnv(envName, "");

    return numErrors;
}

static unsigned int
_TestSetenvInit()
{
    // Test that TfSetenv/TfUnsetenv sets/unsets the value into both the
    // process environment and os.environ.

    unsigned int numErrors = 0;

    const string envName = "TEST_ENV_NAME";
    const string envVal = "TestSetenvInit";

    TfPyInitialize();

    TfSetenv(envName, envVal);

    numErrors += _CheckResultInEnv(envName, envVal);
    numErrors += _CheckResultInOsEnviron(envName, envVal);

    TfUnsetenv(envName);

    numErrors += _CheckResultInEnv(envName, "");
    numErrors += _CheckResultNotInOsEnviron(envName);

    return numErrors;
}

static bool
Test_TfSetenv(int argc, char **argv)
{
    unsigned int numErrors = 0;

    numErrors += _TestSetenvNoInit();
    numErrors += _TestPySetenvNoInit();
    numErrors += _TestSetenvInit();
    numErrors += _TestPySetenvInit();

    bool success = (numErrors == 0);

    // Print status
    if (success) {
        printf("\nTest SUCCEEDED\n");
    } else {
        printf("\nTest FAILED\n");
    }

    return success;
}

TF_ADD_REGTEST(TfSetenv);
