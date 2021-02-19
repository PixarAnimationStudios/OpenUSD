//
// Copyright 2021 Pixar
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
#include "pxr/base/tf/pyInvoke.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyExceptionState.h"
#include "pxr/base/tf/pyInterpreter.h"

#include <boost/python.hpp>
#include <iostream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE


#define DO_TEST(test, expectOk) DoTest(#test, &test, expectOk)

static void DoTest(const std::string &name, void(*func)(), bool expectOk)
{
    // List the test case.
    std::cout << "------------" << std::endl << name << std::endl << std::endl;

    // Set up an error mark to track errors.  Make sure we have the GIL first,
    // since destroying the error mark can deallocate objects from inside the
    // Python interpreter.
    TfPyLock pyLock;
    TfErrorMark errorMark;

    // Run the test.
    func();

    // Verify we either do or don't have errors, as expected.
    TF_AXIOM(errorMark.IsClean() == expectOk);

    // Print any errors raised.  This isn't used for pass/fail purposes, but it
    // lets us manually verify diagnostic info for expected errors.
    //
    // XXX: in Python 3, several of our error cases return a TfPyExceptionState,
    // but it returns an empty exception string.
    for (const TfError &err : errorMark) {
        const TfPyExceptionState* const exc = err.GetInfo<TfPyExceptionState>();
        if (exc) {
            std::cout << exc->GetExceptionString() << std::endl;
        } else {
            std::cout << err.GetDiagnosticCodeAsString() << std::endl;
            std::cout << err.GetCommentary() << std::endl;
        }
    }
}

//-----------

static void TestInvokeAndExtract_NoArgs()
{
    std::string result;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_NoArgs",
        &result);
    TF_AXIOM(ok == true);
    TF_AXIOM(result == "_NoArgs result");
}

static void TestInvokeAndReturn_NoArgs()
{
    boost::python::object result;
    const bool ok = TfPyInvokeAndReturn(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_ReturnInt",
        &result);
    TF_AXIOM(ok == true);
    TF_AXIOM(boost::python::extract<double>(result) == 42);
}

static void TestInvoke_Simple()
{
    const bool ok = TfPyInvoke(
        "pxr.Tf",
        "Debug.IsDebugSymbolNameEnabled",
        "TF_NONEXISTENT_DEBUG_SYMBOL");
    TF_AXIOM(ok == true);
}

static void TestInvokeAndExtract_Simple()
{
    bool result = false;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf",
        "Debug.IsDebugSymbolNameEnabled",
        &result,
        "TF_NONEXISTENT_DEBUG_SYMBOL");
    TF_AXIOM(ok == true);
    TF_AXIOM(result == false);
}

static void TestInvokeAndExtract_Complex()
{
    // Vector-of-strings is wrapped in both directions by
    // wrapPyContainerConversions; rely on that.
    std::vector<std::string> input{"ab", "cd"};
    std::vector<std::string> expected{"abab", "cdcd"};
    std::vector<std::string> result;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_RepeatStrings",
        &result,
        input,
        /* numRepeats = */ 2);
    TF_AXIOM(ok == true);
    TF_AXIOM(result == expected);
}

static void TestInvokeAndExtract_ListArgs()
{
    std::string result;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_ConcatWithList",
        &result,
        "one",
        2,
        "three",
        "four");
    TF_AXIOM(ok == true);
    TF_AXIOM(result == "one 2 three four");
}

static void TestInvokeAndExtract_KwArgs()
{
    std::string result;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_ConcatWithKwArgs",
        &result,
        "one",
        "two",
        TfPyKwArg("arg4", "x"),
        TfPyKwArg("kwargA", 7),
        TfPyKwArg("kwargB", "t"));
    TF_AXIOM(ok == true);
    TF_AXIOM(result == "one two c x kwargA=7 kwargB=t");
}

static void TestInvoke_NoneType()
{
    // Pass None (from nullptr) as an arg.
    const bool ok = TfPyInvoke(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_GetTheeToANonery",
        nullptr);
    TF_AXIOM(ok == true);
}

//-----------

static void TestInvoke_NonexistentModule()
{
    const bool ok = TfPyInvoke(
        "pxr.NonexistentModule",
        "NonexistentFunction");
    TF_AXIOM(ok == false);
}

static void TestInvoke_NonexistentFunction()
{
    const bool ok = TfPyInvoke(
        "pxr.Tf",
        "NonexistentFunction");
    TF_AXIOM(ok == false);
}

static void TestInvoke_NonCallable()
{
    const bool ok = TfPyInvoke(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_globalVar");
    TF_AXIOM(ok == false);
}

static void TestInvoke_BadParamType()
{
    struct {} param;
    const bool ok = TfPyInvoke(
        "pxr.Tf",
        "Debug.IsDebugSymbolNameEnabled",
        param);
    TF_AXIOM(ok == false);
}

static void TestInvoke_WrongParamType()
{
    const bool ok = TfPyInvoke(
        "pxr.Tf",
        "Debug.IsDebugSymbolNameEnabled",
        5);
    TF_AXIOM(ok == false);
}

// Uncomment to test refusal to compile this call.
/*
static void TestInvoke_BadKwArgOrder()
{
    TfPyInvoke(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_ConcatWithKwArgs",
        1,
        TfPyKwArg("arg3", 4),
        17);
}
*/

static void TestInvokeAndExtract_BadResultType()
{
    struct {} result;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf",
        "Debug.IsDebugSymbolNameEnabled",
        &result,
        "TF_NONEXISTENT_DEBUG_SYMBOL");
    TF_AXIOM(ok == false);
}

static void TestInvokeAndExtract_WrongResultType()
{
    std::string result;
    const bool ok = TfPyInvokeAndExtract(
        "pxr.Tf",
        "Debug.IsDebugSymbolNameEnabled",
        &result,
        "TF_NONEXISTENT_DEBUG_SYMBOL");
    TF_AXIOM(ok == false);
}

static void TestInvoke_Exception()
{
    const bool ok = TfPyInvoke(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_RaiseException");
    TF_AXIOM(ok == false);
}

static void TestInvoke_TfError()
{
    const bool ok = TfPyInvoke(
        "pxr.Tf.testenv.testTfPyInvoke_callees",
        "_RaiseTfError");
    TF_AXIOM(ok == false);
}

//-----------

int main(int argc, char *argv[])
{
    // Don't print errors to stderr.  We'll print them to stdout instead.
    TfDiagnosticMgr::GetInstance().SetQuiet(true);

    // Run success-case tests.
    DO_TEST(TestInvokeAndExtract_NoArgs, true);
    DO_TEST(TestInvokeAndReturn_NoArgs, true);
    DO_TEST(TestInvoke_Simple, true);
    DO_TEST(TestInvokeAndExtract_Simple, true);
    DO_TEST(TestInvokeAndExtract_Complex, true);
    DO_TEST(TestInvokeAndExtract_ListArgs, true);
    DO_TEST(TestInvokeAndExtract_KwArgs, true);
    DO_TEST(TestInvoke_NoneType, true);

    // Run failure-case tests.
    DO_TEST(TestInvoke_NonexistentModule, false);
    DO_TEST(TestInvoke_NonexistentFunction, false);
    DO_TEST(TestInvoke_NonCallable, false);
    DO_TEST(TestInvoke_BadParamType, false);
    DO_TEST(TestInvoke_WrongParamType, false);
    DO_TEST(TestInvokeAndExtract_BadResultType, false);
    DO_TEST(TestInvokeAndExtract_WrongResultType, false);
    DO_TEST(TestInvoke_Exception, false);
    DO_TEST(TestInvoke_TfError, false);
}
