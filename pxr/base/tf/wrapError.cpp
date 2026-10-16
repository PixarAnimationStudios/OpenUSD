//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyCallContext.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyErrorInternal.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyTracing.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static void
_RaiseCodingError(string const &msg,
                 string const& moduleName, string const& functionName,
                 string const& fileName, int lineNo)
{
    TfDiagnosticMgr::
        ErrorHelper(Tf_PythonCallContext(fileName, moduleName,
                                          functionName, lineNo),
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
        ErrorHelper(Tf_PythonCallContext(fileName, moduleName,
                                          functionName, lineNo),
                    TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE,
                    TfEnum::GetName(TfEnum(TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE)).c_str()).
        Post("Python runtime error: " + msg);
}

// CODE_COVERAGE_OFF This will abort the program.
static void
_Fatal(string const &msg, string const& moduleName, string const& functionName,
      string const& fileName, int lineNo)
{
    TfDiagnosticMgr::FatalHelper(Tf_PythonCallContext(fileName, moduleName,
                                                      functionName, lineNo),
         TF_DIAGNOSTIC_FATAL_ERROR_TYPE). Post("Python Fatal Error: " + msg);
}
// CODE_COVERAGE_ON

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

static void
_RaiseIfNotClean(const TfErrorMark &mark)
{
    if (!mark.IsClean()) {
        TfPyConvertTfErrorsToPythonException(mark);
        throw nanobind::python_error();
    }
}

// Repost any errors contained in exc to the TfError system.  This is used for
// those python clients that do not intend to handle errors themselves, but need
// to continue executing.  This pushes them back on the TfError list for the
// next client to handle them, or it reports them, if there are no TfErrorMarks.
static bool
_RepostErrors(nanobind::object exc)
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
        Tf_PyGetErrorExceptionClass().ptr()) {
        nanobind::object args = exc.attr("args");
        vector<TfError> errs;
        if (nanobind::try_cast<vector<TfError>>(args, errs)) {
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
                excName = PyUnicode_AsUTF8(r);
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

void wrapError(nanobind::module_& m) {
    namespace nb = nanobind;
    m.def("_RaiseCodingError", &_RaiseCodingError);
    m.def("_RaiseRuntimeError", &_RaiseRuntimeError);
    m.def("_Fatal", &_Fatal);
    m.def("RepostErrors", &_RepostErrors, nb::arg("exception"));
    m.def("ReportActiveErrorMarks", TfReportActiveErrorMarks);
    m.def("SetPythonExceptionDebugTracingEnabled",
        _SetPythonExceptionDebugTracingEnabled, nb::arg("enabled"));
    m.def("__SetErrorExceptionClass", &Tf_PySetErrorExceptionClass);

    typedef TfError This;

    nb::class_<This, TfDiagnosticBase> errorCls(m, "Error");
    errorCls
        .def_prop_ro("errorCode", &This::GetErrorCode,
                     "The error code posted for this error.")

        .def_prop_ro("errorCodeString", &This::GetErrorCodeAsString,
                     "The error code posted for this error, as a string.")

        .def("__repr__", TfError__repr__)
        ;

    nb::class_<TfErrorMark>(errorCls, "Mark")
        .def(nb::init<>())
        .def("SetMark", &TfErrorMark::SetMark)
        .def("IsClean", &TfErrorMark::IsClean)
        .def("Clear", &TfErrorMark::Clear)
        .def("GetErrors", &_GetErrors,
             "A list of the errors held by this mark.")
        .def("RaiseIfNotClean", &_RaiseIfNotClean,
             "If this mark is not clean, raise a Tf.ErrorException with "
             "the contained errors, otherwise do nothing.")
        ;
}
