//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/exception.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyErrorInternal.h"

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/exception_translator.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
        pxr_boost::python::object pyErr(pyExcState.Get().GetValue());
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

static void _CallThrowTest(pxr_boost::python::object fn)
{
    TfPyCall<void> callFn(fn);
    callFn();
}

void wrapException()
{
    char excClassName[] = "pxr.Tf.CppException"; // XXX: Custom py NS here?
    tfExceptionClass = PyErr_NewException(excClassName, NULL, NULL);

    // Expose the exception class to python.
    scope().attr("CppException") = pxr_boost::python::handle<>(tfExceptionClass);
    
    // Register the exception translator with pxr_boost::python.
    register_exception_translator<TfBaseException>(Translate);

    // Test support.
    pxr_boost::python::def("_ThrowTest", _ThrowTest);
    pxr_boost::python::def("_CallThrowTest", _CallThrowTest);
}
