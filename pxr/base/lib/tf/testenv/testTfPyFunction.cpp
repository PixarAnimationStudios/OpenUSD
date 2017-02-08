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
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyInterpreter.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/function.hpp>

#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>

#include <functional>

using namespace boost::python;
PXR_NAMESPACE_USING_DIRECTIVE

static const char VoidFuncSource[] = "def VoidFunc(): pass\n";
static const char BoolFuncSource[] = "def BoolFunc(): return True\n";
static const char IntFuncSource[] = "def IntFunc(): return 13\n";
static const char LongFuncSource[] = "def LongFunc(): return 17L\n";
static const char DoubleFuncSource[] = "def DoubleFunc(): return 19.0\n";
static const char StringFuncSource[] = "def StringFunc(): return 'a string'\n";
static const char ObjectFuncSource[] = "def ObjectFunc(): return testObject\n";

template <typename T>
static void
AssertCallResult(object callable, T const &expected)
{
    extract<std::function<T()>> stdFunc(callable);
    TF_AXIOM(stdFunc.check());
    TF_AXIOM(stdFunc()() == expected);

    extract<boost::function<T()>> boostFunc(callable);
    TF_AXIOM(boostFunc.check());
    TF_AXIOM(boostFunc()() == expected);
}

static void
AssertCallVoid(object callable)
{
    // As there's no result to check, all we care about is that calling the
    // function does not throw.
    extract<std::function<void()>> stdFunc(callable);
    TF_AXIOM(stdFunc.check());
    stdFunc()();

    extract<boost::function<void()>> boostFunc(callable);
    TF_AXIOM(boostFunc.check());
    boostFunc()();
}

static object
DefineFunc(const char *funcName, const char *funcSource, dict testEnv)
{
    TfPyRunString(funcSource, Py_single_input, testEnv);
    return testEnv[funcName];
}

int
main(int argc, char **argv)
{
    TfPyInitialize();

    TfPyLock lock;

    // Import Tf to make sure that we get the function wrappings defined in
    // wrapFunction.cpp.
    object tfModule = import("pxr.Tf");
    TF_AXIOM(!tfModule.is_none());

    // Store our test functions in this dictionary rather than the main module.
    dict testEnv;
    testEnv.update(import("__builtin__").attr("__dict__"));

    // Expected results of calling functions
    const bool expectedBool = true;
    const int expectedInt = 13;
    const long expectedLong = 17;
    const double expectedDouble = 19;
    const std::string expectedString = "a string";
    object expectedObject = TfPyEvaluate("object()");
    testEnv["testObject"] = expectedObject;

    // Define and test regular functions
    object voidFunc = DefineFunc("VoidFunc", VoidFuncSource, testEnv);
    TF_AXIOM(!voidFunc.is_none());
    object boolFunc = DefineFunc("BoolFunc", BoolFuncSource, testEnv);
    TF_AXIOM(!boolFunc.is_none());
    object intFunc = DefineFunc("IntFunc", IntFuncSource, testEnv);
    TF_AXIOM(!intFunc.is_none());
    object longFunc = DefineFunc("LongFunc", LongFuncSource, testEnv);
    TF_AXIOM(!longFunc.is_none());
    object doubleFunc = DefineFunc("DoubleFunc", DoubleFuncSource, testEnv);
    TF_AXIOM(!doubleFunc.is_none());
    object stringFunc = DefineFunc("StringFunc", StringFuncSource, testEnv);
    TF_AXIOM(!stringFunc.is_none());
    object objectFunc = DefineFunc("ObjectFunc", ObjectFuncSource, testEnv);
    TF_AXIOM(!objectFunc.is_none());

    AssertCallVoid(voidFunc);
    AssertCallResult<bool>(boolFunc, expectedBool);
    AssertCallResult<int>(intFunc, expectedInt);
    AssertCallResult<long>(longFunc, expectedLong);
    AssertCallResult<double>(doubleFunc, expectedDouble);
    AssertCallResult<std::string>(stringFunc, expectedString);
    AssertCallResult<object>(objectFunc, expectedObject);

    // Define and test lambda functions
    object voidLambda = TfPyEvaluate("lambda: None");
    TF_AXIOM(!voidLambda.is_none());
    object boolLambda = TfPyEvaluate("lambda: True");
    TF_AXIOM(!boolLambda.is_none());
    object intLambda = TfPyEvaluate("lambda: 13");
    TF_AXIOM(!intLambda.is_none());
    object longLambda = TfPyEvaluate("lambda: 17L");
    TF_AXIOM(!longLambda.is_none());
    object doubleLambda = TfPyEvaluate("lambda: 19.0");
    TF_AXIOM(!doubleLambda.is_none());
    object stringLambda = TfPyEvaluate("lambda: 'a string'");
    TF_AXIOM(!stringLambda.is_none());
    object objectLambda = TfPyEvaluate("lambda: testObject", testEnv);
    TF_AXIOM(!objectLambda.is_none());

    AssertCallVoid(voidLambda);
    AssertCallResult<bool>(boolLambda, expectedBool);
    AssertCallResult<int>(intLambda, expectedInt);
    AssertCallResult<long>(longLambda, expectedLong);
    AssertCallResult<double>(doubleLambda, expectedDouble);
    AssertCallResult<std::string>(stringLambda, expectedString);
    AssertCallResult<object>(objectLambda, expectedObject);
}
