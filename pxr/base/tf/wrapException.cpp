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
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/exception.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyErrorInternal.h"

#include <boost/python/def.hpp>
#include <boost/python/exception_translator.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

// This is created below, in the wrap function.
static PyObject *tfExceptionClass;

static void Translate(TfBaseException const &exc)
{
    // Format an error message showing the C++ throw-location for at least a few
    // frames.
    static constexpr size_t MaxFramesInMsg = 16;
    std::vector<uintptr_t> const &throwStack = exc.GetThrowStack();
    std::vector<std::string> throwStackFrames;
    std::string framesMsg;
    if (!throwStack.empty()) {
        std::stringstream throwStackText;
        ArchPrintStackFrames(throwStackText, throwStack,
                             /*skipUnknownFrames=*/true);
        std::vector<std::string> throwStackMsg =
            TfStringSplit(throwStackText.str(), "\n");

        if (throwStackMsg.size() > MaxFramesInMsg) {
            size_t additional = throwStackMsg.size() - MaxFramesInMsg;
            throwStackMsg.resize(MaxFramesInMsg);
            throwStackMsg.push_back(
                TfStringPrintf("... %zu more frame%s",
                               additional, additional == 1 ? "" : "s"));
        }
        framesMsg = TfStringJoin(throwStackMsg, "\n    ");
    }
    std::string contextMsg;
    if (TfCallContext const &cc = exc.GetThrowContext()) {
        contextMsg = TfStringPrintf(
            "%s at %s:%zu", cc.GetFunction(), cc.GetFile(), cc.GetLine());
    }

    // Raise the Python exception.
    PyErr_Format(tfExceptionClass, "%s - %s%s%s%s",
                 exc.what(),
                 ArchGetDemangled(typeid(exc)).c_str(),
                 contextMsg.empty() ? "" : " thrown:\n -> ",
                 contextMsg.empty() ? "" : contextMsg.c_str(),
                 framesMsg.empty() ? "" : (
                     TfStringPrintf(" from\n    %s",
                                    framesMsg.c_str()).c_str()));

    // Attach the current c++ exception to the python exception so we can
    // rethrow it later, possibly, if we return from python back to C++.
    std::exception_ptr cppExc = std::current_exception();
    if (TF_VERIFY(cppExc)) {
        TfPyExceptionStateScope pyExcState;
        boost::python::object pyErr(pyExcState.Get().GetValue());
        uintptr_t cppExcAddr;
        std::unique_ptr<std::exception_ptr>
            cppExecPtrPtr(new std::exception_ptr(cppExc));
        std::exception_ptr *eptrAddr = cppExecPtrPtr.get();
        memcpy(&cppExcAddr, &eptrAddr, sizeof(cppExcAddr));
        // Stash it.
        pyErr.attr("_pxr_SavedTfException") = cppExcAddr;
        cppExecPtrPtr.release();
    }
}

// Exception class for unit test.
class _TestExceptionToPython : public TfBaseException
{
public:
    using TfBaseException::TfBaseException;
    virtual ~_TestExceptionToPython();
};

_TestExceptionToPython::~_TestExceptionToPython()
{
}

// Unit test support.
static void _ThrowTest(std::string message)
{
    TF_THROW(_TestExceptionToPython, message);
}

static void _CallThrowTest(boost::python::object fn)
{
    TfPyCall<void> callFn(fn);
    callFn();
}

void wrapException()
{
    char excClassName[] = "pxr.Tf.CppException"; // XXX: Custom py NS here?
    tfExceptionClass = PyErr_NewException(excClassName, NULL, NULL);

    // Expose the exception class to python.
    scope().attr("CppException") = boost::python::handle<>(tfExceptionClass);
    
    // Register the exception translator with boost::python.
    register_exception_translator<TfBaseException>(Translate);

    // Test support.
    boost::python::def("_ThrowTest", _ThrowTest);
    boost::python::def("_CallThrowTest", _CallThrowTest);
}
