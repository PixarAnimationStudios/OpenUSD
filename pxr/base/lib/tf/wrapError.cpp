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
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyTracing.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/tf/pyErrorInternal.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/tuple.hpp>

using std::string;
using std::vector;

using namespace boost::python;

static void
_RaiseCodingError(string const &msg,
                 string const& moduleName, string const& functionName,
                 string const& fileName, int lineNo)
{
    TfDiagnosticMgr::
        ErrorHelper(Tf_PythonCallContext(fileName.c_str(), moduleName.c_str(),
                                          functionName.c_str(), lineNo),
                    TF_DIAGNOSTIC_CODING_ERROR_TYPE,
                    TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_CODING_ERROR_TYPE)).
                    c_str()).
        Post("Python coding error: " + msg);
}

static void
_RaiseRuntimeError(string const &msg,
                 string const& moduleName, string const& functionName,
                 string const& fileName, int lineNo)                  
{
    TfDiagnosticMgr::
        ErrorHelper(Tf_PythonCallContext(fileName.c_str(), moduleName.c_str(),
                                          functionName.c_str(), lineNo),
                    TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE,
                    TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE)).c_str()).
        Post("Python runtime error: " + msg);
}

// CODE_COVERAGE_OFF This will abort the program.
static void
_Fatal(string const &msg, string const& moduleName, string const& functionName,
      string const& fileName, int lineNo)
{
    TfDiagnosticMgr::FatalHelper(Tf_PythonCallContext(fileName.c_str(), moduleName.c_str(),
                                                      functionName.c_str(), lineNo),
         TF_DIAGNOSTIC_FATAL_ERROR_TYPE). Post("Python Fatal Error: " + msg);
}
// CODE_COVERAGE_ON

static handle<>
_InvokeWithErrorHandling(tuple const &args, dict const &kw)
{
    // This function uses basically bare Python C API since it wants to be as
    // fast as it can be.
    TfErrorMark m;
    PyObject *argsp = args.ptr();
    // first tuple element is the callable.
    PyObject *callable = PyTuple_GET_ITEM(argsp, 0);
    // remove callable from positional args.
    handle<> args_tail(PyTuple_GetSlice(argsp, 1, PyTuple_GET_SIZE(argsp)));
    // call the callable -- if this raises a python exception, handle<>'s
    // constructor will throw a c++ exception which will exit this function and
    // return to python.
    handle<> ret(PyObject_Call(callable, args_tail.get(), kw.ptr()));
    // if the call completed successfully, then we need to see if any tf errors
    // occurred, and if so, convert them to python exceptions.
    if (not m.IsClean() and TfPyConvertTfErrorsToPythonException(m))
        throw_error_already_set();
    // if we made it this far, we return the result.
    return ret;
}

static string
TfError__repr__(TfError const &self) 
{
    string ret = TfStringPrintf("Error in '%s' at line %zu in file %s : '%s'",
             self.GetSourceFunction().c_str(),
             self.GetSourceLineNumber(),
             self.GetSourceFileName().c_str(),
             self.GetCommentary().c_str());

    if (const TfPyExceptionState* exc = self.GetInfo<TfPyExceptionState>()) {
        ret += "\n" + exc->GetExceptionString();
    }

    return ret;
}

static vector<TfError>
_GetErrors( const TfErrorMark & mark )
{
    return vector<TfError>(mark.GetBegin(), mark.GetEnd());
}

// Repost any errors contained in exc to the TfError system.  This is used for
// those python clients that do not intend to handle errors themselves, but need
// to continue executing.  This pushes them back on the TfError list for the
// next client to handle them, or it reports them, if there are no TfErrorMarks.
static bool
_RepostErrors(boost::python::object exc)
{
    // XXX: Must use the string-based name until bug XXXXX is fixed.
    const bool TF_ERROR_MARK_TRACKING =
        TfDebug::IsDebugSymbolNameEnabled("TF_ERROR_MARK_TRACKING");

    if (TF_ERROR_MARK_TRACKING and
        TfDiagnosticMgr::GetInstance().HasActiveErrorMark()) {
        if (TF_ERROR_MARK_TRACKING)
            printf("Tf.RepostErrors called with active marks\n");
        TfReportActiveErrorMarks();
    } else {
        if (TF_ERROR_MARK_TRACKING)
            printf("no active marks\n");
    }

    if ((PyObject *)exc.ptr()->ob_type ==
        Tf_PyGetErrorExceptionClass().get()) {
        object args = exc.attr("args");
        extract<vector<TfError> > extractor(args);
        if (extractor.check()) {
            vector<TfError> errs = extractor();
            if (errs.empty()) {
                if (TF_ERROR_MARK_TRACKING)
                    printf("Tf.RepostErrors: exception contains no errors\n");
                return false;
            }
            TF_FOR_ALL(i, errs)
                TfDiagnosticMgr::GetInstance().AppendError(*i);
            return true;
        } else {
            if (TF_ERROR_MARK_TRACKING)
                printf("Tf.RepostErrors: "
                       "failed to get errors from exception\n");
        }
    } else {
        if (TF_ERROR_MARK_TRACKING)
            printf("Tf.RepostErrors: invalid exception type\n");
    }
    return false;
}

static void
_PythonExceptionDebugTracer(TfPyTraceInfo const &info)
{
    if (info.what == PyTrace_EXCEPTION) {
        string excName = "<unknown>";
        if (PyObject *excType = PyTuple_GET_ITEM(info.arg, 0)) {
            if (PyObject *r = PyObject_Repr(excType)) {
                excName = PyString_AS_STRING(r);
                Py_DECREF(r);
            }
        }
        if (PyErr_Occurred())
            PyErr_Clear();
        printf("= PyExc: %s in %s %s:%d\n",
               excName.c_str(), info.funcName, info.fileName, info.funcLine);
    }
}

static void
_SetPythonExceptionDebugTracingEnabled(bool enable)
{
    static TfPyTraceFnId traceFnId;
    if (not enable) {
        traceFnId.reset();
    } else if (not traceFnId) {
        traceFnId = TfPyRegisterTraceFn(_PythonExceptionDebugTracer);
    }
}

void wrapError() {
    def("_RaiseCodingError", &::_RaiseCodingError);
    def("_RaiseRuntimeError", &::_RaiseRuntimeError);
    def("_Fatal", &::_Fatal);
    def("RepostErrors", &::_RepostErrors, arg("exception"));
    def("ReportActiveErrorMarks", TfReportActiveErrorMarks);
    def("SetPythonExceptionDebugTracingEnabled",
        _SetPythonExceptionDebugTracingEnabled, arg("enabled"));
    def("__SetErrorExceptionClass", Tf_PySetErrorExceptionClass);
    def("InvokeWithErrorHandling", raw_function(_InvokeWithErrorHandling, 1));
    TfPyContainerConversions::from_python_sequence< vector<TfError>,
        TfPyContainerConversions::variable_capacity_policy >();

    typedef TfDiagnosticBase Base;
    {
        class_<Base>("_DiagnosticBase", no_init)
            .add_property("sourceFileName",
                make_function(&Base::GetSourceFileName,
                              return_value_policy<return_by_value>()),
                              "The source file name that the error was posted from.")

            .add_property("sourceLineNumber", &Base::GetSourceLineNumber,
                          "The source line number that the error was posted from.")

            .add_property("commentary",
                make_function(&Base::GetCommentary,
                              return_value_policy<return_by_value>()),
                              "The commentary string describing this error.")

            .add_property("sourceFunction",
                make_function(&Base::GetSourceFunction,
                              return_value_policy<return_by_value>()),
                              "The source function that the error was posted from.")

            .add_property("diagnosticCode", &Base::GetDiagnosticCode,
                           "The diagnostic code posted.")

            .add_property("diagnosticCodeString",
                make_function(&Base::GetDiagnosticCodeAsString,
                              return_value_policy<return_by_value>()),
                              "The error code posted for this error, as a string.")

            /* XXX -- If we want to make info available to Python we'll need
             *        to store a PyObject generator along with the info.
            .add_property("info", &Base::GetInfo,
                          "The info object associated with this error.")
            */
        ;
    }

    typedef TfError This;

    scope errorScope = class_<This, bases<Base> >("Error", no_init)
        .add_property("errorCode", &This::GetErrorCode,
                       "The error code posted for this error.")

        .add_property("errorCodeString",
                      make_function(&This::GetErrorCodeAsString,
                                    return_value_policy<return_by_value>()),
                                    "The error code posted for this error, as a string.")

        .def("__repr__", TfError__repr__)
        ;

    class_<TfErrorMark, boost::noncopyable>("Mark")
        .def("SetMark", &TfErrorMark::SetMark)
        .def("IsClean", &TfErrorMark::IsClean)
        .def("Clear", &TfErrorMark::Clear)
        .def("GetErrors", &::_GetErrors,
            return_value_policy<TfPySequenceToList>(),
             "A list of the errors held by this mark.")
        ;
}
