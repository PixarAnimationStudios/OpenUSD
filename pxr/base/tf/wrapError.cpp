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

#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyCallContext.h"
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



PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
_RaiseCodingError(std::string const &msg,
                 std::string const& moduleName, std::string const& functionName,
                 std::string const& fileName, int lineNo)
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
_RaiseRuntimeError(std::string const &msg,
                 std::string const& moduleName, std::string const& functionName,
                 std::string const& fileName, int lineNo)                  
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
_Fatal(std::string const &msg, std::string const& moduleName, std::string const& functionName,
      std::string const& fileName, int lineNo)
{
    TfDiagnosticMgr::FatalHelper(Tf_PythonCallContext(fileName.c_str(), moduleName.c_str(),
                                                      functionName.c_str(), lineNo),
         TF_DIAGNOSTIC_FATAL_ERROR_TYPE). Post("Python Fatal Error: " + msg);
}
// CODE_COVERAGE_ON

static boost::python::handle<>
_InvokeWithErrorHandling(boost::python::tuple const &args, boost::python::dict const &kw)
{
    // This function uses basically bare Python C API since it wants to be as
    // fast as it can be.
    TfErrorMark m;
    PyObject *argsp = args.ptr();
    // first tuple element is the callable.
    PyObject *callable = PyTuple_GET_ITEM(argsp, 0);
    // remove callable from positional args.
    boost::python::handle<> args_tail(PyTuple_GetSlice(argsp, 1, PyTuple_GET_SIZE(argsp)));
    // call the callable -- if this raises a python exception, handle<>'s
    // constructor will throw a c++ exception which will exit this function and
    // return to python.
    boost::python::handle<> ret(PyObject_Call(callable, args_tail.get(), kw.ptr()));
    // if the call completed successfully, then we need to see if any tf errors
    // occurred, and if so, convert them to python exceptions.
    if (!m.IsClean() && TfPyConvertTfErrorsToPythonException(m))
        boost::python::throw_error_already_set();
    // if we made it this far, we return the result.
    return ret;
}

static std::string
TfError__repr__(TfError const &self) 
{
    std::string ret = TfStringPrintf("Error in '%s' at line %zu in file %s : '%s'",
             self.GetSourceFunction().c_str(),
             self.GetSourceLineNumber(),
             self.GetSourceFileName().c_str(),
             self.GetCommentary().c_str());

    if (const TfPyExceptionState* exc = self.GetInfo<TfPyExceptionState>()) {
        ret += "\n" + exc->GetExceptionString();
    }

    return ret;
}

static std::vector<TfError>
_GetErrors( const TfErrorMark & mark )
{
    return std::vector<TfError>(mark.GetBegin(), mark.GetEnd());
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

    if (TF_ERROR_MARK_TRACKING &&
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
        boost::python::object args = exc.attr("args");
        boost::python::extract<std::vector<TfError> > extractor(args);
        if (extractor.check()) {
            std::vector<TfError> errs = extractor();
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
        std::string excName = "<unknown>";
        if (PyObject *excType = PyTuple_GET_ITEM(info.arg, 0)) {
            if (PyObject *r = PyObject_Repr(excType)) {
                excName = TfPyString_AsString(r);
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
    if (!enable) {
        traceFnId.reset();
    } else if (!traceFnId) {
        traceFnId = TfPyRegisterTraceFn(_PythonExceptionDebugTracer);
    }
}

} // anonymous namespace 

void wrapError() {
    boost::python::def("_RaiseCodingError", &_RaiseCodingError);
    boost::python::def("_RaiseRuntimeError", &_RaiseRuntimeError);
    boost::python::def("_Fatal", &_Fatal);
    boost::python::def("RepostErrors", &_RepostErrors, boost::python::arg("exception"));
    boost::python::def("ReportActiveErrorMarks", TfReportActiveErrorMarks);
    boost::python::def("SetPythonExceptionDebugTracingEnabled",
        _SetPythonExceptionDebugTracingEnabled, boost::python::arg("enabled"));
    boost::python::def("__SetErrorExceptionClass", Tf_PySetErrorExceptionClass);
    boost::python::def("InvokeWithErrorHandling", boost::python::raw_function(_InvokeWithErrorHandling, 1));
    TfPyContainerConversions::from_python_sequence< std::vector<TfError>,
        TfPyContainerConversions::variable_capacity_policy >();

    typedef TfError This;

    boost::python::scope errorScope = boost::python::class_<This, boost::python::bases<TfDiagnosticBase> >("Error", boost::python::no_init)
        .add_property("errorCode", &This::GetErrorCode,
                       "The error code posted for this error.")

        .add_property("errorCodeString",
                      boost::python::make_function(&This::GetErrorCodeAsString,
                                    boost::python::return_value_policy<boost::python::return_by_value>()),
                                    "The error code posted for this error, as a string.")

        .def("__repr__", TfError__repr__)
        ;

    boost::python::class_<TfErrorMark, boost::noncopyable>("Mark")
        .def("SetMark", &TfErrorMark::SetMark)
        .def("IsClean", &TfErrorMark::IsClean)
        .def("Clear", &TfErrorMark::Clear)
        .def("GetErrors", &_GetErrors,
            boost::python::return_value_policy<TfPySequenceToList>(),
             "A list of the errors held by this mark.")
        ;
    
}
